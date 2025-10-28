#include "wifi_qr_manager.h"

WiFiQRManager::WiFiQRManager() : server(80), isConfigMode(false), shouldConnect(false) {
    qr_code = nullptr;
    info_label = nullptr;
    status_label = nullptr;
}

void WiFiQRManager::begin(const String& apName, const String& apPassword) {
    Serial.println("WiFi QR Manager Starting...");
    
    ap_ssid = apName;
    ap_password = apPassword;
    
    EEPROM.begin(512);
    
    // 嘗試加載保存的憑證
    if (loadCredentials() && sta_ssid.length() > 0) {
        Serial.println("Found saved credentials, attempting to connect...");
        connectToWiFi();
        
        // 等待連接
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected to WiFi!");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            isConfigMode = false;
            showConnectionInfo();
            return;
        }
    }
    
    // 如果連接失敗或沒有保存的憑證，進入配置模式
    Serial.println("Entering configuration mode...");
    setupAP();
    setupWebServer();
    isConfigMode = true;
    
    // 顯示配置用的 QR Code
    showQRCode();
}

void WiFiQRManager::setupAP() {
    WiFi.mode(WIFI_AP_STA);
    
    if (ap_password.length() > 0) {
        WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
    } else {
        WiFi.softAP(ap_ssid.c_str());
    }
    
    delay(2000);
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    
    // 設置 DNS 服務器用於 Captive Portal
    dnsServer.start(53, "*", IP);
}

void WiFiQRManager::setupWebServer() {
    server.on("/", [this]() { handleRoot(); });
    server.on("/scan", [this]() { handleScan(); });
    server.on("/connect", [this]() { handleConnect(); });
    server.on("/info", [this]() { handleInfo(); });
    
    // Captive Portal 重定向
    server.onNotFound([this]() {
        server.sendHeader("Location", "http://192.168.4.1", true);
        server.send(302, "text/plain", "");
    });
    
    server.begin();
    Serial.println("Web server started");
}

void WiFiQRManager::handleRoot() {
    server.send(200, "text/html", getHTML());
}

void WiFiQRManager::handleScan() {
    String json = "[";
    int n = WiFi.scanNetworks();
    
    for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"encryption\":" + String(WiFi.encryptionType(i));
        json += "}";
    }
    json += "]";
    
    server.send(200, "application/json", json);
}

void WiFiQRManager::handleConnect() {
    if (server.hasArg("ssid") && server.hasArg("password")) {
        sta_ssid = server.arg("ssid");
        sta_password = server.arg("password");
        
        saveCredentials();
        shouldConnect = true;
        
        server.send(200, "text/plain", "Connecting to " + sta_ssid + "...");
    } else {
        server.send(400, "text/plain", "Missing SSID or password");
    }
}

void WiFiQRManager::handleInfo() {
    String info = "{";
    info += "\"mode\":\"" + String(isConfigMode ? "config" : "station") + "\",";
    info += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
    info += "\"sta_ip\":\"" + WiFi.localIP().toString() + "\",";
    info += "\"sta_ssid\":\"" + WiFi.SSID() + "\",";
    info += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
    info += "}";
    
    server.send(200, "application/json", info);
}

void WiFiQRManager::connectToWiFi() {
    WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
    updateStatusDisplay("Connecting to " + sta_ssid + "...");
}

void WiFiQRManager::loop() {
    if (isConfigMode) {
        dnsServer.processNextRequest();
        server.handleClient();
        
        if (shouldConnect) {
            shouldConnect = false;
            connectToWiFi();
            
            // 等待連接結果
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 30) {
                delay(500);
                attempts++;
                updateStatusDisplay("Connecting... (" + String(attempts) + "/30)");
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("Connected to WiFi!");
                isConfigMode = false;
                hideQRCode();
                showConnectionInfo();
                
                // 停止 AP 模式
                WiFi.softAPdisconnect(true);
                dnsServer.stop();
                server.stop();
            } else {
                updateStatusDisplay("Connection failed! Back to config mode.");
            }
        }
    }
    
    // 運行 LVGL 任務
    lv_timer_handler();
}

