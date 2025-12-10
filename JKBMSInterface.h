#ifndef JKBMSInterface_H
#define JKBMSInterface_H

#include <Arduino.h>

class JKBMSInterface {
public:
    // Constructor
    JKBMSInterface(HardwareSerial* serial);
    
    // Initialize the BMS communication
    void begin(uint32_t baudRate = 115200);
    
    // Main update function - call this in your loop()
    void update();
    
    // Request data from BMS (automatically called by update())
    void requestData();
    
    // Basic data getters
    float getVoltage();
    float getCurrent();
    uint8_t getSOC();
    uint16_t getCycles();
    
    // Temperature getters
    float getPowerTemp();
    float getBoxTemp();
    float getBatteryTemp();
    
    // Cell voltage getters
    uint8_t getNumCells();
    float getCellVoltage(uint8_t cellIndex);
    float getLowestCellVoltage();
    float getHighestCellVoltage();
    float getCellVoltageDelta();
    
    // Status getters
    uint16_t getAlarmStatus();
    uint16_t getStatusInfo();
    bool isChargingEnabled();
    bool isDischargingEnabled();
    bool isCharging();
    bool isDischarging();
    
    // Info getters
    String getSoftwareVersion();
    String getDeviceInfo();
    uint16_t getProtocolVersion();
    
    // Data validity
    bool isDataValid();
    
    // MOS Control functions
    bool setChargeMOS(bool enable);
    bool setDischargeMOS(bool enable);
    void enableBatteryOperation();
    void disableBatteryOperation();
    void enableChargingOnly();
    void enableDischargingOnly();
    
    // Display functions
    void printSummary();
    void printRawData();

private:
    struct BMSData {
        float cellVoltages[24];
        uint8_t numCells;
        float totalVoltage;
        float current;
        uint8_t soc;
        float powerTemp;
        float boxTemp;
        float batteryTemp;
        uint16_t alarmStatus;
        uint16_t statusInfo;
        uint16_t cycles;
        String softwareVersion;
        String deviceInfo;
        bool dataValid;
        uint16_t protocolVersion;
    };
    
    HardwareSerial* _serial;
    BMSData _bmsData;
    uint8_t _responseBuffer[512];
    int _responseIndex;
    unsigned long _lastCommandSent;
    
    // Private methods
    void parseRawData(uint8_t* data, int length);
    void clearData();
    
    // MOS Control private methods
    uint16_t calculateChecksum(uint8_t* data, int length);
    bool sendMOSCommand(uint8_t dataId, bool enable);
    bool waitForMOSResponse(unsigned long timeoutMs = 3000);
};

#endif