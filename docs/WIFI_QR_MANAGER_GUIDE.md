# ESP32 WiFi QR Manager 使用指南

## 功能概述

ESP32 WiFi QR Manager 是一個增強版的 WiFi 管理器，集成了 QR Code 顯示功能，讓用戶可以通過掃描 QR Code 快速訪問配置界面。

## 主要特性

### 🔗 WiFi 配置功能
- **Web 配置界面**：響應式 HTML 設計，支持手機瀏覽器
- **Captive Portal**：自動重定向到配置頁面
- **網絡掃描**：自動掃描並顯示可用的 WiFi 網絡
- **憑證持久化**：使用 EEPROM 保存 WiFi 憑證
- **連接狀態監控**：實時顯示連接狀態

### 📱 QR Code 功能
- **動態生成**：根據當前狀態生成不同內容的 QR Code
- **螢幕顯示**：在 ESP32 的 LCD 螢幕上顯示 QR Code
- **智能切換**：配置模式顯示配置 URL，連接後顯示訪問 URL
- **手機掃描**：支持任何具有相機功能的智能手機掃描

## QR Code 顯示內容

### 配置模式
- **QR Code 內容**：`http://192.168.4.1`
- **螢幕顯示**：配置說明和當前狀態
- **用途**：用戶掃描後直接跳轉到 WiFi 配置頁面

### 連接模式
- **QR Code 內容**：`http://[設備IP地址]`
- **螢幕顯示**：連接信息（SSID、IP、信號強度）
- **用途**：用戶掃描後直接訪問設備的 Web 界面

## 編譯和部署

### 編譯環境
```bash
# 編譯 WiFi QR Manager 測試環境
platformio run -e wifi-qr-test

# 上傳到設備
platformio run -e wifi-qr-test -t upload
```

### 環境配置
- **平台**：ESP32-S3
- **框架**：Arduino Framework
- **依賴庫**：
  - LVGL (用於 QR Code 顯示)
  - LovyanGFX (顯示驅動)
  - WiFi、WebServer、DNSServer (網絡功能)

## 使用步驟

### 1. 設備啟動
```
=================================
WiFi QR Manager Test Starting...
=================================
WiFi QR Manager initialized!
QR Code should be displayed on screen
Scan QR Code with phone to connect to WiFi setup
Or manually connect to AP: ESP32-QR-Setup
Password: 12345678
Then open browser and go to: http://192.168.4.1
=================================
```

### 2. QR Code 掃描方式
1. **使用手機相機**掃描螢幕上的 QR Code
2. 手機自動彈出瀏覽器並跳轉到配置頁面
3. 或者手動連接到 AP `ESP32-QR-Setup`

### 3. WiFi 配置流程
1. **掃描網絡**：點擊 "🔍 Scan for Networks"
2. **選擇網絡**：從列表中選擇目標 WiFi
3. **輸入密碼**：在密碼欄位輸入 WiFi 密碼
4. **連接**：點擊 "🔗 Connect to WiFi"

### 4. 連接成功後
- 設備自動切換到 Station 模式
- QR Code 更新為設備 IP 地址
- 螢幕顯示連接狀態信息
- 用戶掃描新的 QR Code 可直接訪問設備

## Web 界面功能

### 📋 網絡列表
- 顯示所有可用的 WiFi 網絡
- 信號強度指示器
- 點擊自動填入 SSID

### 🔧 配置表單
- SSID 輸入框（支持手動輸入）
- 密碼輸入框
- 連接按鈕

### 📊 狀態顯示
- 實時連接狀態
- 錯誤信息提示
- 成功連接確認

## 序列監控輸出

### 定期狀態報告
```
=== WiFi Status Report ===
WiFi Mode: Station Connected
Connected to: MyWiFi
Local IP: 192.168.1.100
Signal Strength: -45 dBm
Gateway: 192.168.1.1
DNS: 192.168.1.1
📱 QR Code on screen shows: http://192.168.1.100
Free Heap: 180000 bytes
Uptime: 300 seconds
========================
```

## 技術架構

### 類別結構
```cpp
class WiFiQRManager {
    // 網絡管理
    WebServer server;
    DNSServer dnsServer;
    
    // QR Code 顯示
    lv_obj_t* qr_code;
    lv_obj_t* info_label;
    lv_obj_t* status_label;
    
    // 主要方法
    void begin(const String& apName, const String& apPassword);
    void loop();
    void showQRCode();
    void showConnectionInfo();
}
```

### LVGL 組件
- **QR Code Widget**：使用 `lv_qrcode_create()` 生成
- **信息標籤**：顯示連接信息和說明
- **狀態標籤**：顯示當前操作狀態

## 故障排除

### 常見問題

1. **QR Code 無法掃描**
   - 確認螢幕亮度足夠
   - 檢查 QR Code 大小設定
   - 清理相機鏡頭

2. **無法連接到配置網絡**
   - 檢查 AP 名稱和密碼設定
   - 確認設備在 AP 模式下運行
   - 檢查手機 WiFi 設定

3. **Web 界面無法訪問**
   - 確認瀏覽器地址：`http://192.168.4.1`
   - 檢查 DNS 伺服器運行狀態
   - 嘗試刷新頁面

### 調試信息
```cpp
// 啟用更詳細的日誌輸出
#define WIFI_QR_DEBUG 1
```

## 擴展功能

### 自定義 QR Code 內容
可以修改 `createQRCode()` 函數來生成包含更多信息的 QR Code：
```cpp
String qr_content = "WIFI:T:WPA;S:" + ssid + ";P:" + password + ";;";
lv_qrcode_update(qr_code, qr_content.c_str(), qr_content.length());
```

### 多語言支持
在 HTML 模板中添加語言切換功能，支持中文、英文等多種語言。

### 主題定制
可以修改 CSS 樣式來定制 Web 界面的外觀和主題。

## 安全注意事項

1. **密碼保護**：建議為 AP 模式設置強密碼
2. **訪問控制**：考慮添加管理員認證功能
3. **憑證加密**：敏感信息的 EEPROM 存儲可以考慮加密
4. **超時設置**：配置模式可以添加自動超時功能

## 版本歷史

### v1.0.0
- 基礎 WiFi 管理功能
- QR Code 顯示功能
- Web 配置界面
- Captive Portal 支持
- EEPROM 憑證持久化

---

這個 WiFi QR Manager 為 ESP32 設備提供了現代化的網絡配置解決方案，讓用戶配置更加便捷和直觀。