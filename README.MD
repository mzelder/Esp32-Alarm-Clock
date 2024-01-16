# ESP32 Alarm Clock and Notification System

## Project Overview

This project involves creating an alarm clock and notification system using two ESP32 modules. One ESP32 module (receiver) is connected to a DFPlayer Mini for audio output, an ST7735 TFT display for visual feedback, and buttons for user input. The other ESP32 module (sender) is set up to send notifications to the receiver.

## Features

- Time display on the TFT screen.
- Alarm setup and notification.
- Audio feedback through the DFPlayer Mini.
- Wireless communication between two ESP32 modules using ESP-NOW.

## Hardware Requirements

- 2 x ESP32 Development Boards
- DFPlayer Mini
- ST7735 TFT Display
- Active Buzzer
- Buttons
- Jumper Wires and Breadboard
- External Speaker (for DFPlayer Mini)

## Software Dependencies

- Adafruit GFX Library
- Adafruit ST7735 Library
- DFRobotDFPlayerMini Library
- ESP-NOW Library
- NTPClient Library

## Setup and Configuration

### Receiver Module

1. Connect the ST7735 display and buttons to the ESP32 as per the defined pin configurations.
2. Connect the DFPlayer Mini to the ESP32 using a SoftwareSerial connection.
3. Update the Wi-Fi credentials in the code to match your network.

### Sender Module

1. Connect a button and LED to the ESP32 as per the defined pin configurations.
2. Update the Wi-Fi credentials and the MAC address of the receiver module in the code.

## Usage

1. The receiver module displays the current time and allows setting an alarm using buttons.
2. Once the alarm time is reached, the receiver sends a signal to the sender module.
3. The sender module, upon receiving the signal, activates the LED, simulating a notification.
4. The receiver plays a sound using the DFPlayer Mini as an alarm.

## Known Issues

- The sender cannot send data at the same moment the receiver tries to get data from Wi-Fi for hour management. This limitation requires careful timing of communications.

## Future Enhancements

- Improve synchronization between the two ESP32 modules.
- Add more interactive features to the alarm system.
- Implement power-saving modes for prolonged battery life.

## Contributing

Feel free to fork this project and contribute to its development. Submit pull requests with enhancements or bug fixes for review and inclusion.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE

