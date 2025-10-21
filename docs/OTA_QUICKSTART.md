# ESP32 OTA 更新功能說明

## 快速測試 OTA 功能

### 1. 準備測試環境

```bash
# 檢查系統配置
./test_ota.sh check

# 顯示當前版本
./version.sh current
```

### 2. 修改 WiFi 設置

編輯 `src/main.cpp`，修改 WiFi 連接信息：
```cpp
WiFi.begin("your_ssid", "your_password");  // 改為你的 WiFi
```

### 3. 構建並上傳初始版本

```bash
# 構建固件
pio run -e esp32-s3-devkitc-1

# 上傳到設備
pio run -e esp32-s3-devkitc-1 -t upload

# 監控串行輸出
pio device monitor
```

### 4. 創建新版本進行 OTA 測試

```bash
# 增加版本號
./version.sh increment patch

# 構建並創建發布
./test_ota.sh release $(./version.sh get)

# 推送標籤到 GitHub 觸發自動構建
./test_ota.sh tag v$(./version.sh get)
```

### 5. 監控 OTA 更新過程

設備會自動檢查更新並顯示類似日誌：
```
WiFi connected! IP address: 192.168.1.100
OTA Updater initialized - Current version: 1.0.5
Performing initial OTA check on boot...
Checking for updates from: https://jiangalex.github.io/esp32-OffLineMap/firmware/latest_version.txt
Current: 1.0.5, Remote: 1.0.6
New version available!
Downloading firmware from: https://jiangalex.github.io/esp32-OffLineMap/firmware/firmware-png.bin
Starting OTA update. Firmware size: 1234567 bytes
Download progress: 25%...50%...75%...100%
OTA update completed successfully!
Rebooting device in 3 seconds...
```

## 服務器 URL

- **版本檢查**: https://jiangalex.github.io/esp32-OffLineMap/firmware/latest_version.txt
- **固件下載 (PNG)**: https://jiangalex.github.io/esp32-OffLineMap/firmware/firmware-png.bin
- **固件下載 (BIN)**: https://jiangalex.github.io/esp32-OffLineMap/firmware/firmware-bin.bin
- **版本信息**: https://jiangalex.github.io/esp32-OffLineMap/firmware/version.json
- **GitHub Pages 根目錄**: https://jiangalex.github.io/esp32-OffLineMap/firmware/

## 故障排除

1. **WiFi 連接問題**: 確保 SSID 和密碼正確
2. **OTA 失敗**: 檢查網絡連接和服務器狀態
3. **版本檢查失敗**: 驗證 GitHub Pages 是否正確部署
4. **固件下載失敗**: 檢查固件文件是否存在於 GitHub Pages
5. **自動構建失敗**: 查看 GitHub Actions 工作流程日誌

## GitHub Actions 自動化

當您推送帶有版本標籤的提交時，GitHub Actions 會自動：
1. 構建 PNG 和 BIN 兩個版本的固件
2. 創建 GitHub Release
3. 部署文件到 GitHub Pages
4. 生成版本信息文件

### 觸發自動構建：
```bash
./version.sh increment patch    # 增加版本號
git add -A && git commit -m "Release v$(./version.sh get)"
git tag "v$(./version.sh get)"
git push origin master --tags
```

詳細說明請參閱: `docs/OTA_TESTING_GUIDE.md`