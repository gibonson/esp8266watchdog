#include <Arduino.h>
#include <ESP8266Ping.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>

#include "MODULE_OLED.h"

#include "BOARD_CONFIG.h"
#include "BOARD_WIFI.h"
#include "BOARD_WEB_GUI.h"
#include "BOARD_JSON.h"
#include "BOARD_PUSHOVER.h"

String deviceName = "WatchDog v0.5";

int pingFailCounter[6] = {0, 0, 0, 0, 0, 0};
String oledBuffer[7] = {"", "", "", "", "", "", ""};
String iPstatus = "  ";

unsigned long previousMillis = 0; // Przechowuje czas ostatniej akcji
int currentHostIndex = 0;         // Wskazuje, który z 6 hostów (0-5) aktualnie przetwarzamy
int currentPhase = 0;             // Wskazuje krok (0 = pokaż nazwę, 1 = pokaż IP, 2 = pinguj)

const char *accessPointName = "ESP-Configuration";
const char *accessPointPassword = "12345678";
bool configSaved = false;

void setup()
{

    Serial.begin(115200);
    initOLED();
    updateOLED(deviceName, "", "  (co tu sie      )", "   \\  odpierdala_/", "    \\/", "     |\\__/,|   ( \\", "   _.|o o  |_   ) )", " -(((---(((--------");
    delay(3000);

    initLittleFS();
    loadConfiguration();

    String apIP = initAccessPoint(accessPointName, accessPointPassword);
    initWebGUI();
    updateOLED("CONFIG MODE", "AP Started!", "", "Go to IP:", apIP, "Waiting for Config...", "", "");

    unsigned long apStartTime = millis();
    int lastSecondsLeft = -1;
    bool timerActive = true; // Flaga określająca, czy licznik jest włączony

    // Configuration Mode
    while (!configSaved)
    {
        server.handleClient(); // Obsługa zapytań HTTP

        int clients = WiFi.softAPgetStationNum();

        if (clients > 0) // Ktoś się podłączył! Trwale wyłączamy odliczanie.
        {
            timerActive = false;
            updateOLED("CONFIG MODE", "", "AP: " + String(accessPointName), "PASS: " + String(accessPointPassword), "Go to IP:", WiFi.softAPIP().toString(), "Waitig for Config", "");
        }
        else if (timerActive)
        {
            // Brak klientów i licznik jest włączony - odliczamy 10 sekund
            int secondsLeft = 10 - ((millis() - apStartTime) / 1000);

            if (secondsLeft != lastSecondsLeft)
            {
                updateOLED("CONFIG MODE", "", "AP: " + String(accessPointName), "PASS: " + String(accessPointPassword), "", "Waiting for client", "Starting in: " + String(secondsLeft) + "s", "");
                lastSecondsLeft = secondsLeft;
            }

            if (secondsLeft <= 0)
            {
                Serial.println("no clients. Starting normally.");
                break; // Czas minął, wychodzimy z pętli i ruszamy z kodem dalej
            }
        }
        else

            yield();
    }

    server.stop();
    WiFi.softAPdisconnect(true);

    if (!initWifiClient(ssid, password))
    {
        ESP.reset();
    }

    initPushover(pushoverApiToken, pushoverUserKey, deviceName);
    sendPushover("Hello!! " + ssid + " - Watchdog has just started its watch.");

    initJson(serverJson, deviceName);
    sendJson("String addInfo", 666, "String type", "String requestID");
}

void loop()
{
    unsigned long currentMillis = millis();
    String wifiStatus = wifiConnectionStatus();

    if (currentPhase == 0)
    {
        if (ips[currentHostIndex] == "")
        {
            oledBuffer[currentHostIndex] = "-  | ---";
            updateOLED("SSID: " + String(ssid), wifiStatus + " dBm=" + String(WiFi.RSSI()),
                       oledBuffer[0], oledBuffer[1], oledBuffer[2], oledBuffer[3], oledBuffer[4], oledBuffer[5]);
        }
        else
        {
            bool ret = false;
            if (ipsPort[currentHostIndex] == "")
            {
                oledBuffer[currentHostIndex] = "\x10  |" + ips[currentHostIndex];
                updateOLED("SSID: " + String(ssid), wifiStatus + " dBm=" + String(WiFi.RSSI()),
                           oledBuffer[0], oledBuffer[1], oledBuffer[2], oledBuffer[3], oledBuffer[4], oledBuffer[5]);
                ret = Ping.ping(ips[currentHostIndex].c_str());
            }
            else
            {
                oledBuffer[currentHostIndex] = ("\x10  |" + ips[currentHostIndex] + ":" + ipsPort[currentHostIndex]).substring(0, 22);
                updateOLED("SSID: " + String(ssid), wifiStatus + " dBm=" + String(WiFi.RSSI()),
                           oledBuffer[0], oledBuffer[1], oledBuffer[2], oledBuffer[3], oledBuffer[4], oledBuffer[5]);
                WiFiClient client;
                ret = client.connect(ips[currentHostIndex], ipsPort[currentHostIndex].toInt());
                client.stop();
            }

            if (ret == false)
            {
                if (pingFailCounter[currentHostIndex] < 99)
                {
                    pingFailCounter[currentHostIndex]++;
                    iPstatus = String(pingFailCounter[currentHostIndex]);
                    while (iPstatus.length() < 3)
                    {
                        iPstatus = iPstatus + " ";
                    }
                }
                else
                {
                    iPstatus = "OFF";
                }
                if (pingFailCounter[currentHostIndex] == 5 || pingFailCounter[currentHostIndex] == 50 || pingFailCounter[currentHostIndex] == 99)
                {
                    sendPushover(ipsName[currentHostIndex] + " - " + ips[currentHostIndex] + " - connection error! Attempt: " + String(pingFailCounter[currentHostIndex]));
                }
            }
            else
            {
                iPstatus = "OK ";
                if (pingFailCounter[currentHostIndex] >= 5)
                {
                    sendPushover(ipsName[currentHostIndex] + " - " + ips[currentHostIndex] + " - back online!");
                }
                pingFailCounter[currentHostIndex] = 0;
            }
            oledBuffer[currentHostIndex] = iPstatus + "|" + ipsName[currentHostIndex];
        }
        currentPhase = 1;
    }

    else if (currentPhase == 1)
    {
        if (currentMillis - previousMillis >= 4000)
        {
            updateOLED("SSID: " + String(ssid), wifiStatus + " dBm=" + String(WiFi.RSSI()),
                       oledBuffer[0], oledBuffer[1], oledBuffer[2], oledBuffer[3], oledBuffer[4], oledBuffer[5]);
            previousMillis = currentMillis;
            currentPhase = 2;
        }
    }

    else if (currentPhase == 2)
    {
        if (currentMillis - previousMillis >= 4000)
        {
            currentHostIndex++;
            if (currentHostIndex >= 6)
                currentHostIndex = 0;
            previousMillis = millis();
            currentPhase = 0;
        }
    }
}