void sendJson(String addInfo, int value, String type, String requestID = "")
{
    WiFiClient client;
    HTTPClient http;
    http.begin(client, serverJson);
    http.addHeader("Content-Type", "application/json");
    String jsonString = "{\"deviceIP\":\"" +
                        String(local_IP[0]) + "." +
                        String(local_IP[1]) + "." +
                        String(local_IP[2]) + "." +
                        String(local_IP[3]) +
                        "\",\"deviceName\":\"" + "dog" +
                        "\",\"requestID\":\"" + requestID +
                        "\",\"addInfo\":\"" + addInfo +
                        "\",\"type\":\"" + type +
                        "\",\"value\":" + String(value) + "}";
    Serial.println("Json to sent: " + jsonString);
    if (http.POST(jsonString) == -1)
    {
        Serial.println("Błąd wysyłania JSON-a: " + jsonString);
    }
    else
    {
        Serial.println("JSON wysłany pomyślnie: " + jsonString);
    }
}
