# CardioGuard
Smart Wristband detecting Cardiac abnormalities and potential heart attacks through continuous physiological monitoring.
# 🩺 CardioGuard

**Smart wristband detecting cardiac abnormalities and potential heart attacks through continuous physiological monitoring.**

---

## 📌 Problem Overview
Traditional health checks are periodic and make continuous tracking of physiological parameters difficult. Vulnerable groups—such as the elderly and individuals living alone—often lack instant detection during sudden falls or abnormal cardiac events. 

**CardioGuard** solves this by offering an ESP32-based wearable system that continuously monitors vital signs, identifies anomalies, and provides instant local emergency alerts.

---

## ✨ Features
- **Continuous Vitals Tracking:** Monitors ECG/Heart Rate, SpO2, and body temperature.
- **Fall & Motion Detection:** Utilizes an MPU6050 accelerometer/gyroscope to capture sudden impacts and falls.
- **Local Alert System:** Immediate emergency feedback through a piezo buzzer, LED indicators, and vibration signaling[cite: 1].
- **Interactive Controls:** Dedicated SOS emergency trigger button and a Reset/Cancel button[cite: 1].
- **On-Device Display:** Real-time metrics and alerts shown on a 128×64 I2C OLED screen[cite: 1].
- **Simulation Ready:** Fully compatible with Wokwi ESP32 simulation[cite: 1].

---

## 🏗️ Technical Architecture
[ Physiological Inputs ]       [ Motion Monitoring ]
(ECG/HR, SpO2, Temp)               (MPU6050)
│                             │
└──────────────┬──────────────┘
▼
[ ESP32 Microcontroller ]
(Data Acquisition & Processing)
│
┌────────────────┴────────────────┐
▼                                 ▼
[ Abnormality Detection ]          [ OLED Display ]
│                        (Real-time Values)
▼
[ Emergency Alert System ]
(Buzzer + LED / Vibration)
---

## 🛠️ Hardware & Tech Stack

### Hardware Components
- **Microcontroller:** ESP32 DevKit[cite: 1]
- **Motion Sensor:** MPU6050 (6-axis accelerometer & gyroscope)[cite: 1]
- **Temperature Sensor:** DS18B20[cite: 1]
- **Display:** SSD1306 128×64 OLED (I2C)[cite: 1]
- **Simulation Inputs:** 2× Potentiometers (simulating ECG/HR and SpO2)[cite: 1]
- **Buttons:** SOS button (`INPUT_PULLUP`) and Reset/Cancel button[cite: 1]
- **Alerts:** Piezo buzzer, Status LEDs (Red LED simulates vibration motor)[cite: 1]

### Software & Libraries
- **Language / Framework:** C/C++ (Arduino Core for ESP32)[cite: 1]
- **Development Tool:** Arduino IDE / Wokwi Simulator[cite: 1]
- **Key Libraries:** `Wire.h`, `Adafruit_SSD1306.h`, `Adafruit_GFX.h`, `OneWire.h`, `DallasTemperature.h`[cite: 1]
- **Timing:** Non-blocking 1-second update loops via `millis()`[cite: 1]
- **Baud Rate:** 115200 bps[cite: 1]

---

## 🚀 How to Run

### Using Wokwi Simulator
1. Open your Wokwi project using `wokwi-project.txt` and `diagram.json`.
2. Start the simulation.
3. Adjust the potentiometers to simulate fluctuating Heart Rate and SpO2 readings[cite: 1].
4. Press the SOS pushbutton to trigger the emergency alert[cite: 1].

### Local Hardware Setup
1. Clone the repository:
   ```bash
   git clone [https://github.com/Ritwik-Coder/CardioGuard.git](https://github.com/Ritwik-Coder/CardioGuard.git)

2. Open sketch.ino in the Arduino IDE.

3. Install the required libraries via the Arduino Library Manager (Adafruit SSD1306, Adafruit GFX, DallasTemperature).

4. Select ESP32 Dev Module under Tools > Board.

5. Compile and upload to the board.

🔮 Future Scope
Replace potentiometer simulation with clinical-grade physical sensors (AD8232 ECG sensor and MAX30102 pulse oximeter)[cite: 1].

Add Wi-Fi/Bluetooth support for caregiver mobile applications[cite: 1].

Integrate GPS coordinates for location-aware emergency dispatch[cite: 1].

Design a compact, miniaturized custom PCB for wearable form factor[cite: 1].

👥 Project Details
Team Name: CTRL+CHAOS[cite: 1]

Theme: Healthcare & Safety[cite: 1]

