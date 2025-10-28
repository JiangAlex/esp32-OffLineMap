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
        payload.trim(); // Remove any whitespace
        Serial.printf("Remote version response: %s\n", payload.c_str());
        
        // Check if response is JSON format
        if (payload.startsWith("{")) {
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
            // Assume plain text version format
            String version = payload;
            // Remove 'v' prefix if present
            if (version.startsWith("v") || version.startsWith("V")) {
                version = version.substring(1);
            }
            Serial.printf("Parsed plain text version: %s\n", version.c_str());
            http.end();
            return version;
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
    
    // Improved version comparison
    int comparison = compareVersions(currentVersion, remoteVersion);
    if (comparison < 0) {
        Serial.println("New version available!");
        return true;
    } else if (comparison > 0) {
        Serial.println("Current version is newer than remote");
        return false;
    } else {
        Serial.println("Already up to date");
        return false;
    }
}

int OTAUpdater::compareVersions(const String& version1, const String& version2) {
    // Normalize versions by removing 'v' prefix
    String v1 = version1;
    String v2 = version2;
    
    if (v1.startsWith("v") || v1.startsWith("V")) {
        v1 = v1.substring(1);
    }
    if (v2.startsWith("v") || v2.startsWith("V")) {
        v2 = v2.substring(1);
    }
    
    // Split versions into components
    int v1Parts[3] = {0, 0, 0};
    int v2Parts[3] = {0, 0, 0};
    
    // Parse version1
    int partIndex = 0;
    int lastDot = 0;
    for (int i = 0; i <= v1.length() && partIndex < 3; i++) {
        if (i == v1.length() || v1.charAt(i) == '.') {
            if (i > lastDot) {
                v1Parts[partIndex] = v1.substring(lastDot, i).toInt();
            }
            lastDot = i + 1;
            partIndex++;
        }
    }
    
    // Parse version2
    partIndex = 0;
    lastDot = 0;
    for (int i = 0; i <= v2.length() && partIndex < 3; i++) {
        if (i == v2.length() || v2.charAt(i) == '.') {
            if (i > lastDot) {
                v2Parts[partIndex] = v2.substring(lastDot, i).toInt();
            }
            lastDot = i + 1;
            partIndex++;
        }
    }
    
    // Compare parts
    for (int i = 0; i < 3; i++) {
        if (v1Parts[i] < v2Parts[i]) return -1;
        if (v1Parts[i] > v2Parts[i]) return 1;
    }
    
    return 0; // Versions are equal
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
    
    // Use the serverURL directly as it should contain the complete firmware URL
    String firmwareURL = serverURL;
    
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