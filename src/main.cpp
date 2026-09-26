#include <Arduino.h>
#include <ESP8266Ping.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>

#include "MODULE_OLED.h"

#include "BOARD_CONFIG.h"
#include "BOARD_WIFI.h"
#include "BOARD_WEB_GUI.h"
#include "BOARD_OTA.h"
#include "BOARD_JSON.h"
#include "BOARD_PUSHOVER.h"

String deviceName = "ESP-WatchDog";
String appVersion = "0.9";

int pingFailCounter[6] = {0, 0, 0, 0, 0, 0};
String oledBuffer[7] = {"", "", "", "", "", "", ""};

unsigned long previousMillis = 0;
int currentHostIndex = 0;
int currentPhase = 0;

const char *accessPointName = "ESP-Configuration";
const char *accessPointPassword = "12345678";
bool configSaved = false;

void setup()
{

    Serial.begin(115200);
    initOLED();
    updateOLED(deviceName + " " + appVersion, "", "  (co tu sie      )", "   \\  odpierdala_/", "    \\/", "     |\\__/,|   ( \\", "   _.|o o  |_   ) )", " -(((---(((--------");
    delay(3000);

    initLittleFS();
    loadConfiguration();

    String apIP = initAccessPoint(accessPointName, accessPointPassword);
    initWebGUI();
    updateOLED("CONFIG MODE", "AP Started!", "", "Go to IP:", apIP, "Waiting for Config...", "", "");

    // Configuration Mode
    unsigned long apStartTime = millis();
    int lastSecondsLeft = -1;
    bool timerActive = true;

    while (!configSaved)
    {
        server.handleClient();

        int clients = WiFi.softAPgetStationNum();

        if (clients > 0) // if client deactivate timer
        {
            timerActive = false;
            updateOLED("CONFIG MODE", "", "AP: " + String(accessPointName), "PASS: " + String(accessPointPassword), "Go to IP:", WiFi.softAPIP().toString(), "Waitig for Config", "");
        }
        else if (timerActive)
        {
            int secondsLeft = 10 - ((millis() - apStartTime) / 1000);

            if (secondsLeft != lastSecondsLeft)
            {
                updateOLED("CONFIG MODE", "", "AP: " + String(accessPointName), "PASS: " + String(accessPointPassword), "", "Waiting for client", "Starting in: " + String(secondsLeft) + "s", "");
                lastSecondsLeft = secondsLeft;
            }
            if (secondsLeft <= 0)
            {
                Serial.println("no clients. Starting normally.");
                break;
            }
        }
        else

            yield();
    }

    server.stop();
    WiFi.softAPdisconnect(true);

    updateOLED("SSID: " + String(ssid), "", "  (Connecting to  )", "   \\  the WIFI  _/", "    \\/", "     |\\__/,|   ( \\", "   _.|o o  |_   ) )", " -(((---(((--------");

    initWifiClient(ssid, password);

    updateOLED("SSID: " + String(ssid), "", "  (Okay, I\'m .    )", "   \\  ONLINE!!  _/", "    \\/", "     |\\__/,|   ( \\", "   _.|o o  |_   ) )", " -(((---(((--------");
    initPushover(pushoverApiToken, pushoverUserKey, deviceName + " " + appVersion);
    sendPushover("Watchdog has just started its watch.");

    initJson(serverJson, deviceName);
    sendJson("String addInfo", 666, "String type", "String requestID");

    updateOLED("SSID: " + String(ssid), "FIRMWARE UPDATE", "Version: " + appVersion, "OTA Server:", serverOtaVersion.substring(0, 20), "Status:", "...", "");
    delay(3000);

    String otaStatus = checkForUpdates(serverOtaVersion, appVersion);
    updateOLED("SSID: " + String(ssid), "FIRMWARE UPDATE", "Version: " + appVersion, "OTA Server:", serverOtaVersion.substring(0, 20), "Status:", otaStatus, "");
    delay(3000);
}

void loop()
{
    unsigned long currentMillis = millis();

    if (currentPhase == 0)
    {
        String wifiStatus = wifiConnectionStatus();
        if (ips[currentHostIndex] == "")
        {
            oledBuffer[currentHostIndex] = "-  | ---";
            updateOLED("SSID: " + String(ssid), wifiStatus + " dBm=" + String(WiFi.RSSI()),
                       oledBuffer[0], oledBuffer[1], oledBuffer[2], oledBuffer[3], oledBuffer[4], oledBuffer[5]);
        }
        else
        {
            bool ret = false;
            String IPstatus = "  ";

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
                    IPstatus = String(pingFailCounter[currentHostIndex]);
                    while (IPstatus.length() < 3)
                    {
                        IPstatus = IPstatus + " ";
                    }
                }
                else
                {
                    IPstatus = "OFF";
                }
                if (pingFailCounter[currentHostIndex] == 5 || pingFailCounter[currentHostIndex] == 50 || pingFailCounter[currentHostIndex] == 99)
                {
                    sendPushover(ipsName[currentHostIndex] + " - " + ips[currentHostIndex] + " - connection error! Attempt: " + String(pingFailCounter[currentHostIndex]));
                }
            }
            else
            {
                IPstatus = "OK ";
                if (pingFailCounter[currentHostIndex] >= 5)
                {
                    sendPushover(ipsName[currentHostIndex] + " - " + ips[currentHostIndex] + " - back online!");
                }
                pingFailCounter[currentHostIndex] = 0;
            }
            oledBuffer[currentHostIndex] = IPstatus + "|" + ipsName[currentHostIndex];
        }
        currentPhase = 1;
    }

    else if (currentPhase == 1)
    {
        if (currentMillis - previousMillis >= 4000)
        {
            String wifiStatus = wifiConnectionStatus();
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