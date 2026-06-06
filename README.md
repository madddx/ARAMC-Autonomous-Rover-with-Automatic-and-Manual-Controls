# ARAMC – Autonomous Rover with Automatic and Manual Controls

The **ARAMC Autonomous Rover** is a hybrid robotic system capable of operating in both **manual** and **fully autonomous** modes. 
This repository contains all source code, experimental trial programs, test scripts, and wiring references used throughout the development of the rover.

## 🚗 Project Overview

This rover is built using low-cost, readily available components such as:

- Arduino UNO  
- ESP8266 NodeMCU modules  
- L298N Motor Driver  
- 4-Wheel DC Motor Chassis  
- Ultrasonic Sensors  
- Pan & Tilt Servos  
- Battery Module  
- Breadboard and Jumper Wires  

The system supports:

- Manual Control (Wi-Fi / NodeMCU)  
- Fully Autonomous Navigation  
- Obstacle Avoidance  
- Directional Scanning with Servos  
- Prototype CNN (future upgrade)  

## 🔧 Features

### ✔ Manual Mode
- Remote control of rover movement  
- Ideal for testing basic motor functions and navigation  

### ✔ Autonomous Mode
- Moves forward when path is clear  
- Stops when an obstacle is nearby  
- Reverses and turns to avoid collisions  
- Uses servo scanning pattern:  
  `0°, 30°, 60°, 90°, 120°, 150°, 180°, 150°, 120°, 90°, 60°, 30°`  
- Distance-based directional decision-making  
- Pan–tilt mechanism for safe path detection  

### ✔ Planned Future Upgrades
- ESP32-CAM for CNN/ML object detection  
- SD card logging  
- ESP8266 Wi-Fi dashboard  
- GPS module support  




## 🛠 Hardware Requirements

- Arduino UNO  
- L298N Motor Driver  
- ESP8266 NodeMCU  
- 4 × DC Motors + Rover Chassis  
- 2 × Ultrasonic Sensors  
- 2 × Servos (Pan & Tilt) 
- Switches 
- Battery Pack  
- Breadboard  
- Jumper Wires  

## 🔌 Software Requirements

- Arduino IDE  
- ESP8266 Board Package  
- Servo Library  
- Ultrasonic Library  
- L298N Motor Driver Library (optional)  

## ⚠ Important: CH340 Driver Fix

Many Arduino-compatible boards use the **CH340 USB-to-Serial chip**.  
If you encounter this upload error:

avrdude: ser_open(): can't set com-state for "\.\COMx"


Install the CH340 driver from:  
https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers

This resolves upload failures in Arduino IDE.

## ▶️ How to Use

### 1. Flash the Code
Choose the required trail code to manually ensure all the components works accordingly

### 2. Servos
- Upload Servo Centre code to point the servos to centre and then manually align it to centre
- Attach the Ultrasonic Sensor  
- Mount this on car
- Upload the Servo and HC-SR04 Ultrasonic Sensor.ino in Arduino IDE

### 3. Ultra Sonic Sensor Setup
- Make the Proper Connection which was shown in circuit Diagram
- Install Processing 3 Application
- Upload Processing 3.pde code in the application

### 4. Power Up
Turn on the power module.  


## 📝 License

This project is licensed under the **MIT License**.  


## 🤝 Contributing

Contributions are welcome!  
Feel free to open a pull request for improvements in navigation, ML integration, or sensor performance.

## 📬 Contact

Connect with me on LinkedIn: [Madhesh H](https://www.linkedin.com/in/madheshh/)
For questions, suggestions, or collaborations, open an issue or start a discussion in this repository.
