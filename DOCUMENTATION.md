# ESP32 Keyboard Emulator 2.0.17

This guide explains how to use your ESP32 as a Bluetooth keyboard.

## 📋 What is this?

**ESP32 Keyboard Emulator 2.0.17** is software that turns your ESP32 board into a Bluetooth keyboard. You can type on your computer or phone by using a web page on another device. It also has a small screen (OLED) to show you what is happening.

### Main Features
*   **Bluetooth Keyboard**: Works like a real keyboard for Windows, Mac, Phones, and Tablets.
*   **Web Control**: You can type from a web page on your phone or laptop.
*   **See Status**: The small screen shows your WiFi and Connection.
*   **Shortcuts**: Buttons to Copy, Paste, and Show Desktop.
*   **Media Keys**: Buttons to change volume or play music.

---

## 🛠 What You Need (Hardware)

To make this, you need:

1.  **ESP32 Board**: The main computer chip.
2.  **OLED Screen**: A small 0.96 inch screen (Model: SSD1306).
3.  **USB Cable**: To plug it into your computer.
4.  **Wires**: To connect the screen to the ESP32.

### How to Connect the Screen
Connect the pins like this:

| ESP32 Pin | Screen Pin |
| :--- | :--- |
| 3.3V or VIN | VCC |
| GND | GND |
| Pin 21 | SDA |
| Pin 22 | SCL |


## 💻 Computer Setup (Software)

You need these programs on your computer:

1.  **Arduino IDE**: Download it from the Arduino website.
2.  **ESP32 Add-on**: Install "ESP32" in the Arduino Board Manager.

### Libraries You Need
In Arduino IDE, go to `Sketch` -> `Include Library` -> `Manage Libraries` and find these:

*   **ESP32 Ble Keyboard** (by T-vK)
*   **Adafruit GFX Library**
*   **Adafruit SSD1306**

---

## 🚀 How to Install

### 1. Set WiFi Name and Password
Open the file `KeyBoard.ino`. Look for these lines near the top:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```
Change `YOUR_WIFI_SSID` to your WiFi name.
Change `YOUR_WIFI_PASSWORD` to your WiFi password.

### 2. Put Code on ESP32
1.  Plug your ESP32 into your computer.
2.  In Arduino IDE, pick your board and port.
3.  Click the **Arrow Button** (Upload) to send the code.

### 3. Check if it Works
When finished, the small screen will turn on. It will try to connect to your WiFi.
When connected, it will show an **IP Address** (like `192.168.1.50`) and say **"Ready"**.

---

## 📖 How to Use

### Step 1: Connect Bluetooth
1.  Go to Bluetooth settings on your computer or phone.
2.  Search for devices.
3.  Click on **"Keyboard"**.
4.  The small screen will show **"BT"** when connected.

### Step 2: Open the Web Page
1.  Read the **IP Address** on the small screen.
2.  Open a web browser (Chrome, Safari, etc.) on your phone or laptop.
3.  Type the IP address in the top bar.
4.  You will see the Keyboard Control page.

### Step 3: Type and Control
*   **Type Text**: Write in the box and click **Start Typing**.
*   **Speed**: Change the number to type faster or slower.
*   **Shortcuts**: Click buttons to Copy, Paste, or go to Desktop.

---

## ⚠️ Fix Common Problems

*   **Screen is black**: Check your wires. Make sure VCC goes to Power and GND goes to Ground.
*   **Bluetooth won't connect**: Turn Bluetooth off and on, or restart the ESP32.
*   **WiFi won't connect**: Check your WiFi name and password in the code. Make sure you use 2.4GHz WiFi.

---
*Version 2.0.17*
