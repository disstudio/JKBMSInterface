#include "JKBMSInterface.h"

// Working command from protocol documentation
static const uint8_t readAllCommand[] = {
  0x4E, 0x57, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 
  0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x68, 0x00, 0x00, 0x01, 0x29
};

JKBMSInterface::JKBMSInterface(HardwareSerial* serial) : _serial(serial), _responseIndex(0), _lastCommandSent(0) {
    clearData();
}

void JKBMSInterface::begin(uint32_t baudRate) {
    _serial->begin(baudRate, SERIAL_8N1);
    clearData();
    
    // Clear any existing data in buffer
    while (_serial->available()) {
        _serial->read();
    }
}

void JKBMSInterface::clearData() {
    _bmsData.dataValid = false;
    _bmsData.numCells = 0;
    _bmsData.totalVoltage = 0;
    _bmsData.current = 0;
    _bmsData.soc = 0;
    _bmsData.powerTemp = 0;
    _bmsData.boxTemp = 0;
    _bmsData.batteryTemp = 0;
    _bmsData.alarmStatus = 0;
    _bmsData.statusInfo = 0;
    _bmsData.cycles = 0;
    _bmsData.softwareVersion = "";
    _bmsData.deviceInfo = "";
    _bmsData.protocolVersion = 1; // default protocol version
    
    for (int i = 0; i < 24; i++) {
        _bmsData.cellVoltages[i] = 0;
    }
}

void JKBMSInterface::update() {
    // Send command every 5 seconds
    if (millis() - _lastCommandSent > 5000) {
        requestData();
    }
    
    // Process incoming data
    while (_serial->available()) {
        uint8_t byte = _serial->read();
        _responseBuffer[_responseIndex++] = byte;
        
        // Look for end of frame (checksum pattern)
        if (_responseIndex >= 4 && 
            _responseBuffer[_responseIndex-4] == 0x68 && // End marker
            _responseBuffer[_responseIndex-3] == 0x00 && // Checksum start
            _responseBuffer[_responseIndex-2] == 0x00) {
            
            // Parse the complete frame
            parseRawData(_responseBuffer, _responseIndex);
            _responseIndex = 0;
        }
        
        // Prevent buffer overflow
        if (_responseIndex >= 512) {
            _responseIndex = 0;
        }
    }
}

void JKBMSInterface::requestData() {
    _serial->write(readAllCommand, sizeof(readAllCommand));
    _lastCommandSent = millis();
    _responseIndex = 0;
}

