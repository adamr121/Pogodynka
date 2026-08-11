#include <Arduino.h>
#include <WiFi.h>
#include <config.h>
#include <pinout.h>
#include "logger.h"
#include "Gemini.h"
#include "OpenMeteo.h"
#include "ElevenLabs.h"
#include "AudioFileSourceRAM.h"
#include <AudioOutputI2S.h>
#include <AudioGeneratorMP3.h>
#include <Adafruit_NeoPixel.h>

Gemini llm(model, geminiApiKey);
OpenMeteo openMeteo;
ElevenLabs elevenLabs(elevenLabsApiKey, elevenLabsVoiceId);
AudioOutputI2S audioOutI2S;

Adafruit_NeoPixel led(1, Pins::LED, NEO_GRB + NEO_KHZ800);

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    audioOutI2S.SetPinout(Pins::BCLK, Pins::LRC, Pins::DIN);
    audioOutI2S.begin();

    led.begin();
    led.setBrightness(20);
    led.setPixelColor(0, 0, 255, 0);
    led.show();

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
    led.setPixelColor(0, 0, 0, 255);
    led.show();
    String weatherDesc = llm.askLLM(userInstruction + weatherData );
    LOG_DEBUG(weatherDesc);

    led.setPixelColor(0, 255, 255, 0);
    led.show();

    AudioBuffer audio = elevenLabs.getSpeechAudio(weatherDesc);

    if(audio.data != nullptr && audio.size > 0){
        led.setPixelColor(0, 255, 0, 255);
        led.show();
        AudioFileSourceRAM audioMp3(audio.data, audio.size);

        AudioGeneratorMP3 mp3Conventer;
        mp3Conventer.begin(&audioMp3, &audioOutI2S);

        while(mp3Conventer.isRunning()){
            mp3Conventer.loop();
            yield();
        }
        audioOutI2S.flush();
        mp3Conventer.stop();

        free(audio.data);
        LOG_DEBUG("Audio in RAM cleared");
    }
    else{
        led.setPixelColor(0, 255, 0, 0);
        led.show();
    }

}

void loop()
{
}
