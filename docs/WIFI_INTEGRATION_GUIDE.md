# WiFi Manager 整合指南

## 如何將 WiFi Manager 整合到 ESP32 離線地圖項目

### 1. 修改 main.cpp

在您的 `src/main.cpp` 文件中添加 WiFi Manager：

```cpp
#include <Arduino.h>
#include "App/Utils/WiFiManager/wifi_manager.h"

// 現有的 include...
#ifdef ENABLE_AUTO_OTA_CHECK
#include "App/Utils/OTA/ota_updater.h"
OTAUpdater otaUpdater;
#endif

// 添加 WiFi Manager
WiFiManager wifiManager;
bool isWiFiConnected = false;

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 離線地圖啟動中...");
    
    // 現有的初始化代碼...
    // HAL::Init();
    // App_Init();
    
    // 配置 WiFi Manager
    wifiManager.setDeviceName("ESP32-OffLineMap");
    
    // 設置 WiFi 連接成功回調
    wifiManager.onConnected = []() {
        Serial.println("✅ WiFi 連接成功！");
        Serial.printf("IP 地址: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("連接到: %s\n", WiFi.SSID().c_str());
        
        isWiFiConnected = true;
        
        #ifdef ENABLE_AUTO_OTA_CHECK
        // WiFi 連接成功後啟動 OTA 檢查
        String currentVersion = "1.0.6"; // 從版本管理獲取
        String serverURL = OTA_SERVER_URL;
        String versionURL = OTA_VERSION_URL;
        unsigned long interval = OTA_CHECK_INTERVAL;
        
        otaUpdater.begin(currentVersion, serverURL, versionURL, interval);
        Serial.println("OTA 更新檢查已啟動");
        #endif
    };
    
    // 設置 WiFi 斷開連接回調
    wifiManager.onDisconnected = []() {
        Serial.println("❌ WiFi 連接斷開");
        isWiFiConnected = false;
    };
    
    // 設置配置入口啟動回調
    wifiManager.onPortalStart = []() {
        Serial.println("🌐 WiFi 配置入口已啟動");
        Serial.println("請連接到 'ESP32-OffLineMap-Setup' 熱點進行配置");
        
        // 在 LVGL 界面顯示配置提示（如果需要）
        // App_ShowWiFiConfigMessage();
    };
    
    // 設置配置入口關閉回調
    wifiManager.onPortalStop = []() {
        Serial.println("🛑 WiFi 配置入口已關閉");
        
        // 隱藏 LVGL 配置提示（如果需要）
        // App_HideWiFiConfigMessage();
    };
    
    // 啟動 WiFi Manager
    wifiManager.begin();
    
    Serial.println("ESP32 離線地圖初始化完成");
}

void loop() {
    // WiFi Manager 主循環（必須調用）
    wifiManager.loop();
    
    #ifdef ENABLE_AUTO_OTA_CHECK
    // OTA 更新檢查（如果 WiFi 已連接）
    if (isWiFiConnected) {
        otaUpdater.handleAutoCheck();
    }
    #endif
    
    // 現有的主循環代碼...
    // HAL::Loop();
    // App_Loop();
    
    // 可選：定期顯示 WiFi 狀態
    static unsigned long lastStatusCheck = 0;
    if (millis() - lastStatusCheck > 30000) { // 每 30 秒檢查一次
        lastStatusCheck = millis();
        
        if (wifiManager.isConnected()) {
            Serial.printf("WiFi 狀態: 已連接 | IP: %s | SSID: %s\n", 
                         wifiManager.getIP().c_str(),
                         wifiManager.getSSID().c_str());
        } else if (wifiManager.isPortalActive()) {
            Serial.println("WiFi 狀態: 配置入口活躍");
        } else {
            Serial.println("WiFi 狀態: 未連接");
        }
    }
}
```

### 2. 添加編譯標誌（可選）

在 `platformio.ini` 中添加 WiFi Manager 的編譯標誌：

```ini
; WiFi Manager Configuration
-D ENABLE_WIFI_MANAGER          # 啟用 WiFi Manager
-D WIFI_MANAGER_DEBUG=1         # 啟用調試輸出
-D WIFI_MANAGER_AP_SSID=\"ESP32-OffLineMap-Setup\"  # 自定義 AP 名稱
```

### 3. 與現有按鈕整合

如果您的設備有按鈕，可以添加 WiFi 重置功能：

