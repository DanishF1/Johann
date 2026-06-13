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
float  PWMValue;
const int HOVER = (int)valueHOVER;
int DESCEND = (int)valueDESCEND;
int ASCEND = (int)valueASCEND;
bool flying = false;
bool hovering = false;
bool descending = false; 
bool ascending = false;
bool isSeekbar = false;
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

// KELAS CALLBACK: Ini adalah "Satpam" yang menjaga gerbang koneksi
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      hbTimer = millis();

    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      // Saat HP menjauh atau terputus, status diubah menjadi false.
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      hbTimer = millis();

      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.print("Data masuk dari HP: ");
        
        pesanMasuk = "";
        for (int i = 0; i < rxValue.length(); i++) {
          pesanMasuk += rxValue[i];
        }
        pesanMasuk.trim();
        if (pesanMasuk == "HOVER") {
            Serial.println("🚀 Hovering");
            hovering = true;
        }else if (pesanMasuk == "ASCEND") {
            Serial.println("🛬 Ascending");
            ascending = true;
            descending = false;
        }else if (pesanMasuk == "DESCEND") {
            Serial.println("🔻 Descending");
            descending = true;
            ascending = false;
        }else if (pesanMasuk == "STOP_HOVER") {
            Serial.println("🛑 Berhenti Hovering!");
            hovering = false;
        }else if (pesanMasuk == "STOP_ASCEND") {
            Serial.println("🛑 Berhenti Ascending!");
            ascending = false;
        }else if (pesanMasuk == "STOP_DESCEND") {
            Serial.println("🛑 Berhenti Descending!");
            descending = false;
        }else if (pesanMasuk == "BALANCING") {
            Serial.println("⚖️ Balancing...");
            balancing();
            ascending = false;
            descending = false;
        }else{
         Serial.println(pesanMasuk);
         float altitude = pesanMasuk.toFloat();
            if (altitude >= 0.01){
            isSeekbar = true;
            PWMValue = 255 * altitude;
            }
          }
      }else{
        Serial.println("Hanya Heartbeat");
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
  digitalWrite(LED, LOW);

  // 1. Memberi Nama (Mulai Memancarkan Eksistensi)
  BLEDevice::init("Johann 1.0"); 
 
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
  
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL0, ESP_PWR_LVL_P9);

  // 2. Membuat ESP32 menjadi Server (Slave)
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks()); 
  

  // 3. Membuat Layanan Utama (Service)
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. Membuat Karakteristik (Ibarat 'Laci' untuk menaruh data)
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic->setValue("Status: Standby");
  pCharacteristic->setCallbacks(new MyCallbacks());

  // 5. Menyalakan Layanan
  pService->start();
  
  // 6. Mulai Berteriak (Advertising) agar bisa ditemukan HP
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

  int i = 0;
  int rslt;
  int16_t accelGyro[6]={0};

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
      digitalWrite(LED, HIGH);
      oldDeviceConnected = deviceConnected; // TRUE TRUE
  }
  
  //  (Mode Operasional)
  if (deviceConnected) {
      digitalWrite(LED, HIGH);
      heartbeat();
  }
  delay(10);

}

void balancing() {
  if (isSeekbar){
    Serial.println("Seekbar diaktifkan. Nilai PWM: " + String(PWMValue));
          analogWrite(TRANS_KIRI_ATAS, PWMValue);
          analogWrite(TRANS_KIRI_BAWAH, PWMValue);
          analogWrite(TRANS_KANAN_ATAS, PWMValue);
          analogWrite(TRANS_KANAN_BAWAH, PWMValue);
          if (PWMValue > 0.4){
            flying = true;
          }
        }else if (hovering){
          flying = true;
          Serial.println("Hovering...");
          analogWrite(TRANS_KIRI_ATAS, hovering);
          analogWrite(TRANS_KIRI_BAWAH, hovering);
          analogWrite(TRANS_KANAN_ATAS, hovering);
          analogWrite(TRANS_KANAN_BAWAH, hovering);
        }else if (ascending){
          flying = true;
          Serial.println("Ascending...");
          analogWrite(TRANS_KIRI_ATAS, ascending);
          analogWrite(TRANS_KIRI_BAWAH, ascending);
          analogWrite(TRANS_KANAN_ATAS, ascending);
          analogWrite(TRANS_KANAN_BAWAH, ascending);
        }else if (descending){
          Serial.println("Descending...");
          analogWrite(TRANS_KIRI_ATAS, descending);
          analogWrite(TRANS_KIRI_BAWAH, descending);
          analogWrite(TRANS_KANAN_ATAS, descending);
          analogWrite(TRANS_KANAN_BAWAH, descending);
        }
}

void reconnect() {
      Serial.println("❌ HP Terputus! Bersiap melakukan reconnect...");
      
      delay(500); 
      
      pServer->startAdvertising(); // Paksa ESP32 berteriak/memancarkan sinyal lagi!
      Serial.println("📡 Memancarkan sinyal lagi. Silakan reconnect dari HP.");
      
      oldDeviceConnected = deviceConnected; //FALSE FALSE
}

void heartbeat(){
  if (millis() - hbTimer >= 30000){
    Serial.println("🚨 Heartbeat MATI, HP HILANG/TERPUTUS!");
    deviceConnected = false;
    oldDeviceConnected = true;
  } 
}