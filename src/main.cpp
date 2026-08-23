#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266Ping.h>
#include <LittleFS.h>
#include <ESP8266WebServer.h>

#include "OLED.h"
#include "JSON.h"
#include "PUSHOVER.h"

String deviceName = "WatchDog v0.3";

// --- WI-FI Configuration---
String ssid = "";
String password = "";
IPAddress local_IP(192, 168, 0, 199); // Adres ESP
IPAddress gateway(192, 168, 0, 1);    // Brama (router)
IPAddress subnet(255, 255, 255, 0);   // Maska
IPAddress primaryDNS(8, 8, 8, 8);     // Serwer DNS od Google

// --- PUSHOVER Configuration---
String pushoverApiToken = "";
String pushoverUserKey = "";

// --- JSON Configuration---
String serverJson = "";

// --- Addresses to check ---
IPAddress ips[6] = {
    IPAddress(0, 0, 0, 0),
    IPAddress(0, 0, 0, 0),
    IPAddress(0, 0, 0, 0),
    IPAddress(0, 0, 0, 0),
    IPAddress(0, 0, 0, 0),
    IPAddress(0, 0, 0, 0)};

String ipsPort[6] = {"", "", "", "", "", ""};
String ipsName[6] = {"", "", "", "", "", ""};
int pingFailCounter[6] = {0, 0, 0, 0, 0, 0};
String oledBuffer[7] = {"", "", "", "", "", "", ""};
String status = "  ";
String wifiStatus = "";

ESP8266WebServer server(80);

const char *apName = "ESP-Configuration";
const char *apPassword = "12345678";
bool configSaved = false;

bool stringToIP(const String &str, IPAddress &ip)
{
    return ip.fromString(str);
}

void loadConfiguration()
{
    if (!LittleFS.exists("/config.txt"))
        return;

    File f = LittleFS.open("/config.txt", "r");
    if (!f)
        return;

    String line;

    ssid = f.readStringUntil('\n');
    ssid.trim();

    password = f.readStringUntil('\n');
    password.trim();

    line = f.readStringUntil('\n');
    line.trim();
    stringToIP(line, local_IP);

    pushoverUserKey = f.readStringUntil('\n');
    pushoverUserKey.trim();

    pushoverApiToken = f.readStringUntil('\n');
    pushoverApiToken.trim();

    serverJson = f.readStringUntil('\n');
    serverJson.trim();

    for (int i = 0; i < 6; i++)
    {
        ips[i] = IPAddress(0, 0, 0, 0);
        ipsPort[i] = "";
        ipsName[i] = "-";
        line = f.readStringUntil('\n');
        line.trim();

        if (line.length() > 0)
        {
            int separatorName = line.indexOf(';');

            String address;

            if (separatorName > 0)
            {
                address = line.substring(0, separatorName);
                ipsName[i] = line.substring(separatorName + 1);
            }
            else
            {
                address = line;
            }

            int separatorPort = address.indexOf(':');

            if (separatorPort > 0)
            {
                stringToIP(address.substring(0, separatorPort), ips[i]);
                ipsPort[i] = address.substring(separatorPort + 1);
            }
            else
            {
                stringToIP(address, ips[i]);
            }
        }
    }

    f.close();
}

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

    delay(1000); // time to recieve 200
    ESP.restart(); // hard reset
}

void startAP()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName, apPassword);

    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);

    server.begin();
    updateOLED("CONFIG MODE", "Connected!!","", "Go to IP:", WiFi.softAPIP().toString(), "Waitig for Config...", "", "");
}

