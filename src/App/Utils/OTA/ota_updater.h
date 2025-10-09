#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

class OTAUpdater {
private:
    String currentVersion;
    String serverURL;
    String versionURL;
    unsigned long lastCheckTime;
    unsigned long checkInterval;
    bool autoCheckEnabled;
    
    bool checkWiFiConnection();
    String getRemoteVersion();
    bool downloadAndInstallFirmware(const String& firmwareURL);
    void rebootDevice();

public:
    OTAUpdater();
    void begin(const String& version, const String& serverUrl, const String& versionUrl, unsigned long interval = 3600000);
    void enableAutoCheck(bool enable);
    bool checkForUpdates();
    void handleAutoCheck();
    bool performUpdate();
    void setCurrentVersion(const String& version);
    String getLastCheckTime();
};

extern OTAUpdater otaUpdater;

#endif