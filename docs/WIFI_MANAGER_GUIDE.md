# ESP32 WiFi Manager 配網功能說明

## 概述

WiFi Manager 是一個用於 ESP32 設備的智能配網解決方案，提供了簡潔美觀的 Web Portal 界面，讓用戶可以輕鬆配置 WiFi 網路設置。

## 功能特點

### 🌐 **智能配網**
- 自動檢測已保存的 WiFi 配置並嘗試連接
- 連接失敗時自動啟動 AP 模式和配置入口
- 支援配置超時和自動重試機制

### 📱 **美觀的 Web Interface**
- 響應式設計，支援手機和電腦瀏覽器
- 現代化的 UI 設計，使用漸變背景和動畫效果
- 中文界面，操作簡單直觀

### 🔍 **WiFi 網路掃描**
- 實時掃描附近的 WiFi 網路
- 顯示信號強度和安全性狀態
- 點擊網路名稱快速填入 SSID

### 💾 **配置持久化**
- 使用 EEPROM 保存 WiFi 配置
- 設備重啟後自動載入已保存的設置
- 支援配置重置功能

### 🔧 **靈活的回調系統**
- WiFi 連接成功/失敗回調
- 配置入口啟動/關閉回調
- 方便整合到現有項目中

## 使用方法

### 1. 基本使用

```cpp
#include "wifi_manager.h"

WiFiManager wifiManager;

void setup() {
    Serial.begin(115200);
    
    // 設置設備名稱
    wifiManager.setDeviceName("我的ESP32設備");
    
    // 設置回調函數
    wifiManager.onConnected = []() {
        Serial.println("WiFi 連接成功！");
        // 添加連接成功後的邏輯
    };
    
    // 啟動 WiFi Manager
    wifiManager.begin();
}

void loop() {
    wifiManager.loop();
    // 你的主程式邏輯
}
```

### 2. 高級配置

```cpp
// 自定義 AP 設置
wifiManager.begin("MyDevice-Setup", "password123");

// 設置自動連接
wifiManager.setAutoConnect(true);

// 手動啟動配置入口
wifiManager.startConfigPortal();

// 重置 WiFi 設置
wifiManager.resetSettings();
```

### 3. 狀態檢查

```cpp
// 檢查 WiFi 連接狀態
if (wifiManager.isConnected()) {
    Serial.println("WiFi 已連接");
    Serial.println("IP: " + wifiManager.getIP());
    Serial.println("SSID: " + wifiManager.getSSID());
}

// 檢查配置入口狀態
if (wifiManager.isPortalActive()) {
    Serial.println("配置入口正在運行");
}
```

## 配置入口使用流程

### 步驟 1：設備啟動 AP 模式
- 設備無法連接到已保存的 WiFi 時自動啟動
- 創建名為 "ESP32-OffLineMap-Setup" 的熱點
- 默認密碼：12345678

### 步驟 2：連接到設備熱點
1. 使用手機或電腦搜尋 WiFi 網路
2. 連接到 "ESP32-OffLineMap-Setup"
3. 輸入密碼：12345678

### 步驟 3：訪問配置頁面
1. 瀏覽器會自動打開配置頁面
2. 或手動訪問：`http://192.168.4.1`
3. 進入 WiFi 配置界面

### 步驟 4：配置 WiFi
1. 點擊「掃描網路」按鈕
2. 從列表中選擇要連接的 WiFi
3. 輸入 WiFi 密碼
4. 點擊「連接 WiFi」

### 步驟 5：完成配置
1. 設備嘗試連接到指定的 WiFi
2. 連接成功後配置入口自動關閉
3. 設備正常運行，可在網路中找到

## Web Portal 功能介紹

### 🏠 主頁面 (`/`)
- WiFi 配置表單
- 網路掃描功能
- 現代化的用戶界面

### 🔍 網路掃描 (`/scan`)
- JSON API 接口
- 返回附近 WiFi 網路列表
- 包含 SSID、信號強度、安全性信息

### 💾 配置保存 (`/save`)
- 接收 WiFi 配置參數
- 保存到 EEPROM
- 觸發連接嘗試

### 📊 設備信息 (`/info`)
- 顯示設備詳細信息
- 晶片型號、記憶體使用
- 網路狀態、IP 地址等

### 🔄 重置設定 (`/reset`)
- 清除所有已保存的配置
- 重啟設備
- 返回初始狀態

## 錯誤處理和故障排除

### 常見問題

