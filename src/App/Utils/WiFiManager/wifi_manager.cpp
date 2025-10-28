#include "wifi_manager.h"

WiFiManager::WiFiManager() {
    server = nullptr;
    dnsServer = nullptr;
    status = WM_IDLE;
    portalActive = false;
    autoConnect = true;
    deviceName = "ESP32-OffLineMap";
    
    // 初始化配置
    memset(&config, 0, sizeof(config));
    config.valid = false;
}

WiFiManager::~WiFiManager() {
    if (server) {
        delete server;
    }
    if (dnsServer) {
        delete dnsServer;
    }
}

void WiFiManager::begin(const char* apSSID, const char* apPassword) {
    Serial.println("WiFiManager: Initializing...");
    
    // 初始化 EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // 嘗試加載保存的配置
    if (loadConfig()) {
        Serial.printf("WiFiManager: Loaded config - SSID: %s\n", config.ssid);
        
        if (autoConnect) {
            if (autoConnectToWiFi()) {
                status = WM_CONNECTED;
                Serial.println("WiFiManager: Auto-connected to WiFi");
                if (onConnected) onConnected();
                return;
            }
        }
    }
    
    // 自動連接失敗，啟動配置入口
    Serial.println("WiFiManager: Starting configuration portal");
    startConfigPortal();
}

void WiFiManager::loop() {
    if (portalActive && server) {
        server->handleClient();
        if (dnsServer) {
            dnsServer->processNextRequest();
        }
        
        // 檢查入口超時
        if (millis() - portalStartTime > WIFI_MANAGER_PORTAL_TIMEOUT) {
            Serial.println("WiFiManager: Portal timeout, restarting config portal");
            stopConfigPortal();
            
            // 重新啟動配置門戶而不是退出
            delay(1000);
            startConfigPortal();
        }
    }
    
    // 檢查 WiFi 連接狀態
    if (status == WM_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            status = WM_CONNECTED;
            Serial.printf("WiFiManager: Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            if (onConnected) onConnected();
            stopConfigPortal();
        } else if (millis() - connectStartTime > WIFI_MANAGER_TIMEOUT) {
            status = WM_FAILED;
            Serial.println("WiFiManager: Connection timeout");
            if (onDisconnected) onDisconnected();
            startConfigPortal();
        }
    }
}

bool WiFiManager::autoConnectToWiFi() {
    if (!config.valid || strlen(config.ssid) == 0) {
        Serial.println("WiFiManager: No valid config for auto-connect");
        return false;
    }
    
    Serial.printf("WiFiManager: Connecting to %s...\n", config.ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid, config.password);
    
    status = WM_CONNECTING;
    connectStartTime = millis();
    
    // 等待連接結果
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        status = WM_CONNECTED;
        Serial.printf("\nWiFiManager: Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    } else {
        status = WM_FAILED;
        Serial.println("\nWiFiManager: Connection failed");
        return false;
    }
}

void WiFiManager::startConfigPortal() {
    if (portalActive) {
        return;
    }
    
    setupAP();
    setupWebServer();
    setupDNS();
    
    portalActive = true;
    portalStartTime = millis();
    status = WM_PORTAL_ACTIVE;
    
    Serial.printf("WiFiManager: Portal started at http://%s\n", WiFi.softAPIP().toString().c_str());
    if (onPortalStart) onPortalStart();
}

void WiFiManager::stopConfigPortal() {
    if (!portalActive) {
        return;
    }
    
    portalActive = false;
    
    if (server) {
        server->stop();
        delete server;
        server = nullptr;
    }
    
    if (dnsServer) {
        dnsServer->stop();
        delete dnsServer;
        dnsServer = nullptr;
    }
    
    WiFi.softAPdisconnect(true);
    Serial.println("WiFiManager: Portal stopped");
    if (onPortalStop) onPortalStop();
}

void WiFiManager::setupAP() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WIFI_MANAGER_AP_SSID, WIFI_MANAGER_AP_PASSWORD);
    delay(500);
    
    Serial.printf("WiFiManager: AP started - SSID: %s, IP: %s\n", 
                  WIFI_MANAGER_AP_SSID, WiFi.softAPIP().toString().c_str());
}

void WiFiManager::setupWebServer() {
    server = new WebServer(80);
    
    server->on("/", [this]() { handleRoot(); });
    server->on("/scan", [this]() { handleWiFiScan(); });
    server->on("/save", [this]() { handleWiFiSave(); });
    server->on("/info", [this]() { handleInfo(); });
    server->on("/reset", [this]() { handleReset(); });
    server->onNotFound([this]() { handleNotFound(); });
    
    server->begin();
}

