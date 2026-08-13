void sendPushover(String message)
{
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    http.begin(client, "https://api.pushover.net/1/messages.json");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // 1. Kodujemy tytuł (zastępujemy spacje ręcznie)
    String encodedTitle = ssid + "%20-%20ESP8266%20-%20" + deviceName;

    // 2. Proste kodowanie wiadomości (zamiana spacji na %20 i nowej linii na %0A)
    String encodedMessage = message;
    encodedMessage.replace(" ", "%20");
    encodedMessage.replace("\n", "%0A");

    // 3. Budujemy poprawne zapytanie
    String httpRequestData = "token=" + pushoverApiToken +
                             "&user=" + pushoverUserKey +
                             "&title=" + encodedTitle +
                             "&message=" + encodedMessage;

    Serial.println(httpRequestData);
    Serial.println("Wysyłanie powiadomienia...");

    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0)
    {
        Serial.print("Sukces! Kod odpowiedzi HTTP: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println("Odpowiedź serwera: " + payload);
    }
    else
    {
        Serial.print("Błąd wysyłania. Kod błędu: ");
        Serial.println(httpResponseCode);
    }
    http.end();
}