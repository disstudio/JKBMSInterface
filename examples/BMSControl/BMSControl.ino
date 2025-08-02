#include <JKBMSInterface.h>

// Create BMS instance using Serial2
JKBMSInterface bms(&Serial2);

void setup() {
    Serial.begin(115200);
    
    // Initialize BMS communication (RX=16, TX=17 for ESP32 Serial2 Port)
    Serial2.begin(115200, SERIAL_8N1, 16, 17);
    bms.begin(115200);
    
    Serial.println("JK-BMS MOS Control Example");
    Serial.println("==========================");
    
    delay(2000);
}

void loop() {
    // Update BMS data regularly
    bms.update();
    
    // Display current status every 5 seconds
    static unsigned long lastStatusUpdate = 0;
    if (millis() - lastStatusUpdate > 5000) {
        displayBMSStatus();
        lastStatusUpdate = millis();
    }
    
    delay(100);
}

void displayBMSStatus() {
    if (!bms.isDataValid()) {
        Serial.println("Waiting for BMS data...");
        return;
    }
    
    Serial.println("\n--- BMS Status ---");
    Serial.printf("Voltage: %.2fV  SOC: %d%%  Current: %.2fA\n", 
                  bms.getVoltage(), bms.getSOC(), bms.getCurrent());
    Serial.printf("Charge MOS: %s  Discharge MOS: %s\n",
                  bms.isChargingEnabled() ? "ON" : "OFF",
                  bms.isDischargingEnabled() ? "ON" : "OFF");
    Serial.printf("Battery Temp: %.1f°C  Cell Delta: %.0fmV\n",
                  bms.getBatteryTemp(), bms.getCellVoltageDelta() * 1000);
}


// Function: Individual MOS control with error checking
bool enableCharging() {
    Serial.println("Enabling charging...");
    if (bms.setChargeMOS(true)) {
        Serial.println("✓ Charge MOS enabled");
        return true;
    } else {
        Serial.println("✗ Failed to enable charge MOS");
        return false;
    }
}

bool disableCharging() {
    Serial.println("Disabling charging...");
    if (bms.setChargeMOS(false)) {
        Serial.println("✓ Charge MOS disabled");
        return true;
    } else {
        Serial.println("✗ Failed to disable charge MOS");
        return false;
    }
}

bool enableDischarging() {
    Serial.println("Enabling discharging...");
    if (bms.setDischargeMOS(true)) {
        Serial.println("✓ Discharge MOS enabled");
        return true;
    } else {
        Serial.println("✗ Failed to enable discharge MOS");
        return false;
    }
}

bool disableDischarging() {
    Serial.println("Disabling discharging...");
    if (bms.setDischargeMOS(false)) {
        Serial.println("✓ Discharge MOS disabled");
        return true;
    } else {
        Serial.println("✗ Failed to disable discharge MOS");
        return false;
    }
}