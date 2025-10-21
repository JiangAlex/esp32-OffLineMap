#include "wifi_manager.h"

// Configuration page HTML template
String WiFiManager::getConfigPage() {
    return R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Offline Map - WiFi Setup</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }

        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            padding: 40px;
            max-width: 500px;
            width: 100%;
            animation: slideUp 0.6s ease-out;
        }

        @keyframes slideUp {
            from {
                transform: translateY(30px);
                opacity: 0;
            }
            to {
                transform: translateY(0);
                opacity: 1;
            }
        }

        .header {
            text-align: center;
            margin-bottom: 30px;
        }

        h1 {
            color: #333;
            font-size: 28px;
            margin-bottom: 10px;
        }

        .header p {
            color: #666;
            font-size: 16px;
        }

        .form-group {
            margin-bottom: 25px;
        }

        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 500;
        }

        input[type="text"], input[type="password"], select {
            width: 100%;
            padding: 12px 16px;
            border: 2px solid #e1e5e9;
            border-radius: 10px;
            font-size: 16px;
            transition: border-color 0.3s ease;
        }

        input[type="text"]:focus, input[type="password"]:focus, select:focus {
            outline: none;
            border-color: #667eea;
        }

        .btn {
            padding: 12px 24px;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            cursor: pointer;
            transition: all 0.3s ease;
            text-decoration: none;
            display: inline-block;
            text-align: center;
        }

        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            width: 100%;
        }

        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
        }

        .btn-secondary {
            background: #f8f9fa;
            color: #333;
            border: 2px solid #e1e5e9;
            margin-bottom: 15px;
        }

        .btn-secondary:hover {
            background: #e9ecef;
        }

        .network-list {
            max-height: 200px;
            overflow-y: auto;
            border: 2px solid #e1e5e9;
            border-radius: 10px;
            margin-top: 10px;
        }

        .network-item {
            padding: 12px 16px;
            border-bottom: 1px solid #e1e5e9;
            cursor: pointer;
            transition: background-color 0.2s ease;
        }

        .network-item:hover {
            background-color: #f8f9fa;
        }

        .network-item:last-child {
            border-bottom: none;
        }

        .network-name {
            font-weight: 500;
            color: #333;
        }

        .network-details {
            font-size: 14px;
            color: #666;
            margin-top: 4px;
        }

        .loading {
            text-align: center;
            padding: 20px;
            color: #666;
        }

        .footer {
            text-align: center;
            margin-top: 30px;
        }

        .footer a {
            color: #667eea;
            text-decoration: none;
            font-size: 14px;
        }

        .footer a:hover {
            text-decoration: underline;
        }

        @media (max-width: 600px) {
            .container {
                padding: 30px 20px;
                margin: 10px;
            }
            
            h1 {
                font-size: 24px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>ESP32 Offline Map</h1>
            <p>WiFi Network Configuration</p>
        </div>
        
        <form id="wifiForm">
            <div class="form-group">
                <label for="networkSelect">Available Networks:</label>
                <button type="button" class="btn btn-secondary" onclick="scanNetworks()">
                    Scan Networks
                </button>
                <div id="networkList" class="network-list" style="display: none;">
                    <div class="loading">Scanning networks...</div>
                </div>
            </div>
            
            <div class="form-group">
                <label for="ssid">Network Name (SSID):</label>
                <input type="text" id="ssid" name="ssid" required>
            </div>
            
            <div class="form-group">
                <label for="password">Password:</label>
                <input type="password" id="password" name="password">
            </div>
            
            <button type="submit" class="btn btn-primary">
                Connect WiFi
            </button>
        </form>
        
        <div class="footer">
            <a href="/reset">Reset Settings</a>
        </div>
    </div>

    <script>
        var networks = [];
        
        function scanNetworks() {
            var networkList = document.getElementById("networkList");
            networkList.style.display = "block";
            networkList.innerHTML = "<div class=\"loading\">Scanning networks...</div>";
            
            fetch("/scan")
                .then(function(response) { return response.json(); })
                .then(function(data) {
                    networks = data.networks;
                    displayNetworks(networks);
                })
                .catch(function(error) {
                    networkList.innerHTML = "<div class=\"loading\">Scan failed, please retry</div>";
                });
        }
        
        function displayNetworks(networkList) {
            var container = document.getElementById("networkList");
            
            if (networkList.length === 0) {
                container.innerHTML = "<div class=\"loading\">No networks found</div>";
                return;
            }
            
            container.innerHTML = "";
            for (var i = 0; i < networkList.length; i++) {
                var network = networkList[i];
                var networkDiv = document.createElement("div");
                networkDiv.className = "network-item";
                networkDiv.onclick = function(ssid) {
                    return function() { selectNetwork(ssid); };
                }(network.ssid);
                
                var secure = network.secure ? "[Secured]" : "[Open]";
                var strength = getSignalStrength(network.rssi);
                
                networkDiv.innerHTML = 
                    "<div class=\"network-name\">" + network.ssid + " " + secure + "</div>" +
                    "<div class=\"network-details\">Signal: " + strength + " (" + network.rssi + " dBm)</div>";
                
                container.appendChild(networkDiv);
            }
        }
        
        function selectNetwork(ssid) {
            document.getElementById("ssid").value = ssid;
            document.getElementById("networkList").style.display = "none";
        }
        
        function getSignalStrength(rssi) {
            if (rssi > -50) return "Excellent";
            if (rssi > -60) return "Good";
            if (rssi > -70) return "Fair";
            return "Weak";
        }
        
        document.getElementById("wifiForm").addEventListener("submit", function(e) {
            e.preventDefault();
            
            var ssid = document.getElementById("ssid").value;
            var password = document.getElementById("password").value;
            
            if (!ssid) {
                alert("Please enter network name");
                return;
            }
            
            var submitBtn = e.target.querySelector(".btn-primary");
            submitBtn.innerHTML = "Connecting...";
            submitBtn.disabled = true;
            
            fetch("/connect", {
                method: "POST",
                headers: {
                    "Content-Type": "application/x-www-form-urlencoded"
                },
                body: "ssid=" + encodeURIComponent(ssid) + "&password=" + encodeURIComponent(password)
            })
            .then(function(response) {
                if (response.ok) {
                    window.location.href = "/success";
                } else {
                    throw new Error("Connection failed");
                }
            })
            .catch(function(error) {
                alert("Connection failed, please check network name and password");
                submitBtn.innerHTML = "Connect WiFi";
                submitBtn.disabled = false;
            });
        });
        
        window.addEventListener("load", function() {
            setTimeout(scanNetworks, 1000);
        });
    </script>
</body>
</html>
)HTML";
}

