#ifndef JSON_H
#define JSON_H

String _jsonServerUrl = "";
String _jsonDeviceName = "Watchdog";

void initJson(String serverUrl, String deviceName)
{
    _jsonServerUrl = serverUrl;
    _jsonDeviceName = deviceName;
}

void sendJson(String addInfo, int value, String type, String requestID = "")
{
    if (_jsonServerUrl == "")
    {
        Serial.println("No endpoint address");
        return;
    }

    WiFiClient client;
    HTTPClient http;
    http.begin(client, _jsonServerUrl);
    http.addHeader("Content-Type", "application/json");
    String jsonString = "{\"deviceIP\":\"" + WiFi.localIP().toString() +
                        "\",\"deviceName\":\"" + _jsonDeviceName +
                        "\",\"requestID\":\"" + requestID +
                        "\",\"addInfo\":\"" + addInfo +
                        "\",\"type\":\"" + type +
                        "\",\"value\":" + String(value) + "}";
    Serial.println("Json to sent: " + jsonString);
    if (http.POST(jsonString) == -1)
    {
        Serial.println("JSON sending error: " + jsonString);
    }
    else
    {
        Serial.println("JSON sent successfully: " + jsonString);
    }
    http.end();
}
#endif // JSON_H