void setup()
{
    Serial.begin(115200);
    initOLED();
    updateOLED(deviceName, "", "  (co tu sie      )", "   \\  odpierdala_/", "    \\/", "     |\\__/,|   ( \\", "   _.|o o  |_   ) )", " -(((---(((--------");
    delay(3000);

    LittleFS.begin();
    loadConfiguration();
    startAP();

    unsigned long apStartTime = millis();
    int lastSecondsLeft = -1;
    bool timerActive = true; // Flaga określająca, czy licznik jest włączony

    // Configuration Mode
    while (!configSaved)
    {
        server.handleClient(); // Obsługa zapytań HTTP

        int clients = WiFi.softAPgetStationNum();

        if (clients > 0)
        {
            // Ktoś się podłączył! Trwale wyłączamy odliczanie.
            timerActive = false;
            updateOLED("CONFIG MODE", "", "AP: " + String(apName), "PASS: " + String(apPassword), "Go to IP:", WiFi.softAPIP().toString(), "Waitig for Config", "");
        }
        else if (timerActive)
        {
            // Brak klientów i licznik jest włączony - odliczamy 10 sekund
            int secondsLeft = 10 - ((millis() - apStartTime) / 1000);

            if (secondsLeft != lastSecondsLeft)
            {
                updateOLED("CONFIG MODE", "", "AP: " + String(apName), "PASS: " + String(apPassword), "" ,"Waiting for client", "Starting in: " + String(secondsLeft) + "s", "");
                lastSecondsLeft = secondsLeft;
            }

            if (secondsLeft <= 0)
            {
                Serial.println("o clients. Starting normally.");
                break; // Czas minął, wychodzimy z pętli i ruszamy z kodem dalej
            }
        }
        else

            yield();
    }
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA); // wifi - client mode

    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS))
    {
        Serial.println("IP configuration Error!!!");
    }

    Serial.print(ssid);
    Serial.print(password);
    Serial.print(pushoverApiToken);
    Serial.print(pushoverUserKey);
    Serial.print(serverJson);

    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("\nConnecting to Wi-Fi");

    int wifiTimeout = 0; // timeout counter

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        wifiTimeout++;

        // Jeśli minęło 30 prób (30 * 500ms = 15 sekund)
        if (wifiTimeout > 30)
        {
            Serial.println("\nWiFi connection failed (Timeout). Rebooting...");
            ESP.restart(); // Hard reset
        }
    }

    Serial.println("\nWiFi Connected:");
    Serial.println("Adres IP: " + String(WiFi.localIP().toString()));
    Serial.println("RSSI: " + String(WiFi.RSSI()));

    initPushover(pushoverApiToken, pushoverUserKey, deviceName);
    sendPushover("Hello!! " + ssid + " - Watchdog has just started its watch.");

    initJson(serverJson, deviceName);
    sendJson("String addInfo", 666, "String type", "String requestID");

    updateOLED("ESP8266", "watchdog v0.3", "\x10", "", "\x07", "", "", "");
}

void loop()
{
    for (int x = 0; x < 6; x++)
    {
        switch (WiFi.status())
        {
        case WL_CONNECTED:
            wifiStatus = "CONNECTED";
            break;

        case WL_NO_SSID_AVAIL:
            wifiStatus = "NO SSID";
            break;

        case WL_CONNECT_FAILED:
            wifiStatus = "CONN FAILED";
            break;

        case WL_CONNECTION_LOST:
            wifiStatus = "CONN LOST";
            break;

        case WL_DISCONNECTED:
            wifiStatus = "DISCONN";
            break;

        default:
            Serial.println(WiFi.status());
        }
        oledBuffer[x] = "\x10  | " + ipsName[x];
        updateOLED("SSID: " + String(ssid), wifiStatus + " dBm=" + String(WiFi.RSSI()), oledBuffer[0], oledBuffer[1], oledBuffer[2], oledBuffer[3], oledBuffer[4], oledBuffer[5]);

        delay(2000);
        if (ipsPort[x] == "")
        {
            oledBuffer[x] = "\x10  |" + ips[x].toString();
        }
        else
        {
            String adresPort = ips[x].toString() + ":" + ipsPort[x];

            oledBuffer[x] = "\x10  |" + adresPort.substring(0, 16);
        }
        updateOLED("SSID: " + String(ssid), wifiStatus + " dBm=" + String(WiFi.RSSI()), oledBuffer[0], oledBuffer[1], oledBuffer[2], oledBuffer[3], oledBuffer[4], oledBuffer[5]);
        delay(2000);
        if (ips[x] != IPAddress(0, 0, 0, 0))
        {
            bool ret = false;
            if (ipsPort[x] == "")
            {
                ret = Ping.ping(ips[x]);
            }
            else
            {
                WiFiClient client;
                ret = client.connect(ips[x], ipsPort[x].toInt());
                client.stop();
            }

            if (ret == false)
            {
                if (pingFailCounter[x] < 99)
                {
                    pingFailCounter[x]++;
                    if (pingFailCounter[x] == 5 || pingFailCounter[x] == 50 || pingFailCounter[x] == 99)
                    {
                        sendPushover(ipsName[x] + " - " + ips[x].toString() + " - connection error! Attempt: " + pingFailCounter[x]);
                    }
                }
            }
            else
            {
                if (pingFailCounter[x] >= 5)
                {
                    sendPushover(ipsName[x] + " - " + ips[x].toString() + " - back online!");
                }
                pingFailCounter[x] = 0;
            }

            if (pingFailCounter[x] == 0)
            {
                status = "OK ";
            }
            else if (pingFailCounter[x] == 99)
            {
                status = "OFF";
            }
            else
            {
                status = String(pingFailCounter[x]);
                while (status.length() < 3)
                {
                    status = status + " ";
                }
            }
            oledBuffer[x] = status + "| " + ipsName[x];
        }
        else
        {
            oledBuffer[x] = "-  | -";
        }
    }
    delay(100);
}