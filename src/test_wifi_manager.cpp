#include <Arduino.h>
#include "App/Utils/WiFiManager/wifi_manager.h"

WiFiManager wifiManager;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=================================");
    Serial.println("WiFi Manager Test Starting...");
    Serial.println("=================================");
    
    // 啟動 WiFi Manager
    // AP名稱: "ESP32-OffLineMap-Setup", 密碼: "12345678"
    wifiManager.begin("ESP32-OffLineMap-Setup", "12345678");
    
    Serial.println("WiFi Manager initialized!");
    Serial.println("Please connect to AP: ESP32-OffLineMap-Setup");
    Serial.println("Password: 12345678");
    Serial.println("Then open browser and go to: http://192.168.4.1");
    Serial.println("=================================");
}

void loop() {
    // 運行 WiFi Manager 主循環
    wifiManager.loop();
    
    // 每5秒顯示一次狀態
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 5000) {
        lastStatus = millis();
        
        Serial.print("WiFi Status: ");
        switch (WiFi.status()) {
            case WL_CONNECTED:
                Serial.printf("Connected to %s, IP: %s\n", 
                    WiFi.SSID().c_str(), 
                    WiFi.localIP().toString().c_str());
                break;
            case WL_NO_SSID_AVAIL:
                Serial.println("SSID not available");
                break;
            case WL_CONNECT_FAILED:
                Serial.println("Connection failed");
                break;
            case WL_CONNECTION_LOST:
                Serial.println("Connection lost");
                break;
            case WL_DISCONNECTED:
                Serial.println("Disconnected");
                break;
            default:
                Serial.printf("Unknown status: %d\n", WiFi.status());
                break;
        }
        
        Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
        Serial.println("-------------------------");
    }
    
    delay(100);
}