#include "ota_updater.h"

OTAUpdater otaUpdater;

OTAUpdater::OTAUpdater() {
    currentVersion = "1.0.0";
    lastCheckTime = 0;
    checkInterval = 3600000; // 1 hour default
    autoCheckEnabled = false;
}

void OTAUpdater::begin(const String& version, const String& serverUrl, const String& versionUrl, unsigned long interval) {
    currentVersion = version;
    serverURL = serverUrl;
    versionURL = versionUrl;
    checkInterval = interval * 1000; // Convert seconds to milliseconds
    
    #ifdef ENABLE_AUTO_OTA_CHECK
    autoCheckEnabled = true;
    Serial.println("OTA Auto-check enabled");
    #endif
    
    Serial.printf("OTA Updater initialized - Current version: %s\n", currentVersion.c_str());
}

void OTAUpdater::enableAutoCheck(bool enable) {
    autoCheckEnabled = enable;
}

bool OTAUpdater::checkWiFiConnection() {
    return WiFi.status() == WL_CONNECTED;
}

String OTAUpdater::getRemoteVersion() {
    if (!checkWiFiConnection()) {
        Serial.println("WiFi not connected, cannot check remote version");
        return "";
    }

    HTTPClient http;
    http.begin(versionURL);
    http.setTimeout(10000); // 10 seconds timeout
    
    int httpCode = http.GET();
    String payload = "";
    
    if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
        Serial.printf("Remote version response: %s\n", payload.c_str());
        
        // Parse JSON response
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            String version = doc["version"].as<String>();
            http.end();
            return version;
        } else {
            Serial.println("Failed to parse version JSON");
        }
    } else {
        Serial.printf("HTTP GET failed, error: %d\n", httpCode);
    }
    
    http.end();
    return "";
}

bool OTAUpdater::checkForUpdates() {
    String remoteVersion = getRemoteVersion();
    
    if (remoteVersion.length() == 0) {
        Serial.println("Failed to get remote version");
        return false;
    }
    
    Serial.printf("Current: %s, Remote: %s\n", currentVersion.c_str(), remoteVersion.c_str());
    
    // Simple version comparison
    if (remoteVersion != currentVersion) {
        Serial.println("New version available!");
        return true;
    }
    
    Serial.println("Already up to date");
    return false;
}

bool OTAUpdater::downloadAndInstallFirmware(const String& firmwareURL) {
    if (!checkWiFiConnection()) {
        Serial.println("WiFi not connected, cannot download firmware");
        return false;
    }

    HTTPClient http;
    http.begin(firmwareURL);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        
        if (contentLength > 0) {
            bool canBegin = Update.begin(contentLength);
            
            if (canBegin) {
                Serial.printf("Starting OTA update. Firmware size: %d bytes\n", contentLength);
                
                WiFiClient* client = http.getStreamPtr();
                size_t written = Update.writeStream(*client);
                
                if (written == contentLength) {
                    Serial.println("Written firmware successfully");
                } else {
                    Serial.printf("Written only %d/%d bytes\n", written, contentLength);
                }
                
                if (Update.end()) {
                    if (Update.isFinished()) {
                        Serial.println("OTA update completed successfully!");
                        http.end();
                        return true;
                    } else {
                        Serial.println("Update not finished? Something went wrong!");
                    }
                } else {
                    Serial.printf("Error Occurred. Error #: %d\n", Update.getError());
                }
            } else {
                Serial.println("Not enough space to begin OTA");
            }
        } else {
            Serial.println("Invalid content length");
        }
    } else {
        Serial.printf("HTTP GET failed, error: %d\n", httpCode);
    }
    
    http.end();
    return false;
}

void OTAUpdater::rebootDevice() {
    Serial.println("Rebooting device in 3 seconds...");
    delay(3000);
    ESP.restart();
}

bool OTAUpdater::performUpdate() {
    if (!checkForUpdates()) {
        return false;
    }
    
    String firmwareURL = serverURL;
    if (!firmwareURL.endsWith("/")) {
        firmwareURL += "/";
    }
    firmwareURL += "firmware.bin";
    
    Serial.printf("Downloading firmware from: %s\n", firmwareURL.c_str());
    
    if (downloadAndInstallFirmware(firmwareURL)) {
        Serial.println("Update successful, rebooting...");
        rebootDevice();
        return true;
    } else {
        Serial.println("Update failed");
        return false;
    }
}

void OTAUpdater::handleAutoCheck() {
    if (!autoCheckEnabled) {
        return;
    }
    
    unsigned long currentTime = millis();
    
    if (currentTime - lastCheckTime >= checkInterval) {
        lastCheckTime = currentTime;
        Serial.println("Performing automatic OTA check...");
        
        if (checkForUpdates()) {
            Serial.println("Update available, starting automatic update...");
            performUpdate();
        }
    }
}

void OTAUpdater::setCurrentVersion(const String& version) {
    currentVersion = version;
}

String OTAUpdater::getLastCheckTime() {
    return String(lastCheckTime);
}