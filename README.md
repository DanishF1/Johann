<div align="center">

# Johann

![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Hardware](https://img.shields.io/badge/Hardware-IoT-FFB900?style=for-the-badge)
![Source Available](https://img.shields.io/badge/Source_Available-%E2%99%A5-blue?style=for-the-badge)
![License: CC BY-NC 4.0](https://img.shields.io/badge/License-CC%20BY--NC%204.0-lightgrey.svg?style=for-the-badge)

*Small cutting edge drone that relies on BLE for communication. This repository contains the core firmware and hardware architecture.*

</div>

---

## About The Project

**Johann** is the physical drone hardware and firmware ecosystem designed to execute precise flight maneuvers. Built with a focus on low-latency communication and hardware integration, the drone relies on a Bluetooth Low Energy (BLE) server to receive real-time commands from its dedicated mobile command center. This repository houses the core logic, hardware configuration, and flight control firmware required to keep Johann in the air.

### Specifications & Features

* **Core Processing:** SoC RISC-V (ESP32C3F4)
* **Firmware Language:** C++ Arduino
* **Communication Protocol:** Bluetooth Low Energy (BLE) Server
* **Flight Capabilities:** * Real-time PWM motor control for stability and rapid response.
    * Interprets and executes state-based commands sent via BLE.
    * Smooth data parsing for continuous altitude adjustments.
* **Sensors & Hardware:**
    * Gyroscope/Accelerometer module for flight stabilization.
    * 1020 Coreless Motor
    * 1S 500MaH Li-Po Battery

---

## The Command Center

This drone requires a specialized mobile application to act as its remote controller. The Android app handles the high-contrast user interface, BLE client transmission, and flight state logic.

To explore the Android source code and UI/UX design of the controller, visit the companion repository below:

👉 **[Johann Universal Controller Repository](https://github.com/DanishF1/Johann-Universal-Controller)**

---

## 🤝 Contributing

Contributions, issues, and feature requests are welcome! Feel free to check the [issues page](https://github.com/DanishF1/Johann/issues). 

## 📝 License

This project is Source-Available and is licensed under the [Creative Commons Attribution-NonCommercial 4.0 International License](https://creativecommons.org/licenses/by-nc/4.0/).

You are free to use, modify, and distribute this software for personal and educational purposes, but **commercial use is strictly prohibited** without explicit permission from the author.
