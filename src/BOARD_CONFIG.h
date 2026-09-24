#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <LittleFS.h>
#include <ESP8266WiFi.h> // Wymagane dla typu IPAddress
#include <ArduinoJson.h>

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
String ips[6] = {"", "", "", "", "", ""};
String ipsPort[6] = {"", "", "", "", "", ""};
String ipsName[6] = {"", "", "", "", "", ""};

// --- Config template ---
const String defaultJsonTemplate = "{\n"
                  "  \"wifi\": {\n"
                  "    \"ssid\": \"\",\n"
                  "    \"password\": \"\",\n"
                  "    \"localIp\": \"192.168.0.199\"\n"
                  "  },\n"
                  "  \"pushover\": {\n"
                  "    \"userKey\": \"\",\n"
                  "    \"apiToken\": \"\"\n"
                  "  },\n"
                  "  \"serverJson\": \"http://192.168.0.242:5000/api/addEvent\",\n"
                  "  \"deviceList\": [\n"
                  "    {\n"
                  "      \"name\": \"Router\",\n"
                  "      \"ip\": \"192.168.0.1\"\n"
                  "    },\n"
                  "    {\n"
                  "      \"name\": \"Google DNS (Test)\",\n"
                  "      \"ip\": \"8.8.8.8\"\n"
                  "    }\n"
                  "  ]\n"
                  "}";

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

bool saveRawConfig(const String &rawData)
{
    File f = LittleFS.open("/config.json", "w");
    if (!f)
        return false; // Zwracamy false jeśli błąd zapisu

    f.print(rawData);
    f.close();
    return true; // Zapis udany
}


String readRawConfig()
{
    if (!LittleFS.exists("/config.json")) {
        Serial.println(F("Brak pliku /config.json - ładuję szablon."));
        saveRawConfig(defaultJsonTemplate); // <--- Zapis szablonu do LittleFS
        return defaultJsonTemplate;
    }

    String content = "";
    File f = LittleFS.open("/config.json", "r");
    if (f)
    {
        content = f.readString();
        f.close();
    }

    DynamicJsonDocument testDoc(1024);
    DeserializationError error = deserializeJson(testDoc, content);
    
    if (error || content.length() == 0) {
        Serial.println(F("Plik config.json jest uszkodzony lub pusty. Zwracam szablon."));
        saveRawConfig(defaultJsonTemplate); // <--- Zapis szablonu do LittleFS
        return defaultJsonTemplate;
    }

    return content; // Jeśli wszystko jest OK, zwracamy zawartość pliku
}

void loadConfiguration()
{
    // ZMIANA 3: loadConfiguration nie dotyka LittleFS, prosi o dane funkcję readRawConfig()
    String rawConfig = readRawConfig();

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, rawConfig);

    if (error) {
        // Ten błąd praktycznie nie powinien się zdarzyć (bo readRawConfig go już odrzuca), ale zostawiamy dla bezpieczeństwa
        Serial.print(F("Błąd parsowania JSON w loadConfiguration: "));
        Serial.println(error.c_str());
        return;
    }

    // Odczyt sekcji WiFi
    ssid = doc["wifi"]["ssid"] | "";
    password = doc["wifi"]["password"] | "";
    String ipStr = doc["wifi"]["localIp"] | "";
    if (ipStr != "") stringToIP(ipStr, local_IP);

    // Odczyt sekcji Pushover
    pushoverUserKey = doc["pushover"]["userKey"] | "";
    pushoverApiToken = doc["pushover"]["apiToken"] | "";

    // Odczyt Endpointu
    serverJson = doc["serverJson"] | "";

    // Odczyt tablicy hostów (deviceList)
    JsonArray deviceList = doc["deviceList"].as<JsonArray>();
    
    int i = 0;
    for (JsonObject device : deviceList) 
    {
        if (i >= 6) break; 
        
        ipsName[i] = device["name"] | "-";
        
        // ZMIANA 1: Zapisujemy tekst bezpośrednio, bez stringToIP
        ips[i] = device["ip"] | ""; 
        ipsPort[i] = device["port"] | ""; 
        
        i++;
    }
    
    // Czyszczenie "resztek" w tablicy
    for (int j = i; j < 6; j++) {
        ips[j] = "";
        ipsName[j] = "-";
        ipsPort[j] = "";
    }

    Serial.println(F("Konfiguracja zaktualizowana i załadowana do zmiennych!"));
}





#endif // BOARD_CONFIG_H