#include "OLED.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266Ping.h>
#include <LittleFS.h>

#include <ESP8266WebServer.h>

// --- KONFIGURACJA WI-FI ---
String ssid = "";
String password = "";
IPAddress local_IP(192, 168, 0, 199); // Adres ESP
IPAddress gateway(192, 168, 0, 1);    // Brama (router)
IPAddress subnet(255, 255, 255, 0);   // Maska
IPAddress primaryDNS(8, 8, 8, 8);     // Serwer DNS od Google

// --- KONFIGURACJA PUSHOVER ---
String pushoverApiToken = "";
String pushoverUserKey = "";

#include "PUSHOVER.h"

String serverJson = "";
// String serverJson = "http://192.168.0.242:5000/api/addEvent";

#include "JSON.h"

// --- Adresy do skanowania ---
IPAddress ips[6] = {
    IPAddress(0, 0, 0, 0),
    IPAddress(0, 0, 0, 0),
    IPAddress(0, 0, 0, 0),
    IPAddress(0, 0, 0, 0),
    IPAddress(0, 0, 0, 0),
    IPAddress(0, 0, 0, 0)};

String ipsName[6] = {
    "",
    "",
    "",
    "",
    "",
    ""};

int pingFailCounter[6] = {0, 0, 0, 0, 0, 0};
String oledBuffer[7] = {"", "", "", "", "", "", ""};
String status = "  ";
String wifiStatus = "";

ESP8266WebServer server(80);

const char *apName = "ESP-Konfiguracja";
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
        ipsName[i] = "-";
        line = f.readStringUntil('\n');
        line.trim();

        if (line.length() > 0)
        {

            int separator = line.indexOf(';');

            if (separator > 0)
            {
                stringToIP(line.substring(0, separator), ips[i]);
                ipsName[i] = line.substring(separator + 1);
            }
        }
    }

    f.close();
}

void handleRoot()
{
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Konfiguracja ESP</title>
<style>
body { font-family: Arial; margin:40px; }
input { width:350px; margin-bottom:10px; }
</style>
</head>
<body>

<h2>Konfiguracja ESP</h2>

<form action="/save" method="POST">

SSID:<br>
<input name="ssid" value=")rawliteral";

    html += ssid;

    html += R"rawliteral("><br><br>

Hasło:<br>
<input name="password" value=")rawliteral";

    html += password;

    html += R"rawliteral("><br><br>

IP ESP:<br>
<input name="localIP" value=")rawliteral";

    html += local_IP.toString();

    html += R"rawliteral("><br><br>

User Key:<br>
<input name="userKey" value=")rawliteral";

    html += pushoverUserKey;

    html += R"rawliteral("><br><br>

API Token:<br>
<input name="apiToken" value=")rawliteral";

    html += pushoverApiToken;

    html += R"rawliteral("><br><br>

serverJson:<br>
<input name="serverJson" value=")rawliteral";

    html += serverJson;

    html += R"rawliteral("><br><br>

IP1;Name:<br>
<input name="ip1" value=")rawliteral";

    html += ips[0].toString() + ";" + ipsName[0];

    html += R"rawliteral("><br><br>

IP2;Name:<br>
<input name="ip2" value=")rawliteral";

    html += ips[1].toString() + ";" + ipsName[1];

    html += R"rawliteral("><br><br>

IP3;Name:<br>
<input name="ip3" value=")rawliteral";

    html += ips[2].toString() + ";" + ipsName[2];

    html += R"rawliteral("><br><br>

IP4;Name:<br>
<input name="ip4" value=")rawliteral";

    html += ips[3].toString() + ";" + ipsName[3];

    html += R"rawliteral("><br><br>

IP5;Name:<br>
<input name="ip5" value=")rawliteral";

    html += ips[4].toString() + ";" + ipsName[4];

    html += R"rawliteral("><br><br>

IP6;Name:<br>
<input name="ip6" value=")rawliteral";

    html += ips[5].toString() + ";" + ipsName[5];

    html += R"rawliteral("><br><br>

<input type="submit" value="Zapisz">

</form>

</body>
</html>
)rawliteral";

    server.send(200, "text/html", html);
}

