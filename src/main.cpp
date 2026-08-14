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
ElevenLabs elevenLabs(elevenLabsApiKey, elevenLabsVoiceId, elevenLabsModelId, elevenLabsOutPutFormat);
AudioOutputI2S audioOutI2S;

String weatherData;
String weatherDesc;
AudioBuffer audio;

Adafruit_NeoPixel led(1, Pins::LED, NEO_GRB + NEO_KHZ800);

enum class State{
    INIT,
    WEATHER,
    LLM,
    FETCH_AUDIO,
    PLAY_AUDIO,
    DONE,
    ERROR
};

State currentState = State::INIT;

void updateLed(State state)
{
    switch (state)
    {
    case State::INIT:
        led.setPixelColor(0, 0, 0, 0);
        break;
    case State::WEATHER:
        led.setPixelColor(0, 0, 255, 0);
        break;
    case State::LLM:
         led.setPixelColor(0, 0, 0, 255);
        break;
    case State::FETCH_AUDIO:
        led.setPixelColor(0, 255, 255, 0);
        break;
    case State::PLAY_AUDIO:
        led.setPixelColor(0, 255, 0, 255);
        break;
    case State::DONE:
        led.setPixelColor(0, 255, 255, 255);
        break;
    case State::ERROR:
        led.setPixelColor(0, 255, 0, 0);
        break;
    }
    led.show();
}

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    audioOutI2S.SetPinout(Pins::BCLK, Pins::LRC, Pins::DIN);
    audioOutI2S.begin();

    led.begin();
    led.setBrightness(20);

    int cnt = 0;
    while (WiFi.status() != WL_CONNECTED && cnt < 10)
    {
        LOG_DEBUG(String("Trying to connect to WiFi, try ") << cnt);
        delay(500);
        cnt++;
    }
    LOG_INFO("WiFi connected");
}

void loop()
{
    switch (currentState)
    {
    case State::INIT:
        currentState = State::WEATHER;
        break;
    case State::WEATHER:
        weatherData = openMeteo.getTodayWeatherData();
        if(weatherData.isEmpty()){
            LOG_ERROR("No weather data received!");
            currentState=State::ERROR;
            break;
        }
        currentState = State::LLM;
        break;
    case State::LLM:
    {
        String errDesc = "";
        weatherDesc = llm.askLLM(userInstruction + weatherData, errDesc);
        LOG_DEBUG(weatherDesc);
        if(errDesc != ""){
            LOG_ERROR(errDesc);
            currentState = State::ERROR;
            break;
        }
        currentState=State::FETCH_AUDIO;
    }
        break;
    case State::FETCH_AUDIO:
        if(audio.data != nullptr){
            free(audio.data);
        }

        audio = elevenLabs.getSpeechAudio(weatherDesc);

        if(audio.data != nullptr && audio.size > 0){
            currentState = State::PLAY_AUDIO;
        }
        else{
            LOG_ERROR("Nie udało sie wygenerowac dzwieku");
            currentState = State::ERROR;
            break;
        }
        break;
    case State::PLAY_AUDIO:
    {
        AudioFileSourceRAM audioMp3(audio.data, audio.size);
        AudioGeneratorMP3 mp3Conventer;
        mp3Conventer.begin(&audioMp3, &audioOutI2S);

        while(mp3Conventer.isRunning()){
            mp3Conventer.loop();
            yield();
        }

        audioOutI2S.flush();
        mp3Conventer.stop();

        currentState = State::DONE;
        break;
    }
    case State::DONE:
        break;

    case State::ERROR:
        break;
    }

    updateLed(currentState);
}
