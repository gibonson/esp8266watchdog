#include <ESP8266WebServer.h>




void handleRoot()
{
    String configContent = "";
    File f = LittleFS.open("/config.txt", "r");
    if (f)
    {
        configContent = f.readString();
        f.close();
    }
    else
    {
        // if no file create template
        configContent = "Nazwa_WiFi\nHaslo_WiFi\n192.168.0.199\nUserKey_Pushover\nApiToken_Pushover\nhttp://192.168.0.242:5000/api/addEvent\n192.168.0.10;Serwer\n192.168.0.11;Kamera\n\n\n\n\n";
    }

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

<h2>Konfiguracja ESP (Raw Config)</h2>

<div class="help-box">
  <strong>File structure (each parameter on a new line):</strong><br>
  1. SSID WiFi<br>
  2. WiFi Password<br>
  3. IP Address (np. 192.168.0.199)<br>
  4. Pushover User Key<br>
  5. Pushover API Token<br>
  6. URL to JSON Server<br>
  7-12. IP addresses to check (Format: <b>IP;Name</b>). Empty lines are allowed.
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
    // form checking fild "configRaw"
    if (!server.hasArg("configRaw"))
    {
        server.send(400, "text/plain", "No configuration data!");
        return;
    }

    File f = LittleFS.open("/config.txt", "w");
    if (!f)
    {
        server.send(500, "text/plain", "Error saving to flash memory");
        return;
    }

    // save data from configRaw to config.txt
    f.print(server.arg("configRaw"));
    f.close();

    server.send(200, "text/html", "<h2>Configuration saved! ESP will reboot now.</h2>");

    delay(1000);   // time to recieve 200
    ESP.restart(); // hard reset
}