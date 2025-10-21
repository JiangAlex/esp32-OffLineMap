# WiFi Manager 功能測試指南

## 測試環境編譯成功 ✅

WiFi Manager 測試程序已成功編譯，內存使用情況：
- RAM: 13.7% (44,844 / 327,680 bytes)
- Flash: 23.1% (770,961 / 3,342,336 bytes)

## 測試步驟

### 1. 燒錄測試固件
```bash
cd /home/alex_chiang/Documents/ESP32-Pio-GPT/GitHub/esp32-OffLineMap
platformio run -e wifi-test -t upload
```

### 2. 監控串口輸出
```bash
platformio device monitor -e wifi-test
```

### 3. 預期的串口輸出
```
=================================
WiFi Manager Test Starting...
=================================
WiFiManager: Initializing...
WiFiManager: Starting configuration portal
WiFi Manager initialized!
Please connect to AP: ESP32-OffLineMap-Setup
Password: 12345678
Then open browser and go to: http://192.168.4.1
=================================
WiFi Status: Disconnected
Free heap: XXXXX bytes
-------------------------
```

### 4. 功能測試流程

#### 步驟 1: 連接 WiFi AP
1. 在手機/電腦上搜索 WiFi 網路
2. 連接到 `ESP32-OffLineMap-Setup`
3. 輸入密碼: `12345678`

#### 步驟 2: 配置 WiFi
1. 打開瀏覽器，訪問 `http://192.168.4.1`
2. 應該看到 WiFi 配置頁面，包含：
   - WiFi 網路掃描列表
   - SSID 輸入框
   - 密碼輸入框
   - 連接/重置按鈕

#### 步驟 3: 驗證功能
1. **網路掃描**: 頁面應顯示可用的 WiFi 網路列表
2. **配置保存**: 輸入有效的 WiFi 憑證並點擊連接
3. **自動連接**: ESP32 應該連接到指定的 WiFi 網路
4. **串口反饋**: 串口應顯示連接狀態和獲得的 IP 地址

#### 步驟 4: 測試不同頁面
1. **主配置頁面**: `http://192.168.4.1/`
2. **資訊頁面**: `http://192.168.4.1/info`
3. **重置頁面**: `http://192.168.4.1/reset`

### 5. 預期的 Web 介面

#### 配置頁面特點:
- 響應式設計，支持手機瀏覽器
- 自動刷新 WiFi 網路列表
- 密碼顯示/隱藏切換
- 連接狀態實時顯示

#### 成功連接後:
串口應顯示類似輸出：
```
WiFiManager: Connecting to YourWiFiNetwork...
WiFiManager: Connected! IP: 192.168.1.100
WiFi Status: Connected to YourWiFiNetwork, IP: 192.168.1.100
```

### 6. 故障排除

#### 常見問題:
1. **無法找到 AP**: 確認 ESP32 已正確啟動
2. **無法訪問網頁**: 確認已連接到 ESP32 的 AP
3. **連接失敗**: 檢查 WiFi 憑證是否正確
4. **超時重試**: WiFi Manager 會自動重新啟動配置入口

#### 調試信息:
- 串口監控器提供詳細的連接狀態
- 每5秒更新一次狀態信息
- 包含內存使用情況監控

### 7. 測試完成標準

✅ ESP32 成功建立 AP 模式
✅ 可以連接到配置 AP
✅ Web 配置頁面正常顯示
✅ WiFi 掃描功能正常
✅ 能夠保存 WiFi 配置
✅ 自動連接到配置的 WiFi
✅ 串口輸出正確的狀態信息
✅ 獲得有效的 IP 地址

## 整合到主項目

測試成功後，可以將 WiFi Manager 整合到主項目中：

1. 在 `main.cpp` 中包含 WiFi Manager
2. 在適當的時機調用 `wifiManager.begin()`
3. 在主循環中調用 `wifiManager.loop()`
4. 根據需要添加 WiFi 狀態回調

## 性能指標

- **記憶體佔用**: 約44KB RAM
- **Flash 佔用**: 約771KB
- **啟動時間**: < 3秒
- **配置超時**: 300秒 (5分鐘)
- **連接超時**: 30秒

這個 WiFi Manager 實現提供了完整的 WiFi 配置功能，適合在 ESP32 離線地圖項目中使用。