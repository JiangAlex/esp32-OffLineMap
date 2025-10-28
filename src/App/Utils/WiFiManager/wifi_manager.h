#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// WiFi Manager 配置
#define WIFI_MANAGER_AP_SSID "ESP32-OffLineMap-Setup"
#define WIFI_MANAGER_AP_PASSWORD "12345678"
#define WIFI_MANAGER_TIMEOUT 300000  // 5 minutes timeout
#define WIFI_MANAGER_PORTAL_TIMEOUT 600000  // 10 minutes portal timeout

// EEPROM 地址配置
#define EEPROM_SIZE 512
#define SSID_ADDR 0
#define PASSWORD_ADDR 64
#define CONFIG_FLAG_ADDR 128
#define MAGIC_NUMBER 0xAA55

// WiFi 連接狀態
enum WiFiManagerStatus {
    WM_IDLE,
    WM_CONNECTING,
    WM_CONNECTED,
    WM_FAILED,
    WM_AP_MODE,
    WM_PORTAL_ACTIVE
};

// WiFi 配置結構
struct WiFiConfig {
    char ssid[32];
    char password[64];
    bool valid;
};

class WiFiManager {
private:
    WebServer* server;
    DNSServer* dnsServer;
    WiFiManagerStatus status;
    WiFiConfig config;
    unsigned long connectStartTime;
    unsigned long portalStartTime;
    bool portalActive;
    bool autoConnect;
    String deviceName;
    
    // 私有方法
    void setupAP();
    void setupWebServer();
    void setupDNS();
    void handleRoot();
    void handleWiFiScan();
    void handleWiFiSave();
    void handleInfo();
    void handleReset();
    void handleNotFound();
    void saveConfig();
    bool loadConfig();
    void clearConfig();
    String getConfigPage();
    String getInfoPage();
    String getResetPage();
    String getSuccessPage();
    String scanNetworks();
    
public:
    WiFiManager();
    ~WiFiManager();
    
    // 主要方法
    void begin(const char* apSSID = WIFI_MANAGER_AP_SSID, 
               const char* apPassword = WIFI_MANAGER_AP_PASSWORD);
    void loop();
    bool autoConnectToWiFi();
    void startConfigPortal();
    void stopConfigPortal();
    void resetSettings();
    
    // 狀態查詢
    WiFiManagerStatus getStatus() { return status; }
    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
    bool isPortalActive() { return portalActive; }
    String getSSID() { return String(config.ssid); }
    String getIP() { return WiFi.localIP().toString(); }
    
    // 配置方法
    void setDeviceName(const String& name) { deviceName = name; }
    void setAutoConnect(bool enable) { autoConnect = enable; }
    void setTimeout(unsigned long timeout) { /* 保留接口 */ }
    
    // 回調函數類型
    typedef std::function<void()> CallbackFunction;
    CallbackFunction onConnected;
    CallbackFunction onDisconnected;
    CallbackFunction onPortalStart;
    CallbackFunction onPortalStop;
};

#endif // WIFI_MANAGER_H