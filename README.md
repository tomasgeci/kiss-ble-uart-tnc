# KISS BLE to UART adapter for APRS.FI iOS/iPadOS app

## Credits

- https://github.com/islandmagic/ble-bt-tnc
  - **Special thanks to Georges Auberger from Island Magic Co. for good inspiration**
- https://github.com/nkolban/ESP32_BLE_Arduino
- https://github.com/nkolban/esp32-snippets/issues/796
- https://stackoverflow.com/questions/73781856/how-to-send-bluetooth-data-from-esp32-in-arduino

## Hardware

The code is compatible with the ESP32. Use ESP32 DevKit, Wroom or other boards with native BLE support.
Boards from chinese market tested with arduino IDE 2.x:
  - DOIT ESP32 DEVKIT (most frequent version of the board widely available)
  - ESP32 DEV MODULE
  - ESP32 DEV BOARD
  - ESP32 WROOM

### Params

- power consumption:

  - default CPU clock: ~80mA
  - CPU clock on 80MHz: ~70mA
  - only 80, 160, 240MHz clock frequencies are supported

For CPU clock check `setCpuFrequencyMhz(10);` line

## HowTo

- Install Arduino IDE [https://www.arduino.cc](https://www.arduino.cc)
- Install the ESP32 libraries. Follow this guide [https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#installing-using-boards-manager](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#installing-using-boards-manager)
- Download this sketch
- Flash sketch to ESP32 board (select appropriate board from board manager - see Hardware section)
- Connect your KISS TNC via 3.3V UART pins RX-GPIO16, TX-GPIO17 (with Arduino-like TNC use voltage divider to avoid problems with 5V non-tolerant inputs of ESP32)
- Connect APRS.FI iOS/iPadOS app via `Select TNC or software modem` choose `Aprs.fi TNC` in `BLE TNC discovered`

## License

- GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007

VY 73 de OM5AST
