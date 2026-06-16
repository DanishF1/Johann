#include <Arduino.h>
#include <DFRobot_BMI160.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>

#define TRANS_KIRI_ATAS 5
#define TRANS_KIRI_BAWAH 6
#define TRANS_KANAN_ATAS 4
#define TRANS_KANAN_BAWAH 3
#define LED 8

const int FULL = 255;
const int EMPTY = 0;
float valueHOVER = FULL*0.65; 
float valueDESCEND = FULL*0.425;
float valueASCEND = FULL*0.875;
float PWMValue;
const int HOVER = (int)valueHOVER;
int DESCEND = (int)valueDESCEND;
int ASCEND = (int)valueASCEND;
bool flying = false;
bool hovering = false;
bool descending = false; 
bool ascending = false;
bool isSeekbar = false;
int joyX = 0;
int joyY = 0;
String pesanMasuk;
unsigned long previousMillis = 0; 
const long interval = 1000;       
unsigned long heartbeatMillis = 0;
unsigned long hbTimer;

void reconnect();
void heartbeat();
void balancing();

BLEServer* pServer = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      hbTimer = millis();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      hbTimer = millis();

      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() == 0) {
        Serial.println("Hanya Heartbeat");
        return;
      }

      pesanMasuk = "";
      for (int i = 0; i < rxValue.length(); i++) {
        pesanMasuk += rxValue[i];
      }
      pesanMasuk.trim();

      // Format dari Kotlin: "state,altitude,joyX,joyY"
      // Contoh: "HOVER,50,30,-20" atau ",75,0,0" atau "STOP,0,0,0"
      int c1 = pesanMasuk.indexOf(',');
      int c2 = pesanMasuk.indexOf(',', c1 + 1);
      int c3 = pesanMasuk.indexOf(',', c2 + 1);

      if (c1 == -1 || c2 == -1 || c3 == -1) {
        Serial.println("Format tidak dikenali: " + pesanMasuk);
        return;
      }

      String stateStr = pesanMasuk.substring(0, c1);
      int altitude    = pesanMasuk.substring(c1 + 1, c2).toInt();
      joyX            = pesanMasuk.substring(c2 + 1, c3).toInt();
      joyY            = pesanMasuk.substring(c3 + 1).toInt();

      Serial.println("State:" + stateStr + " Alt:" + String(altitude) + " X:" + String(joyX) + " Y:" + String(joyY));

      if (stateStr == "HOVER") {
        hovering = true; ascending = false; descending = false; isSeekbar = false;
      } else if (stateStr == "ASCEND") {
        ascending = true; hovering = false; descending = false; isSeekbar = false;
      } else if (stateStr == "DESCEND") {
        descending = true; hovering = false; ascending = false; isSeekbar = false;
      } else if (stateStr == "STOP") {
        hovering = false; ascending = false; descending = false; isSeekbar = false;
        flying = false;
        joyX = 0;
        joyY = 0;
      } else {
        // State kosong ("") — gunakan altitude seekbar
        isSeekbar = true;
        hovering = false; ascending = false; descending = false;
        PWMValue = 255 * (altitude / 100.0);
      }
    }
};



void setup() {
  Serial.begin(115200);
  Serial.println("\nMemulai Setup Sistem...");
  pinMode(TRANS_KIRI_ATAS, OUTPUT);
  pinMode(TRANS_KIRI_BAWAH, OUTPUT);
  pinMode(TRANS_KANAN_ATAS, OUTPUT);
  pinMode(TRANS_KANAN_BAWAH, OUTPUT);
  pinMode(LED, OUTPUT);

  analogWrite(TRANS_KIRI_ATAS, EMPTY);
  analogWrite(TRANS_KIRI_BAWAH, EMPTY);
  analogWrite(TRANS_KANAN_ATAS, EMPTY);
  analogWrite(TRANS_KANAN_BAWAH, EMPTY);

  BLEDevice::init("Johann 1.0"); 
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL0, ESP_PWR_LVL_P9);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks()); 

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic->setValue("Status: Standby");
  pCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true); 
  BLEDevice::startAdvertising();
  Serial.println("📡 Menyalakan Bluetooth. Silakan cari 'Johann 1.0' di HP Anda dan hubungkan.");

  Serial.println("Setup Selesai.");
  Serial.println("Bluetooth power maksimal");
  deviceConnected = false;
  oldDeviceConnected = false;
}

