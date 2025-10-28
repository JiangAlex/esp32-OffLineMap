# ESP32 OTA 版本檢查修復指南

## 問題描述

原始錯誤信息：
```
Remote version response: v1.0.6
Failed to parse version JSON
Failed to get remote version
```

這個錯誤表明 OTA 系統收到了純文本版本號（如 "v1.0.6"），但代碼期望 JSON 格式的響應。

## 問題原因

原始的 `getRemoteVersion()` 函數只支持 JSON 格式的版本響應：
```json
{
  "version": "1.0.6",
  "build_date": "2025-10-28T10:30:00Z"
}
```

但實際服務器返回的是純文本格式：
```
v1.0.6
```

## 解決方案

### 1. 改進版本解析邏輯

修改 `getRemoteVersion()` 函數以支持兩種格式：

```cpp
if (httpCode == HTTP_CODE_OK) {
    payload = http.getString();
    payload.trim(); // Remove any whitespace
    
    // Check if response is JSON format
    if (payload.startsWith("{")) {
        // Parse JSON response
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            String version = doc["version"].as<String>();
            return version;
        }
    } else {
        // Assume plain text version format
        String version = payload;
        // Remove 'v' prefix if present
        if (version.startsWith("v") || version.startsWith("V")) {
            version = version.substring(1);
        }
        return version;
    }
}
```

### 2. 增強版本比較功能

添加智能版本比較方法，支持語義化版本：

```cpp
int OTAUpdater::compareVersions(const String& version1, const String& version2) {
    // 支持 x.y.z 格式的版本比較
    // 返回值：-1 表示 version1 < version2
    //         0 表示 version1 == version2
    //         1 表示 version1 > version2
}
```

## 測試驗證

### 編譯測試環境

```bash
# 編譯 OTA 測試環境
platformio run -e ota-test

# 上傳到設備測試
platformio run -e ota-test -t upload -t monitor
```

### 測試預期輸出

```
=================================
OTA Version Check Test Starting...
=================================
Connecting to WiFi: YOUR_WIFI_SSID
..........
WiFi connected!
IP address: 192.168.1.100
OTA Updater initialized!
Testing version check...

Remote version response: v1.0.6
Parsed plain text version: 1.0.6
Current: 1.0.1, Remote: 1.0.6
✅ Update available!
```

## 支持的版本格式

### JSON 格式
```json
{
  "version": "1.0.6",
  "build_date": "2025-10-28T10:30:00Z",
  "changelog": "Bug fixes and improvements"
}
```

### 純文本格式
```
1.0.6
```

或帶前綴：
```
v1.0.6
```

## 版本比較邏輯

系統現在支持語義化版本比較：

| 當前版本 | 遠程版本 | 結果 |
|---------|---------|------|
| 1.0.1   | 1.0.6   | 需要更新 |
| 1.0.6   | 1.0.6   | 已是最新 |
| 1.1.0   | 1.0.6   | 當前版本更新 |
| 1.0.1   | 2.0.0   | 需要更新 |

## 配置選項

### platformio.ini 設定

```ini
build_flags = 
    -D ENABLE_AUTO_OTA_CHECK
    -D OTA_CHECK_INTERVAL=3600   # 檢查間隔（秒）
    -D OTA_SERVER_URL=\"https://your-server.com/firmware.bin\"
    -D OTA_VERSION_URL=\"https://your-server.com/version\"
```

### 代碼中初始化

```cpp
#ifdef ENABLE_AUTO_OTA_CHECK
String currentVersion = "1.0.1";
String serverURL = "https://your-server.com/firmware.bin";
String versionURL = "https://your-server.com/version";
unsigned long interval = 3600; // 1 hour

otaUpdater.begin(currentVersion, serverURL, versionURL, interval);
#endif
```

## 使用指南

### 1. 基本版本檢查

```cpp
if (otaUpdater.checkForUpdates()) {
    Serial.println("Update available!");
    // 可選：自動下載更新
    // otaUpdater.performUpdate();
}
```

### 2. 手動更新

```cpp
bool success = otaUpdater.performUpdate();
if (success) {
    Serial.println("Update successful, rebooting...");
} else {
    Serial.println("Update failed");
}
```

### 3. 自動檢查

```cpp
void loop() {
    // 系統會根據設定的間隔自動檢查
    otaUpdater.handleAutoCheck();
    
    // 其他程序邏輯...
}
```

## 錯誤處理

系統現在能更好地處理各種錯誤情況：

1. **網絡錯誤**：WiFi 斷線檢測和重連
2. **HTTP 錯誤**：超時處理和錯誤代碼報告
3. **格式錯誤**：自動識別 JSON 或純文本格式
4. **版本格式**：自動處理 "v" 前綴

## 日誌輸出

修復後的系統提供詳細的日誌信息：

```
Remote version response: v1.0.6
Parsed plain text version: 1.0.6
Current: 1.0.1, Remote: 1.0.6
New version available!
```

## 向後兼容性

此修復完全向後兼容：
- ✅ 支持原有的 JSON 格式
- ✅ 新增純文本格式支持
- ✅ 保持原有 API 接口不變
- ✅ 現有配置無需修改

## 測試建議

1. **格式測試**：測試 JSON 和純文本兩種格式
2. **版本測試**：測試各種版本號格式（帶/不帶 v 前綴）
3. **網絡測試**：測試網絡異常情況
4. **更新測試**：在實際設備上測試完整更新流程

這個修復確保了 OTA 系統的穩定性和兼容性，能夠適應不同的服務器配置和版本號格式。