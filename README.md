# 🚀 Mars Rover — WiFi-Controlled Robotic Platform

A four-wheeled Mars rover prototype controlled remotely over WiFi through a browser-based interface. Built around an **Arduino Mega 2560** for motor and sensor control, paired with an **ESP32-CAM** that hosts the control webpage and streams live video from the rover's perspective.

> Diploma project — National Vocational High School of Computer Technologies and Systems, Pravets (NPGKTS)

---

## ✨ Features

- 🎥 **Live video streaming** — MJPEG stream from the ESP32-CAM directly in the browser
- 🌐 **Browser-based control** — No app needed; just connect to the rover's IP address
- 🛞 **Tank steering** — Differential drive via dual L298N H-bridge drivers (4 motors total)
- 🌡️ **Environmental telemetry** — Temperature and humidity readings from a DHT11 sensor
- 📐 **Motion sensing** — ADXL345 accelerometer over I2C for tilt/orientation data
- 🔋 **Single-battery design** — One 2S 18650 pack powers both motors and logic via an LM2596 buck converter
- 📡 **UART bridge** — ESP32-CAM forwards single-character commands (F/B/L/R/S) to the Arduino Mega over Serial1

---

## 🛠️ Hardware

| Component | Quantity | Purpose |
|---|---|---|
| Arduino Mega 2560 | 1 | Main controller — motors, sensors |
| ESP32-CAM (AI Thinker) | 1 | WiFi, web server, camera streaming |
| L298N Dual H-Bridge Driver | 2 | Motor control (one per side) |
| DC Gear Motor + Wheel | 4 | Drivetrain |
| 18650 Li-Ion Battery (2S) | 2 | Power source (~7.4V) |
| 2S 18650 Battery Holder | 1 | Battery housing |
| LM2596 Buck Converter | 1 | Steps down 7.4V → 5V for logic |
| DHT11 Sensor | 1 | Temperature + humidity |
| ADXL345 Accelerometer | 1 | I2C motion sensor |
| HC-SR04 Ultrasonic Sensor | 1 | Obstacle detection (planned) |
| Breadboard + jumper wires | — | GND distribution & prototyping |

---
