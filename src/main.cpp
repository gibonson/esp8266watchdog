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
int countdownTimer = 0;           // Licznik koncowy

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

        if (clients > 0)
        {
            // Ktoś się podłączył! Trwale wyłączamy odliczanie.
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

    updateOLED("ESP8266", "watchdog v0.5", "\x10", "", "\x07", "", "", "");
}

void loop()
{

    unsigned long currentMillis = millis();
    String wifiStatus = wifiConnectionStatus();

    if (currentPhase == 0)
    {
        oledBuffer[currentHostIndex] = "\x10  | " + ipsName[currentHostIndex];
        updateOLED("SSID: " + String(ssid), wifiStatus + " dBm=" + String(WiFi.RSSI()),
                   oledBuffer[0], oledBuffer[1], oledBuffer[2], oledBuffer[3], oledBuffer[4], oledBuffer[5]);

        previousMillis = currentMillis; // Zapisz czas
        currentPhase = 1;               // Przejdź do Fazy 1
    }

    else if (currentPhase == 1)
    {
        // Sprawdzamy, czy minęło już 2000 ms od Fazy 0
        if (currentMillis - previousMillis >= 2000)
        {
            if (ipsPort[currentHostIndex] == "")
            {
                oledBuffer[currentHostIndex] = "\x10  |" + ips[currentHostIndex];
            }
            else
            {
                String adresPort = ips[currentHostIndex] + ":" + ipsPort[currentHostIndex];
                oledBuffer[currentHostIndex] = "\x10  |" + adresPort.substring(0, 16);
            }
            updateOLED("SSID: " + String(ssid), wifiStatus + " dBm=" + String(WiFi.RSSI()),
                       oledBuffer[0], oledBuffer[1], oledBuffer[2], oledBuffer[3], oledBuffer[4], oledBuffer[5]);

            previousMillis = currentMillis; // Znów resetujemy zegar
            currentPhase = 2;               // Przejdź do Fazy 2
        }
    }

    // --- FAZA 2: Wykonaj Ping/TCP po kolejnych 2 sekundach ---
    else if (currentPhase == 2)
    {
        // Sprawdzamy, czy minęło 2000 ms od Fazy 1
        if (currentMillis - previousMillis >= 2000)
        {
            if (ips[currentHostIndex] != "")
            {
                bool ret = false;

                // UWAGA: Funkcje Ping i Connect są tutaj jedynymi elementami naturalnie blokującymi procesor
                // na czas oczekiwania na sieć. Z tym nic nie zrobimy bez pisania własnych bibliotek asynchronicznych.
                if (ipsPort[currentHostIndex] == "")
                {
                    ret = Ping.ping(ips[currentHostIndex].c_str());
                }
                else
                {
                    WiFiClient client;
                    ret = client.connect(ips[currentHostIndex], ipsPort[currentHostIndex].toInt());
                    client.stop();
                }

                if (ret == false)
                {
                    if (pingFailCounter[currentHostIndex] < 99)
                    {
                        pingFailCounter[currentHostIndex]++;
                        if (pingFailCounter[currentHostIndex] == 5 || pingFailCounter[currentHostIndex] == 50 || pingFailCounter[currentHostIndex] == 99)
                        {
                            sendPushover(ipsName[currentHostIndex] + " - " + ips[currentHostIndex] + " - connection error! Attempt: " + String(pingFailCounter[currentHostIndex]));
                        }
                    }
                }
                else
                {
                    if (pingFailCounter[currentHostIndex] >= 5)
                    {
                        sendPushover(ipsName[currentHostIndex] + " - " + ips[currentHostIndex] + " - back online!");
                    }
                    pingFailCounter[currentHostIndex] = 0;
                }

                // Aktualizacja statusu dla następnego cyklu wyświetlania
                if (pingFailCounter[currentHostIndex] == 0)
                {
                    iPstatus = "OK ";
                }
                else if (pingFailCounter[currentHostIndex] == 99)
                {
                    iPstatus = "OFF";
                }
                else
                {
                    iPstatus = String(pingFailCounter[currentHostIndex]);
                    while (iPstatus.length() < 3)
                    {
                        iPstatus = iPstatus + " ";
                    }
                }
                oledBuffer[currentHostIndex] = iPstatus + "| " + ipsName[currentHostIndex];
            }
            else
            {
                oledBuffer[currentHostIndex] = "-  | -";
            }

            // Krok końcowy: Przesuwamy się na kolejnego hosta i wracamy do Fazy 0
            currentHostIndex++;
            if (currentHostIndex >= 6)
            {
                currentHostIndex = 0; // Wracamy do początku tablicy

                currentPhase = 2;   // Faza 2 wykona się od razu w następnym przebiegu loop() i sama ustawi zegar
                countdownTimer = 5; // Ustawiamy timer na 5 cykli w kolejnej fazie
                previousMillis = millis();
            }
            else
            {
                currentPhase = 0; // Wracamy do Fazy 0 dla kolejnego hosta
            }
        }
    }
}