void loop() {
  unsigned long currentMillis = millis();
  heartbeat();
  int i = 0;
  int rslt;
  int16_t accelGyro[6]={0};
 
  if (deviceConnected){
    digitalWrite(LED, LOW);
  }else if(!deviceConnected){
    digitalWrite(LED, HIGH);
  }

// SKENARIO 1: HP Tiba-tiba Terputus (Putus Koneksi / Jauh)
  if (!deviceConnected && oldDeviceConnected) {
    reconnect();
    Serial.println("🔄 Mencoba Reconnect...");
      if (flying){
          analogWrite(TRANS_KIRI_ATAS, DESCEND);
          analogWrite(TRANS_KIRI_BAWAH, DESCEND);
          analogWrite(TRANS_KANAN_ATAS, DESCEND);
          analogWrite(TRANS_KANAN_BAWAH, DESCEND);
      }
      digitalWrite(LED, LOW);
  }
 
  // SKENARIO 2: HP Baru Saja Terhubung (Connect)
  if (deviceConnected && !oldDeviceConnected) {
      Serial.println("✅ HP Berhasil Terhubung! Gerbang dikunci untuk perangkat lain.");
          analogWrite(TRANS_KIRI_ATAS, EMPTY);
          analogWrite(TRANS_KIRI_BAWAH, EMPTY);
          analogWrite(TRANS_KANAN_ATAS, EMPTY);
          analogWrite(TRANS_KANAN_BAWAH, EMPTY);
      oldDeviceConnected = deviceConnected; // TRUE TRUE
  }
  
  //  (Mode Operasional)
  if (deviceConnected) {
   if (isSeekbar){
    Serial.println("Seekbar diaktifkan. Nilai PWM: " + String(PWMValue));
          analogWrite(TRANS_KIRI_ATAS, PWMValue);
          analogWrite(TRANS_KIRI_BAWAH, PWMValue);
          analogWrite(TRANS_KANAN_ATAS, PWMValue);
          analogWrite(TRANS_KANAN_BAWAH, PWMValue);
          if (PWMValue > 0.425){
            flying = true;
          }
        } else if (hovering) {
           analogWrite(TRANS_KIRI_ATAS, HOVER);
           analogWrite(TRANS_KIRI_BAWAH, HOVER);
           analogWrite(TRANS_KANAN_ATAS, HOVER);
           analogWrite(TRANS_KANAN_BAWAH, HOVER);
           flying = true;

       } else if (ascending) {
           analogWrite(TRANS_KIRI_ATAS, ASCEND);
           analogWrite(TRANS_KIRI_BAWAH, ASCEND);
           analogWrite(TRANS_KANAN_ATAS, ASCEND);
           analogWrite(TRANS_KANAN_BAWAH, ASCEND);
           flying = true;

       } else if (descending) {
           analogWrite(TRANS_KIRI_ATAS, DESCEND);
           analogWrite(TRANS_KIRI_BAWAH, DESCEND);
           analogWrite(TRANS_KANAN_ATAS, DESCEND);
           analogWrite(TRANS_KANAN_BAWAH, DESCEND);
           flying = true;

       } else{
           analogWrite(TRANS_KIRI_ATAS, EMPTY);
           analogWrite(TRANS_KIRI_BAWAH, EMPTY);
           analogWrite(TRANS_KANAN_ATAS, EMPTY);
           analogWrite(TRANS_KANAN_BAWAH, EMPTY);
           flying = false;
       }
  }
  delay(10);

}

void balancing() {
  
}

void reconnect() {
  Serial.println("❌ HP Terputus! Bersiap melakukan reconnect...");
  delay(500); 
  pServer->startAdvertising();
  Serial.println("📡 Memancarkan sinyal lagi. Silakan reconnect dari HP.");
  oldDeviceConnected = deviceConnected;
}

void heartbeat(){
  if (millis() - hbTimer >= 3000){
    Serial.println("🚨 Heartbeat MATI, HP HILANG/TERPUTUS!");
    deviceConnected = false;
    oldDeviceConnected = true;
  } 
}
