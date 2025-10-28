#ifdef OTA_VERSION_TEST

#include <Arduino.h>
#include <WiFi.h>
#include "App/Utils/OTA/ota_updater.h"

// 測試用的 WiFi 憑證
const char* test_ssid = "YOUR_WIFI_SSID";
const char* test_password = "YOUR_WIFI_PASSWORD";

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=================================");
    Serial.println("OTA Version Check Test Starting...");
    Serial.println("=================================");
    
    // 連接 WiFi
    Serial.printf("Connecting to WiFi: %s\n", test_ssid);
    WiFi.begin(test_ssid, test_password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        
        // 初始化 OTA 更新器
        String currentVersion = "1.0.1"; // 測試版本
        String serverURL = "https://jiangalex.github.io/esp32-OffLineMap/firmware/firmware-bin.bin";
        String versionURL = "https://jiangalex.github.io/esp32-OffLineMap/firmware/latest_version.txt";
        unsigned long checkInterval = 60; // 60 seconds for testing
        
        otaUpdater.begin(currentVersion, serverURL, versionURL, checkInterval);
        
        Serial.println("OTA Updater initialized!");
        Serial.println("Testing version check...");
        
        // 測試版本檢查
        if (otaUpdater.checkForUpdates()) {
            Serial.println("✅ Update available!");
            Serial.println("You can call otaUpdater.performUpdate() to install it.");
        } else {
            Serial.println("✅ No updates available or version check successful.");
        }
        
    } else {
        Serial.println("\n❌ WiFi connection failed!");
        Serial.println("Please check your WiFi credentials.");
    }
}

void loop() {
    // 每60秒測試一次版本檢查
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 60000) {
        lastCheck = millis();
        
        Serial.println("\n--- Periodic Version Check ---");
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Testing OTA version check...");
            
            bool updateAvailable = otaUpdater.checkForUpdates();
            
            Serial.printf("Update available: %s\n", updateAvailable ? "Yes" : "No");
            Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
            Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
        } else {
            Serial.println("WiFi disconnected, reconnecting...");
            WiFi.begin(test_ssid, test_password);
        }
        
        Serial.println("--- End Check ---\n");
    }
    
    delay(1000);
}

#endif // OTA_VERSION_TEST