void JKBMSInterface::parseRawData(uint8_t* data, int length) {
    _bmsData.dataValid = false;
    _bmsData.numCells = 0;
    _bmsData.protocolVersion = 1; // default protocol version
    
    // Look for start of actual data (after header)
    int pos = 11; // Skip header: 4E 57 01 09 00 00 00 00 06 00 01
    
    uint16_t current = 0xFFFF;
    
    while (pos < length - 4) { // Leave room for checksum
        if (pos >= length) break;
        
        uint8_t dataId = data[pos++];
        
        switch (dataId) {
            case 0x79: // Cell voltages
                {
                    if (pos < length) {
                        uint8_t dataLength = data[pos++];
                        int endPos = pos + dataLength;
                        _bmsData.numCells = 0;
                        
                        // Each cell entry is 3 bytes: cell_number(1) + voltage(2)
                        while (pos + 3 <= endPos && pos + 3 <= length) {
                            uint8_t cellNum = data[pos++];
                            uint16_t voltage = (data[pos] << 8) | data[pos+1]; // Big endian
                            pos += 2;
                            
                            if (cellNum > 0 && cellNum <= 24 && voltage > 0) {
                                _bmsData.cellVoltages[cellNum-1] = voltage / 1000.0f;
                                _bmsData.numCells = max(_bmsData.numCells, cellNum);
                            }
                        }
                    }
                }
                break;
                
            case 0x80: // Power tube temperature  
                if (pos + 1 < length) {
                    uint16_t temp = (data[pos] << 8) | data[pos+1]; // Big endian
                    _bmsData.powerTemp = (temp <= 100) ? temp : -(temp - 100);
                    pos += 2;
                }
                break;
                
            case 0x81: // Box temperature
                if (pos + 1 < length) {
                    uint16_t temp = (data[pos] << 8) | data[pos+1]; // Big endian
                    _bmsData.boxTemp = (temp <= 100) ? temp : -(temp - 100);
                    pos += 2;
                }
                break;
                
            case 0x82: // Battery temperature
                if (pos + 1 < length) {
                    uint16_t temp = (data[pos] << 8) | data[pos+1]; // Big endian
                    _bmsData.batteryTemp = (temp <= 100) ? temp : -(temp - 100);
                    pos += 2;
                }
                break;
                
            case 0x83: // Total voltage
                if (pos + 1 < length) {
                    uint16_t voltage = (data[pos] << 8) | data[pos+1]; // Big endian
                    _bmsData.totalVoltage = voltage * 0.01f;
                    pos += 2;
                }
                break;
                
            case 0x84: // Current
                if (pos + 1 < length) {
                    current = (data[pos] << 8) | data[pos+1]; // Big endian
                    pos += 2;
                }
                break;
                
            case 0x85: // SOC
                if (pos < length) {
                    _bmsData.soc = data[pos];
                    pos += 1;
                }
                break;
                
            case 0x86: // Temperature sensor count
                if (pos < length) {
                    pos += 1; // Skip this data
                }
                break;
                
            case 0x87: // Cycles
                if (pos + 1 < length) {
                    _bmsData.cycles = (data[pos] << 8) | data[pos+1]; // Big endian
                    pos += 2;
                }
                break;
                
            case 0x8B: // Alarm status
                if (pos + 1 < length) {
                    _bmsData.alarmStatus = (data[pos] << 8) | data[pos+1]; // Big endian
                    pos += 2;
                }
                break;
                
            case 0x8C: // Status info
                if (pos + 1 < length) {
                    _bmsData.statusInfo = (data[pos] << 8) | data[pos+1]; // Big endian
                    pos += 2;
                }
                break;
                
            case 0xB7: // Software version
                {
                    String version = "";
                    for (int i = 0; i < 15 && pos < length; i++) {
                        if (data[pos] >= 0x20 && data[pos] <= 0x7E) {
                            version += (char)data[pos];
                        }
                        pos++;
                    }
                    _bmsData.softwareVersion = version;
                }
                break;
                
            case 0xB4: // Device info part 1
                {
                    String info = "";
                    while (pos < length && data[pos] >= 0x20 && data[pos] <= 0x7E) {
                        info += (char)data[pos];
                        pos++;
                    }
                    if (pos < length && data[pos] == 0x00) pos++; // Skip null
                }
                break;
                
            case 0xBA: // Device info part 2
                {
                    String fullName = "";
                    while (pos < length && data[pos] >= 0x20 && data[pos] <= 0x7E) {
                        fullName += (char)data[pos];
                        pos++;
                    }
                    _bmsData.deviceInfo = fullName;
                    if (pos < length && data[pos] == 0x00) pos++; // Skip null
                }
                break;
                
            case 0x68: // End marker found
                _bmsData.dataValid = true;
                break;
                
            case 0xC0: // Protocol version number
                if (pos + 1 < length) {
                    _bmsData.protocolVersion = (data[pos] << 8) | data[pos+1]; // Big endian
                    pos += 2;
                }
                break;
                
            default:
                // Skip unknown data types
                if (dataId >= 0x8E && dataId <= 0xC0) {
                    pos += 2;
                } else {
                    pos += 1;
                }
                break;
        }
        
        if (_bmsData.dataValid) {
            break;
        }
    }
    
    if (current != 0xFFFF) {
        // Parse raw current value dependent to protocol version
        switch (_bmsData.protocolVersion) {
            case 0:
                if (current == 0) {
                    _bmsData.current = 0.0f;
                } else if (current == 10000) {
                    _bmsData.current = 0.0f;
                } else if (current > 10000) {
                    _bmsData.current = (current - 10000) * 0.01f; // Discharge (positive)
                } else if (current < 10000 && current > 0) {
                    _bmsData.current = -(10000 - current) * 0.01f; // Charge (negative)  
                } else {
                    _bmsData.current = 0.0f;
                }
                break;
            
            case 1:
                if ((current & 0x8000) == 0) {
                    // Discharge
                    _bmsData.current = current * 0.01f;
                } else {
                    // Charge
                    _bmsData.current = -(current & 0x07FF) * 0.01f;
                }
        }
    }
    
    _bmsData.dataValid = true;
}

