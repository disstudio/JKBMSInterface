#include <JKBMSInterface.h>

// Create BMS instance using Serial2
JKBMSInterface bms(&Serial2);

void setup() {
    Serial.begin(115200);
    
    // Initialize BMS communication (RX=16, TX=17 for ESP32 Serial2 Port)
    Serial2.begin(115200, SERIAL_8N1, 16, 17);
    bms.begin(115200);
    
    Serial.println("JK-BMS Basic Reading Example");
    Serial.println("============================");
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
        
        // Print detailed summary every 10 seconds
        static unsigned long lastSummary = 0;
        if (millis() - lastSummary > 10000) {
            bms.printSummary();
            lastSummary = millis();
        }
    } else {
        Serial.println("Waiting for BMS data...");
    }
    
    delay(1000);
}