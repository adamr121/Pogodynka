#include <Arduino.h>
#include <WiFi.h>
#include <config.h>
#include "Gemini.h"
#include "OpenMeteo.h"

#include "logger.h"

Gemini llm(model, geminiApiKey);
OpenMeteo openMeteo;


void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int cnt = 0;
    while (WiFi.status() != WL_CONNECTED && cnt < 10)
    {
        LOG_DEBUG(String("Trying to connect to WiFi, try ") << cnt);
        delay(500);
        cnt++;
    }
    LOG_INFO("WiFi connected");

    String weatherDesc = llm.askLLM(userInstruction + openMeteo.getTodayWeatherData());
    LOG_DEBUG(weatherDesc);
}

void loop()
{
}
