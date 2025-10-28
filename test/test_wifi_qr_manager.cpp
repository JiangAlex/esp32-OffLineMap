#ifdef WIFI_QR_TEST

#include <Arduino.h>
#include "App/Utils/WiFiManager/wifi_qr_manager.h"
#include "ChappieCore/ChappieCore.h"

ChappieCore Chappie;
WiFiQRManager wifiQRManager;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=================================");
    Serial.println("WiFi QR Manager Test Starting...");
    Serial.println("=================================");
    
    // 初始化 ChappieCore（包含顯示器和 LVGL）
    Chappie.begin();
    
    // 啟動 WiFi QR Manager
    // AP名稱: "ESP32-QR-Setup", 密碼: "12345678"
    //wifiQRManager.begin("ESP32-QR-Setup", "12345678");
    wifiQRManager.begin("ESP32-QR-Setup");
    
    Serial.println("WiFi QR Manager initialized!");
    Serial.println("QR Code should be displayed on screen");
    Serial.println("Scan QR Code with phone to connect to WiFi setup");
    Serial.println("Or manually connect to AP: ESP32-QR-Setup");
    Serial.println("Password: 12345678");
    Serial.println("Then open browser and go to: http://192.168.4.1");
    Serial.println("=================================");
}

void loop() {
    // 運行 WiFi QR Manager 主循環
    wifiQRManager.loop();
    
    // 運行 LVGL 任務處理（用於顯示 QR Code）
    lv_timer_handler();
    
    // 每10秒顯示一次詳細狀態
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 10000) {
        lastStatus = millis();
        
        Serial.println("\n=== WiFi Status Report ===");
        Serial.print("WiFi Mode: ");
        Serial.println(wifiQRManager.isConnected() ? "Station Connected" : "Config Mode");
        
        if (wifiQRManager.isConnected()) {
            Serial.printf("Connected to: %s\n", WiFi.SSID().c_str());
            Serial.printf("Local IP: %s\n", wifiQRManager.getLocalIP().c_str());
            Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
            Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
            Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());
            Serial.println("📱 QR Code on screen shows: http://" + wifiQRManager.getLocalIP());
        } else {
            Serial.printf("AP Mode IP: %s\n", WiFi.softAPIP().toString().c_str());
            Serial.printf("Connected clients: %d\n", WiFi.softAPgetStationNum());
            Serial.println("📱 QR Code on screen shows: http://192.168.4.1");
        }
        
        Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
        Serial.println("========================\n");
    }
    
    // 檢查連接狀態變化
    static bool wasConnected = false;
    bool isConnected = wifiQRManager.isConnected();
    
    if (isConnected != wasConnected) {
        wasConnected = isConnected;
        if (isConnected) {
            Serial.println("🎉 WiFi connected! QR Code updated to show web interface URL");
        } else {
            Serial.println("📡 WiFi disconnected! QR Code showing config portal URL");
        }
    }
    
    delay(50);
}

#endif // WIFI_QR_TEST