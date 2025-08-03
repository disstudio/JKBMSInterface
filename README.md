# JK-BMS Interface Arduino Library

An Arduino library for communicating with JK-BMS (Jikong Battery Management System) units via UART. This library provides easy access to battery data including cell voltages, current, temperature, and status information, **plus the ability to control charging and discharging**.

## Features

- **Easy Integration** - Simple API with clean getter methods
- **Non-blocking** - Asynchronous data processing 
- **Complete Data Access** - Cell voltages, current, SOC, temperatures, status
- **MOS Control** - Enable/disable charging and discharging
- **Error Handling** - Safe defaults when data is invalid
- **Debug Support** - Built-in summary and raw data printing
- **Protocol Compliant** - Follows JK-BMS communication protocol v2.5

## Supported BMS Models

This library has been tested with a JK-BMS model that use the standard UART protocol: JK-BD4A8S4P using the 4-pin JST GH 1.25mm "GPS" port. The library should work with any other UART-enabled JK-BMS modules.

**IT IS IMPERATIVE YOU CHECK THE GPS PORT WIRING. ONE OF THE PINS HAS ALMOST THE FULL BATTERY VOLTAGE NEXT TO THE RX / TX / GND UART PINS. THIS CAN AND WILL DAMAGE YOUR ARDUINO IF MISWIRED**

## Hardware Requirements

- ESP32, Arduino, or compatible microcontroller with UART capability (typically pins 16/17 on ESP32)
- JK-BMS with UART communication output

## Installation

### Method 1: Manual Installation
1. Download this repository
2. Extract to your Arduino libraries folder: `Documents/Arduino/libraries/JKBMSInterface/`
3. Restart Arduino IDE

## Quick Start

```cpp
#include "JKBMSInterface.h"

// Create BMS instance using Serial2
JKBMSInterface bms(&Serial2);

void setup() {
    Serial.begin(115200);
    
    // Initialize BMS communication (RX=16, TX=17 for ESP32 Serial2 Port)
    Serial2.begin(115200, SERIAL_8N1, 16, 17);
    bms.begin(115200);
    
    Serial.println("JK-BMS Example Started");
}

void loop() {
    // Update BMS data (call this regularly)
    bms.update();
    
    // Check if we have valid data
    if (bms.isDataValid()) {
        // Access battery data
        float voltage = bms.getVoltage();
        uint8_t soc = bms.getSOC();
        float current = bms.getCurrent();
        
        Serial.print("Voltage: ");
        Serial.print(voltage, 2);
        Serial.print("V, SOC: ");
        Serial.print(soc);
        Serial.print("%, Current: ");
        Serial.print(current, 2);
        Serial.println("A");
    }
    
    delay(1000);
}
```

## API Reference

### Initialization

```cpp
JKBMSInterface bms(&Serial2);           // Create instance with serial port
bms.begin(115200);             // Initialize communication
bms.update();                  // Update data (call in loop)
```

### Basic Data

| Method | Return | Description |
|--------|--------|-------------|
| `getVoltage()` | `float` | Total pack voltage (V) |
| `getCurrent()` | `float` | Current (A, positive=discharge, negative=charge) |
| `getSOC()` | `uint8_t` | State of charge (0-100%) |
| `getCycles()` | `uint16_t` | Charge cycle count |

### Temperature Data

| Method | Return | Description |
|--------|--------|-------------|
| `getPowerTemp()` | `float` | Power tube temperature (°C) |
| `getBoxTemp()` | `float` | BMS box temperature (°C) |
| `getBatteryTemp()` | `float` | Battery temperature (°C) |

### Cell Voltage Data

| Method | Return | Description |
|--------|--------|-------------|
| `getNumCells()` | `uint8_t` | Number of active cells |
| `getCellVoltage(index)` | `float` | Individual cell voltage (V) |
| `getLowestCellVoltage()` | `float` | Lowest cell voltage (V) |
| `getHighestCellVoltage()` | `float` | Highest cell voltage (V) |
| `getCellVoltageDelta()` | `float` | Voltage difference between highest and lowest cell (V) |

### Status Information

| Method | Return | Description |
|--------|--------|-------------|
| `isChargingEnabled()` | `bool` | Charging MOS tube enabled |
| `isDischargingEnabled()` | `bool` | Discharging MOS tube enabled |
| `isCharging()` | `bool` | Currently charging (current < -0.01A) |
| `isDischarging()` | `bool` | Currently discharging (current > 0.01A) |
| `getAlarmStatus()` | `uint16_t` | Raw alarm status bits |
| `getStatusInfo()` | `uint16_t` | Raw status information |

