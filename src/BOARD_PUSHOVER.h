#ifndef PUSHOVER_H
#define PUSHOVER_H

String _pushoverToken = "";
String _pushoverUser = "";
String _pushoverDeviceName = "";

void initPushover(String apiToken, String userKey, String deviceName)
{
    _pushoverToken = apiToken;
    _pushoverUser = userKey;
    _pushoverDeviceName = deviceName;
}

String encodeString(String str)
{
    String encodedString = str;
    encodedString.replace(" ", "%20");
    encodedString.replace("\n", "%0A");
    return encodedString;
}

void sendPushover(String message)
{
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    http.begin(client, "https://api.pushover.net/1/messages.json");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String title = WiFi.SSID() + " - " + _pushoverDeviceName;

    String httpRequestData = "token=" + _pushoverToken +
                             "&user=" + _pushoverUser +
                             "&title=" + encodeString(title) +
                             "&message=" + encodeString(message);

    Serial.println("Sending a notification...");

    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0)
    {
        Serial.print("Success! response code: ");
        Serial.println(httpResponseCode);
    }
    else
    {
        Serial.print("Sending error! response code: ");
        Serial.println(httpResponseCode);
    }
    http.end();
}

#endif // PUSHOVER_H