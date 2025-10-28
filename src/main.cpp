#include <Arduino.h>
#include <WiFi.h>

#ifdef ENABLE_AUTO_OTA_CHECK
#include "App/Utils/OTA/ota_updater.h"
extern OTAUpdater otaUpdater;  // 外部聲明
#endif

#include "lvgl.h"
#include "ChappieCore/ChappieCore.h"
#include "App/App.h"
#include "App/Common/HAL/HAL.h"
#include "App/Version.h"
#include "App/Utils/WiFiManager/wifi_manager.h"

ChappieCore Chappie;

#ifdef ENABLE_AUTO_OTA_CHECK
WiFiManager wifiManager;  // 全局聲明
#endif

void setup()
{
  Serial.begin(115200);

  /* Init Chappie Core */
  Chappie.begin();

  /* Connect HAL */
  HAL::HAL_Init();
  //HAL::Key_Init();
  HAL::Buzz_SetEnable(true);  // 启用蜂鸣器

  /* UI Create */
  App_Init();

  /* Initialize LVGL encoder after UI is ready */
  HAL::Encoder_InitLVGL();

  /* Memory Check */
  _LOG("[PSRAM] free PSRAM: %d\r\n", ESP.getFreePsram());
  // Serial.printf("Deafult free size: %d\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
  // Serial.printf("PSRAM free size: %d\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

#if !CHAPPIE_CORE_USE_LVGL_WITHOUT_RTOS
  Chappie.lvgl.enable();
#endif

  // 启动提示音
  HAL::Buzz_Tone(1000, 200);  // 1kHz, 200ms

  // WiFi connection setup for OTA using WiFi Manager
  #ifdef ENABLE_AUTO_OTA_CHECK
  Serial.println("===== WIFI MANAGER DEBUG =====");
  Serial.println("Starting WiFi connection...");
  
  // 初始化 WiFi Manager 和 EEPROM
  wifiManager.begin();
  
  // 嘗試自動連接到保存的網路
  bool connected = wifiManager.autoConnectToWiFi();
  Serial.printf("autoConnectToWiFi() returned: %s\n", connected ? "true" : "false");
  
  if (connected) {
      Serial.println("Connected to saved WiFi network!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
  } else {
      Serial.println("Failed to connect to saved network.");
      Serial.println("Starting WiFi configuration portal...");
      Serial.printf("Connect to AP: %s\n", WIFI_MANAGER_AP_SSID);
      Serial.printf("Password: %s\n", WIFI_MANAGER_AP_PASSWORD);
      
      // 啟動 WiFi Manager 配置門戶
      Serial.println("Calling startConfigPortal()...");
      wifiManager.startConfigPortal();
      Serial.println("startConfigPortal() called");
      Serial.println("WiFi Manager will handle timeout and restart automatically.");
      Serial.println("Configuration portal will stay active until WiFi is configured.");
  }
  Serial.println("===== WIFI MANAGER DEBUG END =====");
  #endif
  
  #ifdef ENABLE_AUTO_OTA_CHECK
  // Initialize OTA updater
  String currentVersion = String(VERSION_SOFTWARE).substring(1); // 移除 'v' 前綴：v1.0.5 -> 1.0.5
  
  #ifdef OTA_SERVER_URL
  String serverURL = OTA_SERVER_URL;
  #else
  String serverURL = "http://your-server.com/firmware";
  #endif
  
  #ifdef OTA_VERSION_URL
  String versionURL = OTA_VERSION_URL;
  #else
  String versionURL = "http://your-server.com/version";
  #endif
  
  #ifdef OTA_CHECK_INTERVAL
  unsigned long interval = OTA_CHECK_INTERVAL;
  #else
  unsigned long interval = 3600; // 1 hour default
  #endif
  
  otaUpdater.begin(currentVersion, serverURL, versionURL, interval);
  
  // Perform initial update check on boot
  Serial.println("Performing initial OTA check on boot...");
  delay(5000); // Wait for WiFi to stabilize
  
  if (otaUpdater.checkForUpdates()) {
      Serial.println("Update available on boot, starting update...");
      otaUpdater.performUpdate();
  }
  #endif
}

void loop()
{
#if CHAPPIE_CORE_USE_LVGL_WITHOUT_RTOS
  lv_timer_handler();
  delay(1);
#endif
  
  // 更新HAL组件
  //HAL::Key_Update();
  
  delay(10);
  
  #ifdef ENABLE_AUTO_OTA_CHECK
  // Handle WiFi Manager
  wifiManager.loop();
  
  // Handle automatic OTA checks
  otaUpdater.handleAutoCheck();
  #endif
}
