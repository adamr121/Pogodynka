#include <Arduino.h>
#include <WiFi.h>
#include <config.h>
#include "Gemini.h"
#include "OpenMeteo.h"
#include "ElevenLabs.h"

#include "logger.h"

Gemini llm(model, geminiApiKey);
OpenMeteo openMeteo;
ElevenLabs elevenLabs(elevenLabsApiKey, elevenLabsVoiceId);

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

    String weatherData = openMeteo.getTodayWeatherData();
    if(weatherData.isEmpty()){
        LOG_ERROR("No weather data received!");
        return;
    }
    String weatherDesc = llm.askLLM(userInstruction + weatherData );
    LOG_DEBUG(weatherDesc);

    // AudioBuffer audio = elevenLabs.getSpeechAudio(weatherDesc);

    // if(audio.data != nullptr && audio.size > 0){
    //     // odtworz audio



    //     free(audio.data);
    //     LOG_DEBUG("Audio in RAM cleared");
    // }

}

void loop()
{
}
