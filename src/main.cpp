#include <Arduino.h>
#include <WiFi.h>

#ifdef ENABLE_AUTO_OTA_CHECK
#include "App/Utils/OTA/ota_updater.h"
#endif

#include "lvgl.h"
#include "ChappieCore/ChappieCore.h"
#include "App/App.h"
#include "App/Common/HAL/HAL.h"

ChappieCore Chappie;

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

  // WiFi connection setup for OTA (請根據你的網路環境修改)
  #ifdef ENABLE_AUTO_OTA_CHECK
  WiFi.begin("your_ssid", "your_password");  // 請修改為你的WiFi SSID和密碼
  Serial.print("Connecting to WiFi");
  int wifi_timeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_timeout < 30) {
      delay(1000);
      Serial.print(".");
      wifi_timeout++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());
  } else {
      Serial.println();
      Serial.println("WiFi connection failed! OTA updates will be disabled.");
  }
  #endif
  
  #ifdef ENABLE_AUTO_OTA_CHECK
  // Initialize OTA updater
  String currentVersion = "1.0.5"; // 设置当前版本
  
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
  // Handle automatic OTA checks
  otaUpdater.handleAutoCheck();
  #endif
}
