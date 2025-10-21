# WiFi Manager 使用範例

這個目錄包含 WiFi Manager 的使用範例代碼。

## 檔案說明

- `wifi_manager_example.cpp` - 完整的 WiFi Manager 使用範例

## 如何使用範例

1. **創建新的 PlatformIO 專案**：
   ```bash
   pio project init --board esp32-s3-devkitc-1
   ```

2. **複製 WiFi Manager 檔案**：
   ```bash
   # 複製核心檔案到新專案
   cp src/App/Utils/WiFiManager/wifi_manager.h new_project/src/
   cp src/App/Utils/WiFiManager/wifi_manager.cpp new_project/src/
   cp src/App/Utils/WiFiManager/wifi_manager_html.cpp new_project/src/
   ```

3. **複製範例代碼**：
   ```bash
   cp examples/WiFiManager/wifi_manager_example.cpp new_project/src/main.cpp
   ```

4. **編譯並上傳**：
   ```bash
   cd new_project
   pio run --target upload
   ```

## 範例功能

- 自動連接已保存的 WiFi 網路
- 啟動配置入口（AP 模式）
- Web 界面配置 WiFi
- 按鈕控制重置設定
- 狀態回調函數

## 硬體需求

- ESP32-S3 開發板
- 可選：按鈕連接到 GPIO（範例中使用 GPIO 0）

## 配置頁面

當設備進入 AP 模式時：
1. 連接到 WiFi 熱點：`ESP32-OffLineMap-Setup`
2. 打開瀏覽器訪問：`http://192.168.4.1`
3. 選擇要連接的 WiFi 網路並輸入密碼
4. 點擊連接完成配置