// Success page HTML
String WiFiManager::getSuccessPage() {
    return R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Connection Successful - ESP32 Offline Map</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #56ab2f 0%, #a8e6cf 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }

        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            padding: 60px 40px;
            max-width: 500px;
            width: 100%;
            text-align: center;
            animation: bounceIn 0.8s ease-out;
        }

        @keyframes bounceIn {
            0% {
                transform: scale(0.3);
                opacity: 0;
            }
            50% {
                transform: scale(1.05);
            }
            70% {
                transform: scale(0.9);
            }
            100% {
                transform: scale(1);
                opacity: 1;
            }
        }

        .success-icon {
            font-size: 80px;
            margin-bottom: 30px;
            color: #28a745;
        }

        h1 {
            color: #28a745;
            font-size: 32px;
            margin-bottom: 20px;
        }

        p {
            color: #666;
            font-size: 18px;
            line-height: 1.6;
            margin-bottom: 30px;
        }

        .countdown {
            font-size: 16px;
            color: #999;
            margin-bottom: 20px;
        }

        .back-btn {
            display: inline-block;
            padding: 12px 30px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            text-decoration: none;
            border-radius: 25px;
            font-size: 16px;
            transition: all 0.3s ease;
        }

        .back-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
        }

        @media (max-width: 600px) {
            .container {
                padding: 40px 20px;
                margin: 10px;
            }
            
            h1 {
                font-size: 28px;
            }
            
            .success-icon {
                font-size: 60px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="success-icon">[SUCCESS]</div>
        <h1>Connection Successful!</h1>
        <p>Your ESP32 device has successfully connected to the WiFi network.<br>
        The configuration portal will close automatically in a few seconds.</p>
        <div class="countdown" id="countdown">Closing in 10 seconds...</div>
        <a href="/" class="back-btn">Back to Configuration</a>
    </div>

    <script>
        var timeLeft = 10;
        var countdownElement = document.getElementById("countdown");
        
        var timer = setInterval(function() {
            timeLeft--;
            countdownElement.textContent = "Closing in " + timeLeft + " seconds...";
            
            if (timeLeft <= 0) {
                clearInterval(timer);
                countdownElement.textContent = "Closing configuration portal...";
                setTimeout(function() {
                    document.body.innerHTML = "<div style=\"text-align: center; padding: 50px; font-family: Arial;\">Configuration complete. Please reconnect to your WiFi network.</div>";
                }, 2000);
            }
        }, 1000);
    </script>
</body>
</html>
)HTML";
}