void WiFiQRManager::createQRCode(const String& url) {
    // 創建 QR Code 顯示區域
    if (qr_code == nullptr) {
        qr_code = lv_qrcode_create(lv_scr_act(), 150, lv_color_black(), lv_color_white());
        lv_obj_center(qr_code);
        lv_obj_set_pos(qr_code, lv_obj_get_x(qr_code), lv_obj_get_y(qr_code) - 40);
    }
    
    // 設置 QR Code 內容
    lv_qrcode_update(qr_code, url.c_str(), url.length());
    
    // 創建信息標籤
    if (info_label == nullptr) {
        info_label = lv_label_create(lv_scr_act());
        lv_obj_align_to(info_label, qr_code, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        lv_obj_set_style_text_align(info_label, LV_TEXT_ALIGN_CENTER, 0);
    }
    
    String info_text = "Scan QR Code to connect\n" + url;
    lv_label_set_text(info_label, info_text.c_str());
    
    // 創建狀態標籤
    if (status_label == nullptr) {
        status_label = lv_label_create(lv_scr_act());
        lv_obj_align_to(status_label, info_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    }
    
    updateStatusDisplay("Configuration Mode Active");
}

void WiFiQRManager::updateStatusDisplay(const String& status) {
    if (status_label != nullptr) {
        lv_label_set_text(status_label, status.c_str());
    }
    Serial.println("Status: " + status);
}

void WiFiQRManager::showConnectionInfo() {
    // 隱藏配置相關的顯示
    hideQRCode();
    
    // 顯示連接信息
    if (info_label == nullptr) {
        info_label = lv_label_create(lv_scr_act());
        lv_obj_center(info_label);
        lv_obj_set_style_text_align(info_label, LV_TEXT_ALIGN_CENTER, 0);
    }
    
    String connection_info = "WiFi Connected!\n";
    connection_info += "SSID: " + WiFi.SSID() + "\n";
    connection_info += "IP: " + WiFi.localIP().toString() + "\n";
    connection_info += "Signal: " + String(WiFi.RSSI()) + " dBm";
    
    lv_label_set_text(info_label, connection_info.c_str());
    
    // 創建訪問網址的 QR Code
    String web_url = "http://" + WiFi.localIP().toString();
    
    if (qr_code == nullptr) {
        qr_code = lv_qrcode_create(lv_scr_act(), 120, lv_color_black(), lv_color_white());
        lv_obj_align_to(qr_code, info_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    }
    
    lv_qrcode_update(qr_code, web_url.c_str(), web_url.length());
    
    if (status_label == nullptr) {
        status_label = lv_label_create(lv_scr_act());
        lv_obj_align_to(status_label, qr_code, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    }
    
    lv_label_set_text(status_label, ("Scan to visit: " + web_url).c_str());
}

void WiFiQRManager::showQRCode() {
    String config_url = "http://192.168.4.1";
    createQRCode(config_url);
}

void WiFiQRManager::hideQRCode() {
    if (qr_code != nullptr) {
        lv_obj_del(qr_code);
        qr_code = nullptr;
    }
    if (info_label != nullptr) {
        lv_obj_del(info_label);
        info_label = nullptr;
    }
    if (status_label != nullptr) {
        lv_obj_del(status_label);
        status_label = nullptr;
    }
}

bool WiFiQRManager::loadCredentials() {
    String ssid = "";
    String password = "";
    
    // 讀取 SSID 長度
    int ssid_len = EEPROM.read(0);
    if (ssid_len <= 0 || ssid_len > 32) return false;
    
    // 讀取 SSID
    for (int i = 0; i < ssid_len; ++i) {
        ssid += char(EEPROM.read(1 + i));
    }
    
    // 讀取密碼長度
    int pass_len = EEPROM.read(33);
    if (pass_len < 0 || pass_len > 64) return false;
    
    // 讀取密碼
    for (int i = 0; i < pass_len; ++i) {
        password += char(EEPROM.read(34 + i));
    }
    
    sta_ssid = ssid;
    sta_password = password;
    
    return ssid.length() > 0;
}

void WiFiQRManager::saveCredentials() {
    // 清除 EEPROM
    for (int i = 0; i < 512; ++i) {
        EEPROM.write(i, 0);
    }
    
    // 保存 SSID
    EEPROM.write(0, sta_ssid.length());
    for (int i = 0; i < sta_ssid.length(); ++i) {
        EEPROM.write(1 + i, sta_ssid[i]);
    }
    
    // 保存密碼
    EEPROM.write(33, sta_password.length());
    for (int i = 0; i < sta_password.length(); ++i) {
        EEPROM.write(34 + i, sta_password[i]);
    }
    
    EEPROM.commit();
    Serial.println("Credentials saved");
}

String WiFiQRManager::getHTML() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 WiFi Manager</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 20px; 
            background: #f5f5f5;
        }
        .container { 
            max-width: 600px; 
            margin: 0 auto; 
            background: white; 
            padding: 20px; 
            border-radius: 10px; 
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 { 
            color: #333; 
            text-align: center; 
        }
        .network-list { 
            margin: 20px 0; 
        }
        .network-item { 
            padding: 10px; 
            border: 1px solid #ddd; 
            margin: 5px 0; 
            border-radius: 5px; 
            cursor: pointer; 
            background: #f9f9f9;
        }
        .network-item:hover { 
            background: #e9e9e9; 
        }
        .signal-strength { 
            float: right; 
            color: #666; 
        }
        input[type="text"], input[type="password"] { 
            width: 100%; 
            padding: 10px; 
            margin: 5px 0; 
            border: 1px solid #ddd; 
            border-radius: 5px; 
            box-sizing: border-box;
        }
        button { 
            background: #4CAF50; 
            color: white; 
            padding: 12px 20px; 
            border: none; 
            border-radius: 5px; 
            cursor: pointer; 
            width: 100%; 
            font-size: 16px;
        }
        button:hover { 
            background: #45a049; 
        }
        .scan-btn {
            background: #2196F3;
        }
        .scan-btn:hover {
            background: #1976D2;
        }
        .status { 
            margin: 20px 0; 
            padding: 10px; 
            border-radius: 5px; 
            text-align: center;
        }
        .success { 
            background: #d4edda; 
            color: #155724; 
            border: 1px solid #c3e6cb;
        }
        .error { 
            background: #f8d7da; 
            color: #721c24; 
            border: 1px solid #f5c6cb;
        }
        .qr-info {
            background: #d1ecf1;
            color: #0c5460;
            border: 1px solid #bee5eb;
            margin: 10px 0;
            padding: 10px;
            border-radius: 5px;
            text-align: center;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔧 ESP32 WiFi Manager</h1>
        
        <div class="qr-info">
            📱 Scan the QR code on device screen to access this page quickly!
        </div>
        
        <button class="scan-btn" onclick="scanNetworks()">🔍 Scan for Networks</button>
        
        <div id="networkList" class="network-list"></div>
        
        <form onsubmit="connectWiFi(event)">
            <h3>WiFi Credentials</h3>
            <input type="text" id="ssid" placeholder="WiFi Network Name (SSID)" required>
            <input type="password" id="password" placeholder="WiFi Password">
            <button type="submit">🔗 Connect to WiFi</button>
        </form>
        
        <div id="status" class="status" style="display:none;"></div>
    </div>

    <script>
        function scanNetworks() {
            showStatus('Scanning for networks...', 'info');
            fetch('/scan')
                .then(response => response.json())
                .then(networks => {
                    const list = document.getElementById('networkList');
                    list.innerHTML = '<h3>Available Networks:</h3>';
                    
                    networks.forEach(network => {
                        const item = document.createElement('div');
                        item.className = 'network-item';
                        item.innerHTML = `
                            <strong>${network.ssid}</strong>
                            <span class="signal-strength">${getSignalIcon(network.rssi)} ${network.rssi}dBm</span>
                        `;
                        item.onclick = () => selectNetwork(network.ssid);
                        list.appendChild(item);
                    });
                    
                    hideStatus();
                })
                .catch(error => {
                    showStatus('Error scanning networks: ' + error, 'error');
                });
        }
        
        function selectNetwork(ssid) {
            document.getElementById('ssid').value = ssid;
        }
        
        function getSignalIcon(rssi) {
            if (rssi > -50) return '📶';
            if (rssi > -60) return '📶';
            if (rssi > -70) return '📶';
            return '📶';
        }
        
        function connectWiFi(event) {
            event.preventDefault();
            
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('password').value;
            
            showStatus('Connecting to ' + ssid + '...', 'info');
            
            const formData = new FormData();
            formData.append('ssid', ssid);
            formData.append('password', password);
            
            fetch('/connect', {
                method: 'POST',
                body: formData
            })
            .then(response => response.text())
            .then(result => {
                showStatus(result, 'success');
                
                // Check connection status after 10 seconds
                setTimeout(checkStatus, 10000);
            })
            .catch(error => {
                showStatus('Error connecting: ' + error, 'error');
            });
        }
        
        function checkStatus() {
            fetch('/info')
                .then(response => response.json())
                .then(info => {
                    if (info.connected) {
                        showStatus(`✅ Connected! Access point at: http://${info.sta_ip}`, 'success');
                    } else {
                        showStatus('❌ Connection failed. Please try again.', 'error');
                    }
                });
        }
        
        function showStatus(message, type) {
            const status = document.getElementById('status');
            status.textContent = message;
            status.className = 'status ' + type;
            status.style.display = 'block';
        }
        
        function hideStatus() {
            document.getElementById('status').style.display = 'none';
        }
        
        // Auto-scan on page load
        window.onload = () => {
            scanNetworks();
        };
    </script>
</body>
</html>
)rawliteral";
}

bool WiFiQRManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiQRManager::getLocalIP() {
    return WiFi.localIP().toString();
}