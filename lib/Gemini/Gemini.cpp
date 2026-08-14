#include "Gemini.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <logger.h>

Gemini::Gemini(const char *model, const char *apiKey) : model(model), apiKey(apiKey)
{
}

String Gemini::askLLM(const String &prompt, String &errDesc)
{
    LOG_INFO("Gemini::askLLM()");
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    String url = "https://generativelanguage.googleapis.com/v1beta/models/" + model + ":generateContent?key=" + String(apiKey);
    String result = "";

    if (WiFi.status() != WL_CONNECTED)
    {
        LOG_ERROR("No WiFi connection!");
        return "";
    }

    https.begin(client, url);
    https.addHeader("Content-Type", "application/json");

    JsonDocument docInput;
    JsonObject content = docInput["contents"].add<JsonObject>();
    JsonObject part = content["parts"].add<JsonObject>();
    part["text"] = prompt;

    String jsonPayload;
    serializeJson(docInput, jsonPayload);

    int httpResponseCode = https.POST(jsonPayload);

    if (httpResponseCode > 0)
    {
        String responseBody = https.getString();
        JsonDocument docOutput;
        DeserializationError error = deserializeJson(docOutput, responseBody);
        LOG_DEBUG("Json z odpowiedzia GEMINI: " << responseBody);
        if (!error)
        {
            const char *text = docOutput["candidates"][0]["content"]["parts"][0]["text"];
            if (text)
            {
                result = String(text);
            }
            else
            {
                errDesc = "Błąd: Nie znaleziono pola z tekstem w odpowiedzi.";
            }
        }
        else
        {
            errDesc = "Błąd deserializacji JSON: " + String(error.c_str());
        }
    }
    else
    {
        errDesc = "Błąd HTTP: " + String(httpResponseCode);
    }
    LOG_DEBUG("Odpowiedz GEMINI: " << result);
    https.end();
    return result;
}