#### 1. 無法訪問配置頁面
**問題**：連接到設備熱點後無法打開配置頁面

**解決方案**：
- 確認已成功連接到設備熱點
- 手動訪問 `http://192.168.4.1`
- 檢查瀏覽器是否啟用了自動重定向
- 嘗試清除瀏覽器快取

#### 2. WiFi 連接失敗
**問題**：輸入正確的 WiFi 密碼但無法連接

**解決方案**：
- 確認 WiFi 密碼正確
- 檢查 WiFi 網路是否使用特殊字元
- 確認路由器是否限制設備連接
- 嘗試重啟路由器

#### 3. 配置入口自動關閉
**問題**：配置入口在操作過程中突然關閉

**解決方案**：
- 檢查是否超過了配置超時時間（3分鐘）
- 設備可能自動重試連接已保存的配置
- 重新啟動設備進入配置模式

#### 4. 設備重啟後無法自動連接
**問題**：設備重啟後不會自動連接到已配置的 WiFi

**解決方案**：
- 檢查 EEPROM 是否正常工作
- 確認配置是否成功保存
- 使用重置功能清除配置後重新設置

### 調試信息

啟用 Serial Monitor 查看詳細的調試信息：

```
WiFiManager: Initializing...
WiFiManager: Loaded config - SSID: MyWiFi
WiFiManager: Connecting to MyWiFi...
WiFiManager: Connected! IP: 192.168.1.100
```

### 重置設備

如果需要重置 WiFi 配置：

1. **通過 Web 界面**：訪問 `/reset` 頁面
2. **通過代碼**：調用 `wifiManager.resetSettings()`
3. **硬體重置**：長按重置按鈕（如果實現了按鈕功能）

## 整合到現有項目

### 1. 添加到 main.cpp

```cpp
#include "App/Utils/WiFiManager/wifi_manager.h"

WiFiManager wifiManager;

void setup() {
    // 現有的初始化代碼...
    
    // 初始化 WiFi Manager
    wifiManager.begin();
}

void loop() {
    // WiFi Manager 循環
    wifiManager.loop();
    
    // 現有的主循環代碼...
}
```

### 2. 與 OTA 更新集成

```cpp
wifiManager.onConnected = []() {
    Serial.println("WiFi 連接成功，啟動 OTA 檢查");
    
    // 啟動 OTA 更新檢查
    #ifdef ENABLE_AUTO_OTA_CHECK
    otaUpdater.begin(currentVersion, serverURL, versionURL, interval);
    #endif
};
```

### 3. 與 LVGL 界面集成

```cpp
wifiManager.onPortalStart = []() {
    // 在 LVGL 界面顯示配置提示
    showWiFiConfigMessage();
};

wifiManager.onConnected = []() {
    // 更新 LVGL 界面的網路狀態
    updateNetworkStatus(true);
};
```

## 自定義配置

### 修改 AP 設置

在 `wifi_manager.h` 中修改以下常數：

```cpp
#define WIFI_MANAGER_AP_SSID "你的設備名稱"
#define WIFI_MANAGER_AP_PASSWORD "你的密碼"
#define WIFI_MANAGER_TIMEOUT 300000  // 連接超時時間
#define WIFI_MANAGER_PORTAL_TIMEOUT 180000  // 入口超時時間
```

### 自定義界面樣式

修改 `wifi_manager_html.cpp` 中的 CSS 樣式來自定義界面外觀。

### 添加額外功能

可以在 Web Portal 中添加更多功能：
- 設備狀態監控
- 系統設置配置
- 日誌查看
- 韌體更新界面

## 安全注意事項

1. **修改默認密碼**：在生產環境中修改 AP 默認密碼
2. **超時設置**：設置合適的配置入口超時時間
3. **訪問限制**：考慮添加 MAC 地址過濾或其他安全措施
4. **HTTPS 支援**：在需要時添加 HTTPS 支援

## 技術規格

- **支援平台**：ESP32 系列
- **記憶體需求**：約 20KB Flash，2KB RAM
- **網路協議**：IEEE 802.11 b/g/n
- **Web 服務器**：ESP32WebServer
- **DNS 服務**：DNSServer（用於 Captive Portal）
- **配置存儲**：EEPROM（512 bytes）

---

這個 WiFi Manager 提供了完整的配網解決方案，讓 ESP32 設備的 WiFi 配置變得簡單而優雅。通過直觀的 Web 界面，用戶可以輕鬆完成網路設置，而開發者則可以專注於核心功能的開發。