// Device info page HTML
String WiFiManager::getInfoPage() {
    return R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Device Information - ESP32 Offline Map</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }

        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            overflow: hidden;
        }

        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }

        .header h1 {
            font-size: 28px;
            margin-bottom: 10px;
        }

        .content {
            padding: 40px;
        }

        .info-section {
            margin-bottom: 30px;
        }

        .info-section h2 {
            color: #333;
            font-size: 20px;
            margin-bottom: 15px;
            border-bottom: 2px solid #667eea;
            padding-bottom: 5px;
        }

        .info-grid {
            display: grid;
            grid-template-columns: 1fr 2fr;
            gap: 15px;
            margin-bottom: 20px;
        }

        .info-label {
            font-weight: 500;
            color: #555;
        }

        .info-value {
            color: #333;
            font-family: monospace;
            background: #f8f9fa;
            padding: 8px 12px;
            border-radius: 5px;
        }

        .back-btn {
            display: inline-block;
            padding: 12px 30px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            text-decoration: none;
            border-radius: 25px;
            font-size: 16px;
            transition: all 0.3s ease;
            margin-right: 10px;
        }

        .back-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
        }

        .reset-btn {
            background: #dc3545;
        }

        .reset-btn:hover {
            box-shadow: 0 10px 20px rgba(220, 53, 69, 0.3);
        }

        @media (max-width: 600px) {
            .info-grid {
                grid-template-columns: 1fr;
                gap: 10px;
            }
            
            .content {
                padding: 20px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Device Information</h1>
            <p>ESP32 Offline Map System Details</p>
        </div>
        
        <div class="content">
            <div class="info-section">
                <h2>Hardware Information</h2>
                <div class="info-grid">
                    <div class="info-label">Chip Model:</div>
                    <div class="info-value" id="chipModel">Loading...</div>
                    
                    <div class="info-label">MAC Address:</div>
                    <div class="info-value" id="macAddress">Loading...</div>
                    
                    <div class="info-label">Flash Size:</div>
                    <div class="info-value" id="flashSize">Loading...</div>
                    
                    <div class="info-label">Free Memory:</div>
                    <div class="info-value" id="freeHeap">Loading...</div>
                </div>
            </div>
            
            <div class="info-section">
                <h2>Software Information</h2>
                <div class="info-grid">
                    <div class="info-label">Firmware Version:</div>
                    <div class="info-value" id="firmwareVersion">Loading...</div>
                    
                    <div class="info-label">Build Time:</div>
                    <div class="info-value" id="buildTime">Loading...</div>
                    
                    <div class="info-label">Uptime:</div>
                    <div class="info-value" id="uptime">Loading...</div>
                </div>
            </div>
            
            <div class="info-section">
                <h2>Network Information</h2>
                <div class="info-grid">
                    <div class="info-label">WiFi Status:</div>
                    <div class="info-value" id="wifiStatus">Loading...</div>
                    
                    <div class="info-label">Connected Network:</div>
                    <div class="info-value" id="connectedSSID">Loading...</div>
                    
                    <div class="info-label">IP Address:</div>
                    <div class="info-value" id="ipAddress">Loading...</div>
                    
                    <div class="info-label">Signal Strength:</div>
                    <div class="info-value" id="signalStrength">Loading...</div>
                </div>
            </div>
            
            <div style="text-align: center; margin-top: 40px;">
                <a href="/" class="back-btn">Back to Configuration</a>
                <a href="/reset" class="back-btn reset-btn">Reset Settings</a>
            </div>
        </div>
    </div>

    <script>
        function loadDeviceInfo() {
            fetch("/info")
                .then(function(response) { return response.json(); })
                .then(function(data) {
                    document.getElementById("chipModel").textContent = data.chipModel || "Unknown";
                    document.getElementById("macAddress").textContent = data.macAddress || "Unknown";
                    document.getElementById("flashSize").textContent = data.flashSize || "Unknown";
                    document.getElementById("freeHeap").textContent = data.freeHeap || "Unknown";
                    document.getElementById("firmwareVersion").textContent = data.firmwareVersion || "Unknown";
                    document.getElementById("buildTime").textContent = data.buildTime || "Unknown";
                    document.getElementById("uptime").textContent = data.uptime || "Unknown";
                    document.getElementById("wifiStatus").textContent = data.wifiStatus || "Unknown";
                    document.getElementById("connectedSSID").textContent = data.connectedSSID || "Not connected";
                    document.getElementById("ipAddress").textContent = data.ipAddress || "Unknown";
                    document.getElementById("signalStrength").textContent = data.signalStrength || "Unknown";
                })
                .catch(function(error) {
                    console.error("Failed to load device information:", error);
                });
        }
        
        window.addEventListener("load", loadDeviceInfo);
        
        setInterval(function() {
            loadDeviceInfo();
        }, 5000);
    </script>
</body>
</html>
)HTML";
}