// Public getter methods
float JKBMSInterface::getVoltage() {
    return _bmsData.dataValid ? _bmsData.totalVoltage : -1.0f;
}

float JKBMSInterface::getCurrent() {
    return _bmsData.dataValid ? _bmsData.current : 0.0f;
}

uint8_t JKBMSInterface::getSOC() {
    return _bmsData.dataValid ? _bmsData.soc : 0;
}

float JKBMSInterface::getPowerTemp() {
    return _bmsData.dataValid ? _bmsData.powerTemp : -999.0f;
}

float JKBMSInterface::getBoxTemp() {
    return _bmsData.dataValid ? _bmsData.boxTemp : -999.0f;
}

float JKBMSInterface::getBatteryTemp() {
    return _bmsData.dataValid ? _bmsData.batteryTemp : -999.0f;
}

uint16_t JKBMSInterface::getCycles() {
    return _bmsData.dataValid ? _bmsData.cycles : 0;
}

uint8_t JKBMSInterface::getNumCells() {
    return _bmsData.dataValid ? _bmsData.numCells : 0;
}

float JKBMSInterface::getCellVoltage(uint8_t cellIndex) {
    if (!_bmsData.dataValid || cellIndex >= 24 || cellIndex >= _bmsData.numCells) {
        return -1.0f;
    }
    return _bmsData.cellVoltages[cellIndex];
}

float JKBMSInterface::getLowestCellVoltage() {
    if (!_bmsData.dataValid || _bmsData.numCells == 0) return -1.0f;
    
    float lowest = _bmsData.cellVoltages[0];
    for (int i = 1; i < _bmsData.numCells; i++) {
        if (_bmsData.cellVoltages[i] > 0 && _bmsData.cellVoltages[i] < lowest) {
            lowest = _bmsData.cellVoltages[i];
        }
    }
    return lowest;
}

float JKBMSInterface::getHighestCellVoltage() {
    if (!_bmsData.dataValid || _bmsData.numCells == 0) return -1.0f;
    
    float highest = _bmsData.cellVoltages[0];
    for (int i = 1; i < _bmsData.numCells; i++) {
        if (_bmsData.cellVoltages[i] > highest) {
            highest = _bmsData.cellVoltages[i];
        }
    }
    return highest;
}

float JKBMSInterface::getCellVoltageDelta() {
    if (!_bmsData.dataValid || _bmsData.numCells == 0) return -1.0f;
    return getHighestCellVoltage() - getLowestCellVoltage();
}

uint16_t JKBMSInterface::getAlarmStatus() {
    return _bmsData.dataValid ? _bmsData.alarmStatus : 0;
}

uint16_t JKBMSInterface::getStatusInfo() {
    return _bmsData.dataValid ? _bmsData.statusInfo : 0;
}

bool JKBMSInterface::isChargingEnabled() {
    return _bmsData.dataValid ? (_bmsData.statusInfo & 0x01) != 0 : false;
}

bool JKBMSInterface::isDischargingEnabled() {
    return _bmsData.dataValid ? (_bmsData.statusInfo & 0x02) != 0 : false;
}

bool JKBMSInterface::isCharging() {
    return _bmsData.dataValid ? _bmsData.current < -0.01f : false;
}

bool JKBMSInterface::isDischarging() {
    return _bmsData.dataValid ? _bmsData.current > 0.01f : false;
}

String JKBMSInterface::getSoftwareVersion() {
    return _bmsData.dataValid ? _bmsData.softwareVersion : "Unknown";
}

String JKBMSInterface::getDeviceInfo() {
    return _bmsData.dataValid ? _bmsData.deviceInfo : "Unknown";
}