```cpp
// 在 main.cpp 或相關的按鈕處理代碼中
void handleButtonEvents() {
    static unsigned long buttonPressTime = 0;
    static bool buttonPressed = false;
    
    // 假設使用 GPIO 0 作為重置按鈕
    bool currentButtonState = digitalRead(0) == LOW;
    
    if (currentButtonState && !buttonPressed) {
        // 按鈕剛被按下
        buttonPressed = true;
        buttonPressTime = millis();
        Serial.println("重置按鈕被按下...");
    } else if (!currentButtonState && buttonPressed) {
        // 按鈕被釋放
        buttonPressed = false;
        unsigned long pressDuration = millis() - buttonPressTime;
        
        if (pressDuration > 5000) {
            // 長按 5 秒，重置 WiFi 設定
            Serial.println("🔄 重置 WiFi 設定...");
            wifiManager.resetSettings();
        } else if (pressDuration > 1000) {
            // 短按 1 秒，重新啟動配置入口
            Serial.println("🌐 啟動 WiFi 配置入口...");
            wifiManager.startConfigPortal();
        }
    }
}

// 在主循環中調用
void loop() {
    wifiManager.loop();
    handleButtonEvents();
    // ... 其他代碼
}
```

### 4. 與 LVGL 界面整合

如果您使用 LVGL 界面，可以添加 WiFi 狀態顯示：

```cpp
// 在適當的 LVGL 頁面中添加 WiFi 狀態指示器
void App_UpdateWiFiStatus() {
    static lv_obj_t* wifi_label = nullptr;
    
    if (wifi_label == nullptr) {
        // 創建 WiFi 狀態標籤
        wifi_label = lv_label_create(lv_scr_act());
        lv_obj_align(wifi_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    }
    
    if (wifiManager.isConnected()) {
        lv_label_set_text(wifi_label, LV_SYMBOL_WIFI " 已連接");
        lv_obj_set_style_text_color(wifi_label, lv_color_hex(0x00FF00), 0);
    } else if (wifiManager.isPortalActive()) {
        lv_label_set_text(wifi_label, LV_SYMBOL_SETTINGS " 配置中");
        lv_obj_set_style_text_color(wifi_label, lv_color_hex(0xFFAA00), 0);
    } else {
        lv_label_set_text(wifi_label, LV_SYMBOL_CLOSE " 未連接");
        lv_obj_set_style_text_color(wifi_label, lv_color_hex(0xFF0000), 0);
    }
}

// 在 LVGL 主循環中定期更新
void App_Loop() {
    static unsigned long lastWiFiUpdate = 0;
    if (millis() - lastWiFiUpdate > 5000) { // 每 5 秒更新一次
        lastWiFiUpdate = millis();
        App_UpdateWiFiStatus();
    }
    
    // ... 其他 LVGL 代碼
}
```

### 5. 配置頁面自定義

您可以修改 `wifi_manager_html.cpp` 來自定義配置頁面：

```cpp
// 修改設備名稱和圖標
<div class="header">
    <h1>🗺️ ESP32 離線地圖</h1>
    <p>WiFi 網路配置</p>
</div>

// 添加項目特定的說明
<div class="status info">
    <strong>歡迎使用 ESP32 離線地圖！</strong><br>
    請配置 WiFi 連接以啟用在線功能：
    <ul>
        <li>📡 OTA 固件更新</li>
        <li>🌐 地圖數據同步</li>
        <li>📊 使用統計上傳</li>
    </ul>
</div>
```

### 6. 測試和驗證

1. **編譯項目**：
   ```bash
   pio run -e esp32-s3-devkitc-1
   ```

2. **上傳到設備**：
   ```bash
   pio run -e esp32-s3-devkitc-1 -t upload
   ```

3. **監控串口輸出**：
   ```bash
   pio device monitor
   ```

4. **測試 WiFi 配置**：
   - 設備啟動後應該看到配置入口信息
   - 使用手機連接到 "ESP32-OffLineMap-Setup"
   - 訪問配置頁面並設置 WiFi

5. **驗證功能**：
   - WiFi 連接成功後檢查 OTA 功能
   - 確認設備在網路中可見
   - 測試重置功能

### 7. 故障排除

#### 編譯錯誤
```bash
# 如果遇到 WebServer.h 找不到的錯誤
# 確保使用 ESP32 Arduino Core 2.0.0 或更高版本
```

#### 記憶體不足
```ini
# 在 platformio.ini 中調整分區表
board_build.partitions = custom_partitions.csv
```

#### WiFi 連接問題
- 檢查 2.4GHz 頻段支援
- 確認路由器設置
- 查看串口調試信息

---

## 🎉 完成！

現在您的 ESP32 離線地圖設備具備了智能 WiFi 配網功能！用戶可以通過簡潔美觀的 Web 界面輕鬆配置網路設置，而您的設備也能自動處理連接和重連邏輯。