// Reset confirmation page HTML
String WiFiManager::getResetPage() {
    return R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Reset Settings - ESP32 Offline Map</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #dc3545 0%, #fd7e14 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }

        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            padding: 60px 40px;
            max-width: 500px;
            width: 100%;
            text-align: center;
            animation: slideUp 0.6s ease-out;
        }

        @keyframes slideUp {
            from {
                transform: translateY(30px);
                opacity: 0;
            }
            to {
                transform: translateY(0);
                opacity: 1;
            }
        }

        .reset-icon {
            font-size: 80px;
            margin-bottom: 30px;
            color: #dc3545;
        }

        h1 {
            color: #dc3545;
            font-size: 32px;
            margin-bottom: 20px;
        }

        p {
            color: #666;
            font-size: 18px;
            line-height: 1.6;
            margin-bottom: 40px;
        }

        .btn-group {
            display: flex;
            gap: 20px;
            justify-content: center;
            flex-wrap: wrap;
        }

        .btn {
            padding: 15px 30px;
            border: none;
            border-radius: 25px;
            font-size: 16px;
            cursor: pointer;
            text-decoration: none;
            transition: all 0.3s ease;
            min-width: 120px;
        }

        .btn-danger {
            background: #dc3545;
            color: white;
        }

        .btn-danger:hover {
            background: #c82333;
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(220, 53, 69, 0.3);
        }

        .btn-secondary {
            background: #6c757d;
            color: white;
        }

        .btn-secondary:hover {
            background: #545b62;
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(108, 117, 125, 0.3);
        }

        @media (max-width: 600px) {
            .container {
                padding: 40px 20px;
                margin: 10px;
            }
            
            h1 {
                font-size: 28px;
            }
            
            .reset-icon {
                font-size: 60px;
            }
            
            .btn-group {
                flex-direction: column;
                align-items: center;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="reset-icon">[RESET]</div>
        <h1>Reset Settings</h1>
        <p>This operation will clear all saved WiFi configurations and device settings.<br>
        The device will restart and return to initial configuration state.</p>
        
        <div class="btn-group">
            <button class="btn btn-danger" onclick="confirmReset()">Confirm Reset</button>
            <a href="/" class="btn btn-secondary">Cancel</a>
        </div>
    </div>

    <script>
        function confirmReset() {
            if (confirm("Are you sure you want to reset all settings? This action cannot be undone.")) {
                fetch("/reset", {
                    method: "POST"
                })
                .then(function(response) {
                    if (response.ok) {
                        alert("Settings have been reset, device will restart.");
                        document.body.innerHTML = "<div style=\"text-align: center; padding: 50px; font-family: Arial;\">Resetting settings and restarting...</div>";
                    } else {
                        alert("Reset failed, please try again.");
                    }
                })
                .catch(function(error) {
                    alert("Reset failed, please try again.");
                });
            }
        }
    </script>
</body>
</html>
)HTML";
}