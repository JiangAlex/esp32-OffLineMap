# ESP32 離線地圖項目

## 📋 項目概述

ESP32 離線地圖是一個基於 ESP32-S3 的智能地圖顯示設備，具備離線地圖瀏覽、WiFi 配網、OTA 更新等功能。

## ✨ 主要功能

### 🗺️ **離線地圖瀏覽**
- 支援 PNG 和 BIN 兩種地圖格式
- 流暢的地圖縮放和平移
- 基於 LVGL 的現代化用戶界面

### 📡 **智能 WiFi 配網**
- 自動 AP 模式和配網入口
- 美觀的 Web Portal 配置界面
- 支援 WiFi 網路掃描和快速選擇
- 配置持久化存儲

### 🔄 **OTA 固件更新**
- 自動檢查和下載固件更新
- 支援 GitHub Pages 和 GitHub Releases
- 安全的固件驗證和回滾機制

### 🛠️ **開發者友好**
- 模組化設計，易於擴展
- 完整的 API 文檔和示例
- 支援多種開發環境

## 🚀 快速開始

### 硬體需求
- ESP32-S3 開發板
- 顯示屏（支援 LVGL）
- 8MB Flash 存儲空間

### 軟體依賴
- PlatformIO 開發環境
- ESP32 Arduino Core 2.0.0+
- LVGL 8.3.1+

### 編譯說明

⚠️ **重要提醒**：編譯該源碼需要注意 `lib/lv_conf.h`
- 需要將 `lv_conf.h` 放在 LVGL 庫文件夾的同一目錄：`.pio/libdeps/esp32-s3-devkitc-1`

```bash
# 克隆項目
git clone https://github.com/JiangAlex/esp32-OffLineMap.git
cd esp32-OffLineMap

# 編譯項目
pio run -e esp32-s3-devkitc-1

# 上傳到設備
pio run -e esp32-s3-devkitc-1 -t upload
```

## 📖 文檔指南

### WiFi 配網功能
- [📘 WiFi Manager 完整指南](docs/WIFI_MANAGER_GUIDE.md) - 詳細功能說明和 API 文檔
- [🚀 WiFi 快速入門](docs/WIFI_QUICKSTART.md) - 用戶配網操作指南
- [🔧 整合指南](docs/WIFI_INTEGRATION_GUIDE.md) - 開發者整合教程

### OTA 更新功能  
- [📘 OTA 測試指南](docs/OTA_TESTING_GUIDE.md) - 完整的 OTA 功能說明
- [🚀 OTA 快速入門](OTA_QUICKSTART.md) - 快速測試 OTA 功能

### 開發文檔
- [📝 修改日誌](docs/) - 詳細的版本更新記錄
- [🔄 OTA 流程圖](docs/sequenceDiagram_OTA.mmd) - OTA 更新流程圖

## 🌐 在線服務

### OTA 更新服務器
- **GitHub Pages**: https://jiangalex.github.io/esp32-OffLineMap/firmware/
- **版本檢查**: https://jiangalex.github.io/esp32-OffLineMap/firmware/latest_version.txt
- **固件下載**: 
  - PNG 版本: https://jiangalex.github.io/esp32-OffLineMap/firmware/firmware-png.bin
  - BIN 版本: https://jiangalex.github.io/esp32-OffLineMap/firmware/firmware-bin.bin

## 🛠️ 開發工具

### 版本管理
```bash
# 查看當前版本
./version.sh current

# 獲取版本號（用於腳本）
./version.sh get

# 增加版本號
./version.sh increment patch
```

### OTA 測試
```bash
# 檢查 OTA 配置
./test_ota.sh check

# 構建並發布
./test_ota.sh release $(./version.sh get)

# 創建 Git 標籤
./test_ota.sh tag v$(./version.sh get)
```

## 🏗️ 項目結構

```
esp32-OffLineMap/
├── src/
│   ├── main.cpp                    # 主程式入口
│   ├── App/                        # 應用程式邏輯
│   │   ├── Utils/
│   │   │   ├── WiFiManager/        # WiFi 配網模組
│   │   │   └── OTA/                # OTA 更新模組
│   │   └── ...
│   └── ChappieCore/                # 核心硬體抽象層
├── docs/                           # 說明文檔
├── lib/                            # 第三方庫
├── .github/workflows/              # GitHub Actions
├── platformio.ini                  # PlatformIO 配置
├── version.sh                      # 版本管理腳本
└── test_ota.sh                     # OTA 測試腳本
```

## 🤝 貢獻指南

歡迎提交 Issue 和 Pull Request！

1. Fork 此項目
2. 創建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打開 Pull Request

## 📄 授權協議

此項目採用 MIT 授權協議。詳情請見 [LICENSE](LICENSE) 文件。

## 🙏 致謝

- [LVGL](https://lvgl.io/) - 圖形用戶界面庫
- [PlatformIO](https://platformio.org/) - 嵌入式開發平台  
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) - ESP32 Arduino 支援

---

## 📊 項目狀態

![GitHub release](https://img.shields.io/github/v/release/JiangAlex/esp32-OffLineMap)
![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/JiangAlex/esp32-OffLineMap/ota-build.yml)
![GitHub](https://img.shields.io/github/license/JiangAlex/esp32-OffLineMap)

**當前版本**: v1.0.6  
**最後更新**: 2025-10-21