void WiFiManager::setupDNS() {
    dnsServer = new DNSServer();
    dnsServer->start(53, "*", WiFi.softAPIP());
}

void WiFiManager::handleRoot() {
    Serial.println("WiFiManager: Serving root page");
    server->send(200, "text/html", getConfigPage());
}

void WiFiManager::handleWiFiScan() {
    Serial.println("WiFiManager: Handling WiFi scan request");
    String result = scanNetworks();
    Serial.printf("WiFiManager: Scan result length: %d\n", result.length());
    server->send(200, "application/json", result);
}

void WiFiManager::handleWiFiSave() {
    Serial.println("WiFiManager: Handling WiFi save request");
    Serial.printf("WiFiManager: Has ssid arg: %s\n", server->hasArg("ssid") ? "true" : "false");
    Serial.printf("WiFiManager: Has password arg: %s\n", server->hasArg("password") ? "true" : "false");
    
    if (server->hasArg("ssid") && server->hasArg("password")) {
        String ssid = server->arg("ssid");
        String password = server->arg("password");
        
        Serial.printf("WiFiManager: Saving SSID: %s\n", ssid.c_str());
        Serial.printf("WiFiManager: Password length: %d\n", password.length());
        
        // 保存配置
        strncpy(config.ssid, ssid.c_str(), sizeof(config.ssid) - 1);
        strncpy(config.password, password.c_str(), sizeof(config.password) - 1);
        config.valid = true;
        
        saveConfig();
        
        server->send(200, "text/html", getSuccessPage());
        
        // 延遲後嘗試連接
        delay(2000);
        stopConfigPortal();
        autoConnectToWiFi();
    } else {
        Serial.println("WiFiManager: Missing SSID or password parameters");
        server->send(400, "text/html", "<h1>Missing parameters</h1>");
    }
}

void WiFiManager::handleInfo() {
    server->send(200, "text/html", getInfoPage());
}

void WiFiManager::handleReset() {
    clearConfig();
    server->send(200, "text/html", getResetPage());
    delay(2000);
    ESP.restart();
}

void WiFiManager::handleNotFound() {
    String uri = server->uri();
    String method = (server->method() == HTTP_GET) ? "GET" : "POST";
    Serial.printf("WiFiManager: 404 - %s %s\n", method.c_str(), uri.c_str());
    Serial.printf("WiFiManager: Available routes: /, /scan, /save, /info, /reset\n");
    
    // Debug: print all arguments
    for (int i = 0; i < server->args(); i++) {
        Serial.printf("WiFiManager: Arg %d: %s = %s\n", i, 
                     server->argName(i).c_str(), 
                     server->arg(i).c_str());
    }
    
    server->sendHeader("Location", "/", true);
    server->send(302, "text/plain", "");
}

bool WiFiManager::loadConfig() {
    uint16_t magic = 0;
    EEPROM.get(CONFIG_FLAG_ADDR, magic);
    
    if (magic != MAGIC_NUMBER) {
        return false;
    }
    
    EEPROM.get(SSID_ADDR, config.ssid);
    EEPROM.get(PASSWORD_ADDR, config.password);
    config.valid = true;
    
    return true;
}

void WiFiManager::saveConfig() {
    EEPROM.put(SSID_ADDR, config.ssid);
    EEPROM.put(PASSWORD_ADDR, config.password);
    EEPROM.put(CONFIG_FLAG_ADDR, (uint16_t)MAGIC_NUMBER);
    EEPROM.commit();
    
    Serial.println("WiFiManager: Configuration saved");
}

void WiFiManager::clearConfig() {
    EEPROM.put(CONFIG_FLAG_ADDR, (uint16_t)0x0000);
    EEPROM.commit();
    memset(&config, 0, sizeof(config));
    config.valid = false;
    
    Serial.println("WiFiManager: Configuration cleared");
}

void WiFiManager::resetSettings() {
    clearConfig();
    WiFi.disconnect(true);
    ESP.restart();
}

String WiFiManager::scanNetworks() {
    DynamicJsonDocument doc(2048);
    JsonArray networks = doc.createNestedArray("networks");
    
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
        JsonObject network = networks.createNestedObject();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
    
    String result;
    serializeJson(doc, result);
    return result;
}

// HTML 頁面模板將在下一個函數中實現...