### MOS Control Functions

| Method | Return | Description |
|--------|--------|-------------|
| `setChargeMOS(enable)` | `bool` | Enable/disable charge MOSFET (returns success) |
| `setDischargeMOS(enable)` | `bool` | Enable/disable discharge MOSFET (returns success) |
| `enableChargingOnly()` | `void` | Enable charging, disable discharging |
| `enableDischargingOnly()` | `void` | Enable discharging, disable charging |

### Device Information

| Method | Return | Description |
|--------|--------|-------------|
| `getSoftwareVersion()` | `String` | BMS firmware version |
| `getDeviceInfo()` | `String` | Device model information |
| `isDataValid()` | `bool` | Check if current data is valid |

### Debug Functions

| Method | Description |
|--------|-------------|
| `printSummary()` | Print formatted summary of all BMS data |
| `printRawData()` | Print raw hex data for debugging |

## Examples

The library includes three comprehensive examples:

### 1. BasicReading Example
Simple data reading without MOS control - perfect for monitoring applications.

### 2. MOSControl Example ⚡ **NEW**
Demonstrates all MOS control functions with safety checks:

```cpp
// Individual MOS control with error checking
bool enableCharging() {
    if (bms.setChargeMOS(true)) {
        Serial.println("✓ Charge MOS enabled");
        return true;
    } else {
        Serial.println("✗ Failed to enable charge MOS");
        return false;
    }
}
```

## Wiring

### ESP32 to JK-BMS
```
ESP32         JK-BMS
-----         ------
GPIO16 (TX) → RX (Serial2)
GPIO17 (RX) → TX (Serial2)
GND         → GND
```

### Arduino Mega to JK-BMS
```
Mega          JK-BMS
-----         ------
Pin 17 (TX) → RX (Serial2)
Pin 16 (RX) → TX (Serial2)
GND         → GND
```

## Safety Considerations ⚠️

**IMPORTANT**: The MOS control functions directly control battery charging and discharging. Use with extreme caution:

- Always verify commands are working by checking `isChargingEnabled()` and `isDischargingEnabled()`
- Implement safety timeouts for emergency situations
- Monitor temperature and voltage before enabling MOSFETs
- Test thoroughly in a safe environment before production use

## Troubleshooting

### No Data Received
- Check wiring connections **CRITICAL**
- Verify baud rate (115200)
- Ensure BMS UART is enabled
- Check if BMS is powered on

### MOS Control Not Working ⚡ **NEW**
- Verify BMS supports write commands
- Check if BMS is in a locked state
- Ensure proper wiring (especially ground)
- Some BMS units may require specific unlock sequences

### Invalid Data
- Verify protocol compatibility
- Check for electromagnetic interference
- Ensure stable power supply
- Try different update intervals

### Compilation Errors
- Ensure library is in correct folder
- Restart Arduino IDE after installation
- Check that all required files are present

## Protocol Information

This library implements the JK-BMS UART communication protocol v2.5. Key features:

- **Baud Rate**: 115200
- **Data Format**: 8N1
- **Frame Format**: Custom JK protocol with checksums
- **Update Rate**: Configurable (default 5 seconds)
- **Endianness**: Big-endian for multi-byte values
- **MOS Control**: Write commands using data IDs 0xAB (charge) and 0xAC (discharge)

## File Structure

```
JKBMSInterface/
├── JKBMSInterface.h                    # Main header file
├── JKBMSInterface.cpp                  # Implementation file
├── examples/
│   ├── BasicReading/
│   │   └── BasicReading.ino   # Simple data reading
│   ├── BMSControl/
│   │   └── BMSControl.ino     # MOS control functions
├── README.md                  # This file
└── LICENSE                    # MIT License
```

## Contributing

Contributions are welcome! Please feel free to submit issues, feature requests, or pull requests.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

```
MIT License - you are free to use, modify, and distribute this software
for any purpose, including commercial applications.
```

## Acknowledgments

- JK-BMS protocol documentation
- Arduino and ESP32 communities
- Contributors and testers

---

**Note**: This library is not officially affiliated with JK-BMS. Use at your own risk and **always verify battery data with official tools** when safety is critical. The MOS control features should be thoroughly tested before use in production environments.