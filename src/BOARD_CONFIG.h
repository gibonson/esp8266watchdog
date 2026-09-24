#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <LittleFS.h>
#include <ESP8266WiFi.h> // Wymagane dla typu IPAddress


// --- WI-FI Configuration---
String ssid = "";
String password = "";
IPAddress local_IP(192, 168, 0, 199); // Adres ESP


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


bool stringToIP(const String &str, IPAddress &ip)
{
    return ip.fromString(str);
}

void initLittleFS()
{
    if (!LittleFS.begin())
    {
        Serial.println(F("Blad montowania LittleFS"));
    }
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

String readRawConfig()
{
    String content = "";
    File f = LittleFS.open("/config.txt", "r");
    if (f)
    {
        content = f.readString();
        f.close();
    }
    else
    {
        content = "Nazwa_WiFi\nHaslo_WiFi\n192.168.0.199\nUserKey_Pushover\nApiToken_Pushover\nhttp://192.168.0.242:5000/api/addEvent\n192.168.0.10;Serwer\n192.168.0.11;Kamera\n\n\n\n\n";
    }
    return content;
}

bool saveRawConfig(const String &rawData)
{
    File f = LittleFS.open("/config.txt", "w");
    if (!f)
        return false; // Zwracamy false jeśli błąd zapisu

    f.print(rawData);
    f.close();
    return true; // Zapis udany
}

#endif // BOARD_CONFIG_H