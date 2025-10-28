#ifndef WIFI_QR_MANAGER_H
#define WIFI_QR_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include "lvgl.h"

class WiFiQRManager {
private:
    WebServer server;
    DNSServer dnsServer;
    
    String ap_ssid;
    String ap_password;
    String sta_ssid;
    String sta_password;
    
    bool isConfigMode;
    bool shouldConnect;
    
    // QR Code 相關
    lv_obj_t* qr_code;
    lv_obj_t* info_label;
    lv_obj_t* status_label;
    
    void setupAP();
    void setupWebServer();
    void handleRoot();
    void handleScan();
    void handleConnect();
    void handleInfo();
    String getHTML();
    void saveCredentials();
    bool loadCredentials();
    void connectToWiFi();
    
    // QR Code 功能
    void createQRCode(const String& url);
    void updateStatusDisplay(const String& status);
    void showConnectionInfo();
    
public:
    WiFiQRManager();
    void begin(const String& apName = "ESP32-Config", const String& apPassword = "");
    void loop();
    bool isConnected();
    String getLocalIP();
    void showQRCode();
    void hideQRCode();
};

#endif