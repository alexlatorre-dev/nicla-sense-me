# Nicla Sense ME - BLE Sensor Monitor Firmware

This repository contains the Arduino firmware (`ble_full.ino`) for the **Nicla Sense ME** board. It broadcasts sensor data (orientation, gravity, acceleration, environment) via **Bluetooth Low Energy (BLE)** and provides visual feedback through the RGB LED.

## Features

- **BLE Broadcasting**: Streams data from BHY2 sensors custom characteristics.
- **Power Saving**: Automatically stops sensor updates and enters a low-power wait state when no BLE client is connected.
- **Smart LED Indication**:
  - **Advertising**: Blue Single Blink (Flash... Pause).
  - **Connected**: Blue Double Blink (Flash-Flash... Pause).
  - **Charging**: Green Slow Blink (High Priority).
  - **Fully Charged**: Solid Green (Highest Priority).
  - **Low Battery**: Red Rapid Blink.
- **Sensor Fallback**: Automatically switches to Accelerometer readings if the virtual Gravity sensor returns (0,0,0) (sensor idle/off).

## Hardware Requirements



- **Arduino Nicla Sense ME**
- Micro-USB cable
- **LiPo Battery**: 3.7V 150mAh (via JST connector) for portable operation.

<p align="center">
  <img src="assets/photo_2026-01-13_12-21-23_1.jpg" alt="Nicla Sense ME" width="300" />
  <img src="assets/photo_2026-01-13_12-21-23_2.jpg" alt="Nicla with Battery" width="300" />
</p>

## Dependencies

Required Arduino Libraries:
- `Nicla_System`
- `Arduino_BHY2`
- `ArduinoBLE`

## Installation

### Using Arduino CLI

1.  **Install the Core**:
    ```sh
    arduino-cli core install arduino:mbed_nicla
    ```

2.  **Install Libraries**:
    ```sh
    arduino-cli lib install "Arduino_BHY2"
    arduino-cli lib install "ArduinoBLE"
    ```

3.  **Compile**:
    ```sh
    arduino-cli compile --fqbn arduino:mbed_nicla:nicla_sense .
    ```

4.  **Upload**:
    Connect the board via USB (e.g., to `COM4`) and run:
    ```sh
    arduino-cli upload -p COM4 --fqbn arduino:mbed_nicla:nicla_sense .
    ```
    > **Note**: If upload fails, double-press the reset button to enter bootloader mode (pulsing green LED) and try again.

### Using Arduino IDE

1.  Open `ble_full.ino` in Arduino IDE.
2.  Install the **Arduino Mbed OS Nicla Boards** via Board Manager.
3.  Install **Arduino_BHY2** and **ArduinoBLE** via Library Manager.
4.  Select the board and port, then click **Upload**.

## BLE UUIDs Reference

The firmware uses the following 128-bit UUIDs for the service and characteristics:

| Characteristic | UUID | Access |
| :--- | :--- | :--- |
| **Service** | `19B10000-0000-0000-0000-000000000000` | - |
| Heading | `...0001-...` | Read \| Notify |
| Pitch | `...0002-...` | Read \| Notify |
| Roll | `...0003-...` | Read \| Notify |
| Gravity Vector | `...0004-...` | Read \| Notify |
| Acceleration | `...0013-...` | Read \| Notify |
| Temperature | `...0007-...` | Read \| Notify |
# Nicla Sense ME - BLE Sensor Monitor Firmware

This repository contains the Arduino firmware (`ble_full.ino`) for the **Nicla Sense ME** board. It broadcasts sensor data (orientation, gravity, acceleration, environment) via **Bluetooth Low Energy (BLE)** and provides visual feedback through the RGB LED.

## Features

- **BLE Broadcasting**: Streams data from BHY2 sensors custom characteristics.
- **Power Saving**: Automatically stops sensor updates and enters a low-power wait state when no BLE client is connected.
- **Smart LED Indication**:
  - **Advertising**: Blue Single Blink (Flash... Pause).
  - **Connected**: Blue Double Blink (Flash-Flash... Pause).
  - **Charging**: Green Slow Blink (High Priority).
  - **Fully Charged**: Solid Green (Highest Priority).
  - **Low Battery**: Red Rapid Blink.
- **Sensor Fallback**: Automatically switches to Accelerometer readings if the virtual Gravity sensor returns (0,0,0) (sensor idle/off).

## Hardware Requirements

- **Arduino Nicla Sense ME**
- Micro-USB cable

## Dependencies

Required Arduino Libraries:
- `Nicla_System`
- `Arduino_BHY2`
- `ArduinoBLE`

## Installation

### Using Arduino CLI

1.  **Install the Core**:
    ```sh
    arduino-cli core install arduino:mbed_nicla
    ```

2.  **Install Libraries**:
    ```sh
    arduino-cli lib install "Arduino_BHY2"
    arduino-cli lib install "ArduinoBLE"
    ```

3.  **Compile**:
    ```sh
    arduino-cli compile --fqbn arduino:mbed_nicla:nicla_sense .
    ```

4.  **Upload**:
    Connect the board via USB (e.g., to `COM4`) and run:
    ```sh
    arduino-cli upload -p COM4 --fqbn arduino:mbed_nicla:nicla_sense .
    ```
    > **Note**: If upload fails, double-press the reset button to enter bootloader mode (pulsing green LED) and try again.

### Using Arduino IDE

1.  Open `ble_full.ino` in Arduino IDE.
2.  Install the **Arduino Mbed OS Nicla Boards** via Board Manager.
3.  Install **Arduino_BHY2** and **ArduinoBLE** via Library Manager.
4.  Select the board and port, then click **Upload**.

## BLE UUIDs Reference

The firmware uses the following 128-bit UUIDs for the service and characteristics:

| Characteristic | UUID | Access |
| :--- | :--- | :--- |
| **Service** | `19B10000-0000-0000-0000-000000000000` | - |
| Heading | `...0001-...` | Read \| Notify |
| Pitch | `...0002-...` | Read \| Notify |
| Roll | `...0003-...` | Read \| Notify |
| Gravity Vector | `...0004-...` | Read \| Notify |
| Acceleration | `...0013-...` | Read \| Notify |
| Temperature | `...0007-...` | Read \| Notify |
| Humidity | `...0008-...` | Read \| Notify |
| Pressure | `...0009-...` | Read \| Notify |
| Gas (eCO2) | `...0010-...` | Read \| Notify |
| Battery State | `...0011-...` | Read \| Notify |
| Altitude | `...0012-...` | Read \| Notify |

## License
    
This project is licensed under the MIT License - see the `LICENSE` file for details.
This project is provided as-is for educational and development purposes.

## Web Interface

A web dashboard is available to visualize the sensor data. 
The interface is located in the `docs` folder, making it ready for **GitHub Pages**.

<p align="center">
  <img src="assets/dashboard_image.png" alt="Web Dashboard Interface" width="600" />
</p>

### Deployment (GitHub Pages)

1.  Go to your GitHub repository **Settings**.
2.  Navigate to the **Pages** section (sidebar).
3.  Under **Build and deployment** > **Branch**, select `main` and choose the `/docs` folder.
4.  Click **Save**. Your site will be live at `https://<your-username>.github.io/<repo-name>/`.
