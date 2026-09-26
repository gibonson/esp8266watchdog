#ifndef BOARD_WEB_GUI_H
#define BOARD_WEB_GUI_H


#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

void handleRoot()
{
    String configContent = readRawConfig();

    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Konfiguracja ESP</title>
<style>
body { font-family: Arial; margin:40px; }
textarea { width: 100%; max-width: 500px; font-family: monospace; white-space: pre; }
.help-box { background-color: #f2f2f2; border-left: 4px solid #4CAF50; padding: 10px; margin-bottom: 20px; max-width: 480px; font-size: 13px; line-height: 1.4; }
</style>
</head>
<body>

<h2>ESP Configuration (Raw)</h2>

<div class="help-box">
  <strong>File structure (each parameter on a new line):</strong><br>
  1. SSID WiFi<br>
  2. WiFi Password<br>
  3. IP Address (np. 192.168.0.199)<br>
  4. Pushover User Key<br>
  5. Pushover API Token<br>
  6. URL to JSON Server<br>
  7-12. IP addresses to check. Empty lines are allowed.

{<br>
  "wifi": {<br>
    "ssid": "",<br>
    "password": "",<br>
    "localIp": "192.168.0.199"<br>
  },<br>
  "pushover": {<br>
    "userKey": "",<br>
    "apiToken": ""<br>
  },<br>
  "serverJson": "http://192.168.0.242:5000/api/addEvent",<br>
  "deviceList": [<br>
    {<br>
      "name": "Router",<br>
      "ip": "192.168.0.1"<br>
    },<br>
    {<br>
      "name": "Google DNS (Test)",<br>
      "ip": "8.8.8.8"<br>
    }<br>
  ]<br>
}";

</div>

<form action="/save" method="POST">
<textarea name="configRaw" rows="18">)rawliteral";

    html += configContent;

    html += R"rawliteral(</textarea><br><br>
<input type="submit" value="Save and Reboot">
</form>

</body>
</html>
)rawliteral";

    server.send(200, "text/html", html);
}

void handleSave()
{
    if (!server.hasArg("configRaw"))
    {
        server.send(400, "text/plain", "No configuration data!");
        return;
    }

    if (!saveRawConfig(server.arg("configRaw")))
    {
        server.send(500, "text/plain", "Error saving to flash memory");
        return;
    }

    server.send(200, "text/html", "<h2>Configuration saved! ESP will reboot now.</h2>");

    delay(1000);
    ESP.restart();
}

void initWebGUI()
{
    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();
    Serial.println(F("Web GUI Started"));
}

#endif //BOARD_WEB_GUI_H