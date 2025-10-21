#include "wifi_manager.h"

// WiFiManager 使用示例

WiFiManager wifiManager;
bool wifiConnected = false;

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 離線地圖 - WiFi Manager 示例");
    
    // 設置設備名稱
    wifiManager.setDeviceName("ESP32-OffLineMap-Demo");
    
    // 設置回調函數
    wifiManager.onConnected = []() {
        Serial.println("✅ WiFi 連接成功回調");
        wifiConnected = true;
        
        // 在這裡添加 WiFi 連接成功後的初始化代碼
        // 例如：啟動 OTA 更新檢查、連接到服務器等
        Serial.printf("IP 地址: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("連接到: %s\n", WiFi.SSID().c_str());
    };
    
    wifiManager.onDisconnected = []() {
        Serial.println("❌ WiFi 連接斷開回調");
        wifiConnected = false;
        
        // 在這裡添加 WiFi 斷開連接後的處理代碼
    };
    
    wifiManager.onPortalStart = []() {
        Serial.println("🌐 配置入口啟動回調");
        
        // 在這裡添加配置入口啟動時的代碼
        // 例如：顯示 LED 指示燈、LCD 顯示信息等
    };
    
    wifiManager.onPortalStop = []() {
        Serial.println("🛑 配置入口關閉回調");
        
        // 在這裡添加配置入口關閉時的代碼
    };
    
    // 啟動 WiFi Manager
    wifiManager.begin();
}

void loop() {
    // WiFi Manager 主循環
    wifiManager.loop();
    
    // 你的主程式邏輯
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 10000) { // 每 10 秒檢查一次
        lastCheck = millis();
        
        Serial.printf("狀態檢查 - WiFi: %s, 入口: %s\n", 
                     wifiManager.isConnected() ? "已連接" : "未連接",
                     wifiManager.isPortalActive() ? "活躍" : "關閉");
        
        if (wifiManager.isConnected()) {
            Serial.printf("IP: %s, SSID: %s\n", 
                         wifiManager.getIP().c_str(),
                         wifiManager.getSSID().c_str());
        }
    }
    
    // 添加你的其他應用邏輯
    // ...
}

// 可選：添加按鈕重置功能
void handleButtonPress() {
    // 如果檢測到按鈕長按（例如 5 秒）
    static unsigned long buttonPressTime = 0;
    static bool buttonPressed = false;
    
    // 假設 GPIO 0 是重置按鈕
    if (digitalRead(0) == LOW) {
        if (!buttonPressed) {
            buttonPressed = true;
            buttonPressTime = millis();
        } else if (millis() - buttonPressTime > 5000) {
            // 長按 5 秒，重置 WiFi 設定
            Serial.println("🔄 重置 WiFi 設定...");
            wifiManager.resetSettings();
        }
    } else {
        buttonPressed = false;
    }
}