uint16_t JKBMSInterface::getProtocolVersion() {
    return _bmsData.dataValid ? _bmsData.protocolVersion : 0;
}

bool JKBMSInterface::isDataValid() {
    return _bmsData.dataValid;
}

// MOS Control Functions
uint16_t JKBMSInterface::calculateChecksum(uint8_t* data, int length) {
    uint16_t sum = 0;
    for (int i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

bool JKBMSInterface::sendMOSCommand(uint8_t dataId, bool enable) {
    uint8_t command[32];
    int pos = 0;
    
    // Start bytes
    command[pos++] = 0x4E;
    command[pos++] = 0x57;
    
    // Length (will be calculated)
    int lengthPos = pos;
    pos += 2; // Skip length for now
    
    // Terminal ID (4 bytes)
    command[pos++] = 0x00;
    command[pos++] = 0x00;
    command[pos++] = 0x00;
    command[pos++] = 0x00;
    
    // Command word (0x02 = write)
    command[pos++] = 0x02;
    
    // Frame source (3 = PC)
    command[pos++] = 0x03;
    
    // Transport type (0 = request)
    command[pos++] = 0x00;
    
    // Data identifier (0xAB = charge MOS, 0xAC = discharge MOS)
    command[pos++] = dataId;
    
    // Data payload (1 byte: 0x00 = OFF, 0x01 = ON)
    command[pos++] = enable ? 0x01 : 0x00;
    
    // Record number (4 bytes)
    command[pos++] = 0x00;
    command[pos++] = 0x00;
    command[pos++] = 0x00;
    command[pos++] = 0x01;
    
    // End identifier
    command[pos++] = 0x68;
    
    // Calculate and set length (total length including length field itself)
    uint16_t frameLength = pos + 4 - 2; // +4 for checksum, -2 for start bytes
    command[lengthPos] = (frameLength >> 8) & 0xFF;     // High byte
    command[lengthPos + 1] = frameLength & 0xFF;        // Low byte
    
    // Calculate checksum (sum of all bytes from start to end)
    uint16_t checksum = calculateChecksum(command, pos);
    
    // Add checksum (4 bytes: 2 bytes CRC16 not used + 2 bytes sum)
    command[pos++] = 0x00; // CRC16 high (not used)
    command[pos++] = 0x00; // CRC16 low (not used)
    command[pos++] = (checksum >> 8) & 0xFF; // Sum high byte
    command[pos++] = checksum & 0xFF;        // Sum low byte
    
    // Clear receive buffer before sending command
    while (_serial->available()) {
        _serial->read();
    }
    
    // Send command
    _serial->write(command, pos);
    
    // Wait for and verify response
    return waitForMOSResponse(3000);
}

bool JKBMSInterface::waitForMOSResponse(unsigned long timeoutMs) {
    unsigned long startTime = millis();
    uint8_t buffer[256];
    int bufferIndex = 0;
    
    while (millis() - startTime < timeoutMs) {
        if (_serial->available()) {
            uint8_t byte = _serial->read();
            buffer[bufferIndex++] = byte;
            
            // Look for valid frame start within the buffer
            for (int i = 0; i <= bufferIndex - 4; i++) {
                if (buffer[i] == 0x4E && buffer[i+1] == 0x57) {
                    // Found potential start, check if we have a complete frame
                    int frameStart = i;
                    int remainingBytes = bufferIndex - frameStart;
                    
                    if (remainingBytes >= 4) {
                        // Get frame length from bytes 2-3 after start
                        if (frameStart + 3 < bufferIndex) {
                            uint16_t frameLength = (buffer[frameStart + 2] << 8) | buffer[frameStart + 3];
                            int expectedTotalLength = frameLength + 2; // +2 for start bytes
                            
                            if (remainingBytes >= expectedTotalLength) {
                                // Verify this is a write response
                                if (expectedTotalLength >= 12 && 
                                    buffer[frameStart + 8] == 0x02 &&  // Command word (write)
                                    buffer[frameStart + 9] == 0x00 &&  // Frame source (BMS)
                                    buffer[frameStart + 10] == 0x01) { // Transport type (response)
                                    return true; // Command acknowledged
                                }
                            }
                        }
                    }
                }
            }
            
            // Prevent buffer overflow
            if (bufferIndex >= 256) {
                return false;
            }
        }
    }
    
    return false; // Timeout or no valid response
}

bool JKBMSInterface::setChargeMOS(bool enable) {
    return sendMOSCommand(0xAB, enable); // 0xAB = Charge MOS tube switch
}

bool JKBMSInterface::setDischargeMOS(bool enable) {
    return sendMOSCommand(0xAC, enable); // 0xAC = Discharge MOS tube switch
}

void JKBMSInterface::enableBatteryOperation() {
    setChargeMOS(true);
    delay(500);
    setDischargeMOS(true);
}

void JKBMSInterface::disableBatteryOperation() {
    setChargeMOS(false);
    delay(500);
    setDischargeMOS(false);
}

void JKBMSInterface::enableChargingOnly() {
    setDischargeMOS(false);
    delay(500);
    setChargeMOS(true);
}

void JKBMSInterface::enableDischargingOnly() {
    setChargeMOS(false);
    delay(500);
    setDischargeMOS(true);
}

void JKBMSInterface::printSummary() {
    if (!_bmsData.dataValid) {
        Serial.println("No valid BMS data available");
        return;
    }
    
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║            BMS SUMMARY                ║");
    Serial.println("╠═══════════════════════════════════════╣");
    
    Serial.print("║ Total Voltage: ");
    Serial.print(_bmsData.totalVoltage, 2);
    Serial.println("V                   ║");
    
    Serial.print("║ Current: ");
    if (_bmsData.current > 0.01f) {
        Serial.print(_bmsData.current, 2);
        Serial.println("A (Discharging)        ║");
    } else if (_bmsData.current < -0.01f) {
        Serial.print(-_bmsData.current, 2);
        Serial.println("A (Charging)           ║");
    } else {
        Serial.println("0.00A (Idle)               ║");
    }
    
    Serial.print("║ SOC: ");
    Serial.print(_bmsData.soc);
    Serial.println("%                            ║");
    
    Serial.print("║ Temperatures: Power=");
    Serial.print(_bmsData.powerTemp, 1);
    Serial.print("°C Battery=");
    Serial.print(_bmsData.batteryTemp, 1);
    Serial.println("°C ║");
    
    Serial.print("║ Charge Cycles: ");
    Serial.print(_bmsData.cycles);
    Serial.println("                   ║");
    
    Serial.print("║ MOS Status: Charge=");
    Serial.print((_bmsData.statusInfo & 0x01) ? "ON" : "OFF");
    Serial.print(" Discharge=");
    Serial.print((_bmsData.statusInfo & 0x02) ? "ON" : "OFF");
    Serial.println("    ║");
    
    if (_bmsData.numCells > 0) {
        Serial.println("║ Cell Voltages:                        ║");
        for (int i = 0; i < _bmsData.numCells; i++) {
            if (_bmsData.cellVoltages[i] > 0) {
                Serial.print("║   Cell ");
                Serial.print(i+1);
                Serial.print(": ");
                Serial.print(_bmsData.cellVoltages[i], 3);
                Serial.println("V                      ║");
            }
        }
    }
    
    if (_bmsData.softwareVersion.length() > 0) {
        Serial.print("║ Software: ");
        Serial.print(_bmsData.softwareVersion);
        Serial.println("           ║");
    }
    
    
    Serial.print("║ Protocol version: ");
    Serial.print(_bmsData.protocolVersion);
    Serial.println("                   ║");
    
    Serial.println("╚═══════════════════════════════════════╝");
}

void JKBMSInterface::printRawData() {
    Serial.print("Raw data (");
    Serial.print(_responseIndex);
    Serial.print(" bytes): ");
    for (int i = 0; i < min(_responseIndex, 80); i++) {
        if (_responseBuffer[i] < 16) Serial.print("0");
        Serial.print(_responseBuffer[i], HEX);
        Serial.print(" ");
    }
    if (_responseIndex > 80) Serial.print("...");
    Serial.println();
}