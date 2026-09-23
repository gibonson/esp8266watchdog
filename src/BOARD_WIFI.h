#ifndef BOARD_WIFI_H
#define BOARD_WIFI_H

#include <ESP8266WiFi.h>

IPAddress local_IP(192, 168, 0, 199); // Adres ESP
IPAddress gateway(192, 168, 0, 1);    // Brama (router)
IPAddress subnet(255, 255, 255, 0);   // Maska
IPAddress primaryDNS(8, 8, 8, 8);     // Serwer DNS od Google

String initAccessPoint(String apName, String apPassword)
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName, apPassword);

    String ipAddress = WiFi.softAPIP().toString();

    Serial.print(F("AP Started. IP: "));
    Serial.println(ipAddress);

    return ipAddress;
}

void stopAccessPoint()
{
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    Serial.println(F("AP Stopped. Mode set to STA."));
}

bool initWifiClient(String ssid, String password)
{
    WiFi.mode(WIFI_STA); // wifi - client mode

    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS))
    {
        Serial.println("IP configuration Error!!!");
    }

    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("\nConnecting to Wi-Fi");
    Serial.print(ssid);

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
            return false;
        }
    }

    Serial.println("\nWiFi Connected:");
    Serial.println("Adres IP: " + String(WiFi.localIP().toString()));
    Serial.println("RSSI: " + String(WiFi.RSSI()));
    return true;
}

String wifiConnectionStatus()
{
    String status = "";
    switch (WiFi.status())
    {
    case WL_CONNECTED:
        status = "CONNECTED";
        break;
    case WL_NO_SSID_AVAIL:
        status = "NO SSID";
        break;
    case WL_CONNECT_FAILED:
        status = "CONN FAILED";
        break;
    case WL_CONNECTION_LOST:
        status = "CONN LOST";
        break;
    case WL_DISCONNECTED:
        status = "DISCONN";
        break;
    default:
        status = "UNKNOWN";
        break;
    }
    return status;
}

#endif // BOARD_WIFI_H