void handleSave()
{
    File f = LittleFS.open("/config.txt", "w");

    if (!f)
    {
        server.send(500, "text/plain", "Blad zapisu");
        return;
    }

    f.println(server.arg("ssid"));
    f.println(server.arg("password"));
    f.println(server.arg("localIP"));
    f.println(server.arg("userKey"));
    f.println(server.arg("apiToken"));
    f.println(server.arg("serverJson"));

    f.println(server.arg("ip1"));
    f.println(server.arg("ip2"));
    f.println(server.arg("ip3"));
    f.println(server.arg("ip4"));
    f.println(server.arg("ip5"));
    f.println(server.arg("ip6"));

    f.close();

    server.send(200, "text/html",
                "<h2>Zapisano konfigurację.</h2>");

    configSaved = true; // Zmieniamy flagę - to przerwie pętlę w setup()
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
    updateOLED("TRYB KONFIGURACJI", "Polaczono z WiFi!", "Wejdz na adres:", WiFi.softAPIP().toString(), "Czekam na zapis...", "", "", "");
}

void setup()
{
    Serial.begin(115200);
    initOLED();
    updateOLED("watchdog v0.1", "\x10 IP: ", "", "\x07 RSSI: ", "", "", "", "");

    LittleFS.begin();
    loadConfiguration();
    startAP();

    unsigned long apStartTime = millis();
    int lastSecondsLeft = -1;
    bool timerActive = true; // Flaga określająca, czy licznik jest włączony

    // 3. Pętla trybu konfiguracji
    while (!configSaved)
    {
        server.handleClient(); // Obsługa zapytań HTTP

        int clients = WiFi.softAPgetStationNum();

        if (clients > 0)
        {
            // Ktoś się podłączył! Trwale wyłączamy odliczanie.
            timerActive = false;
            updateOLED("TRYB KONFIGURACJI", "", "AP: " + String(apName), "PASS: " + String(apPassword), "Wejdz na adres:", WiFi.softAPIP().toString(), "Czekam na zapis...", "");
        }
        else if (timerActive)
        {
            // Brak klientów i licznik jest włączony - odliczamy 10 sekund
            int secondsLeft = 10 - ((millis() - apStartTime) / 1000);

            if (secondsLeft != lastSecondsLeft)
            {
                updateOLED("TRYB KONFIGURACJI", "", "AP: " + String(apName), "PASS: " + String(apPassword), "Oczekuje na klienta", "Start za: " + String(secondsLeft) + "s", "", "");
                lastSecondsLeft = secondsLeft;
            }

            if (secondsLeft <= 0)
            {
                Serial.println("Brak klientow. Startuje normalnie.");
                break; // Czas minął, wychodzimy z pętli i ruszamy z kodem dalej
            }
        }
        else

            yield(); // Pozwala ESP "odetchnąć"
    }

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
    Serial.print("\nŁączenie z WiFi");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nPołączono z siecią WiFi!");
    Serial.println("Adres IP: " + String(WiFi.localIP().toString()));
    Serial.println("RSSI: " + String(WiFi.RSSI()));

    sendPushover("Witaj! To jest testowa wiadomość z ESP8266.");
    sendJson("addInfo", 666, "String type", "String requestID");

    updateOLED("ESP8266", "watchdog v0.2", "\x10", "", "\x07", "", "", "");
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

        oledBuffer[x] = "\x10  | " + ips[x].toString();
        updateOLED("SSID: " + String(ssid), wifiStatus + " dBm=" + String(WiFi.RSSI()), oledBuffer[0], oledBuffer[1], oledBuffer[2], oledBuffer[3], oledBuffer[4], oledBuffer[5]);

        if (ips[x] != IPAddress(0, 0, 0, 0))
        {
            bool ret = Ping.ping(ips[x]);
            if (ret == 0)
            {
                if (pingFailCounter[x] < 99)
                {
                    pingFailCounter[x]++;
                }
                if (pingFailCounter[x] == 5 || pingFailCounter[x] == 50 || pingFailCounter[x] == 99)
                {
                    sendPushover(ips[x].toString() + " - connection error! Attempt: " + pingFailCounter[x]);
                    pingFailCounter[x]++;
                }
            }
            else
            {
                if (pingFailCounter[x] != 0)
                {
                    sendPushover(ips[x].toString() + " - back online");
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