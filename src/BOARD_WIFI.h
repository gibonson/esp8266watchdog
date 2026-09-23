#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>


ESP8266WebServer server(80);



void initAccessPoint(String accessPointName, String accessPointPassword)
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(accessPointName, accessPointPassword);

    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);

    server.begin();
    updateOLED("CONFIG MODE", "Connected!!", "", "Go to IP:", WiFi.softAPIP().toString(), "Waitig for Config...", "", "");
}

void initWifiClient()
{
    WiFi.mode(WIFI_STA); // wifi - client mode

    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS))
    {
        Serial.println("IP configuration Error!!!");
    }

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
}
