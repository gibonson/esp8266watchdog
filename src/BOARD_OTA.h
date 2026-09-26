#ifndef BOARD_OTA_H
#define BOARD_OTA_H

#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

String checkForUpdates(String VERSION_JSON_URL, String FIRMWARE_VERSION)
{
  Serial.println("Checking for updates...");

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  // 1. DOWNLOAD VERSION.JSON FILE
  if (https.begin(client, VERSION_JSON_URL))
  {
    int httpCode = https.GET();

    if (httpCode == HTTP_CODE_OK)
    {
      String payload = https.getString();

      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);

      if (error)
      {
        Serial.println("JSON parsing error");
        https.end();
        return "JSON parsing error";
      }

      String latest_version = doc["version"].as<String>();
      String bin_url = doc["bin_url"].as<String>();

      Serial.printf("Current version: %s, Latest version: %s\n", FIRMWARE_VERSION.c_str(), latest_version.c_str());

      // 2. COMPARE VERSIONS
      if (latest_version != FIRMWARE_VERSION)
      {
        Serial.println("New version found! Starting download...");
        ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

        // 3. Update
        t_httpUpdate_return ret = ESPhttpUpdate.update(client, bin_url);

        switch (ret)
        {
        case HTTP_UPDATE_FAILED:
          Serial.printf("Update failed (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          https.end();
          return "Update failed";
        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("No updates available.");
          https.end();
          return "No updates available.";
        case HTTP_UPDATE_OK:
          Serial.println("Update completed!"); // ESP zrestartuje się samo przed tą linią
          https.end();
          return "Update completed!";
        default:
          return "Unknown update status";
        }
      }
      else
      {
        Serial.println("You are running the latest firmware version.");
        https.end();
        return "Version - OK";
      }
    }
    else
    {
      Serial.printf("Failed to download JSON file. HTTP code: %d\n", httpCode);
      https.end();
      return "Failed to download JSON.";
    }
  }
  else
  {
    Serial.println("Failed to connect to the server.");
    https.end();
    return "Failed to connect.";
  }
}

#endif // BOARD_OTA_H