#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>
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
#include <EasyButton.h>
#include "Timer.h"

Gemini llm(model, geminiApiKey);
OpenMeteo openMeteo;
ElevenLabs elevenLabs(elevenLabsApiKey, elevenLabsVoiceId, elevenLabsModelId, elevenLabsOutPutFormat);
AudioOutputI2S audioOutI2S;

String weatherData;
String weatherDesc;
AudioBuffer audio;

Adafruit_NeoPixel led(1, Pins::LED, NEO_GRB + NEO_KHZ800);
EasyButton btn (Pins::Button);

Timer wifiTimer(500);

enum class State{
    INIT,
    WEATHER,
    LLM,
    FETCH_AUDIO,
    PLAY_AUDIO,
    DONE,
    ERROR,
    WiFi_CONNECTION
};

State currentState;

// --- Deep sleep ---
unsigned long terminalStateSince = 0; // millis() w momencie wejścia w DONE/ERROR
bool terminalStateActive = false;     // czy obecnie przebywamy w DONE/ERROR

void goToDeepSleep()
{
    LOG_INFO("Wchodzę w deep sleep (bezczynność w DONE/ERROR)");
    led.setPixelColor(0,0,0,0);
    led.show();

    // Wyłącz wzmacniacz audio (MAX98357A) – SD w stan niski = shutdown
    pinMode(Pins::SD_MODE, OUTPUT);
    digitalWrite(Pins::SD_MODE, LOW);

    // Wyłącz WiFi, żeby oszczędzać energię w sleep
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);

    // Budzenie przyciskiem – stan niski po wciśnięciu
#ifdef ESP_C3_MINI
    esp_deep_sleep_enable_gpio_wakeup(1ULL << Pins::Button, ESP_GPIO_WAKEUP_GPIO_LOW);
#else
    esp_sleep_enable_ext0_wakeup((gpio_num_t) Pins::Button, LOW);
#endif
    // Deep sleep resetuje układ – po obudzeniu setup() wykona się od nowa
    esp_deep_sleep_start();
}

void updateLed(State state)
{
    switch (state)
    {
    case State::INIT:
        led.setPixelColor(0, 255, 50, 0); 
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
    case State::WiFi_CONNECTION:
        led.setPixelColor(0, 255, 125, 1);
        break;
    }
    led.show();
}

const char* stateToString(State state)
{
    switch (state)
    {
    case State::INIT:        return "INIT";
    case State::WEATHER:     return "WEATHER";
    case State::LLM:         return "LLM";
    case State::FETCH_AUDIO: return "FETCH_AUDIO";
    case State::PLAY_AUDIO:  return "PLAY_AUDIO";
    case State::DONE:        return "DONE";
    case State::ERROR:       return "ERROR";
    case State::WiFi_CONNECTION:       return "WiFi_CONNECTION";
    }
    return "UNKNOWN";
}

void setState(State newState)
{
    if (currentState != newState)
    {
        LOG_DEBUG("Zmiana stanu: " << stateToString(currentState) << " -> " << stateToString(newState));
        currentState = newState;
        updateLed(currentState); // natychmiast ustaw kolor dla nowego stanu

        switch (currentState)
        {
        case State::WiFi_CONNECTION:
            WiFi.disconnect();
            WiFi.begin(ssid, password);
            break;
        }
    }
}

void btnOnPressed(){
    switch (currentState)
    {
    case State::INIT:
        setState(State::WEATHER);
        break;
    case State::DONE:
        setState(State::PLAY_AUDIO);
        break;
    case State::ERROR:
        if(WiFi.status() == WL_CONNECTED){
            setState(State::INIT);
        }
        else{
            setState(State::WiFi_CONNECTION);
        }
        break;
    }
}
void onWiFi(arduino_event_id_t event, arduino_event_info_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        LOG_WARN("Rozlaczenie, reason=" << (int)info.wifi_sta_disconnected.reason
                 << " (" << WiFi.disconnectReasonName((wifi_err_reason_t)info.wifi_sta_disconnected.reason) << "), RSSI = " << WiFi.RSSI());
    }
}
void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(ssid, password);

    audioOutI2S.SetPinout(Pins::BCLK, Pins::LRC, Pins::DIN);
    audioOutI2S.begin();

    led.begin();
    led.setBrightness(20);
    led.setPixelColor(0, 0, 0, 0); 
    led.show();
    btn.begin();
    btn.onPressed(btnOnPressed);
    
    WiFi.onEvent(onWiFi);
    setState(State::WiFi_CONNECTION);
}


void loop()
{
    btn.read();

    switch (currentState)
    {
    case State::WiFi_CONNECTION:
        static int timeoutTicks = 0;
        if(wifiTimer.isReady())
        {
            if (WiFi.status() != WL_CONNECTED)
            {
                timeoutTicks++;
                LOG_DEBUG(String("Trying to connect to WiFi, try ") << timeoutTicks);
            }
            if (WiFi.status() == WL_CONNECTED)
            {
                timeoutTicks = 0;
                LOG_INFO("Nawiazano polaczenie z WIFI " << WiFi.localIP());
                LOG_DEBUG("WiFI RSSI = " << WiFi.RSSI() << ", MAC = " << WiFi.macAddress());
                setState(State::INIT);
            }
            if (timeoutTicks >= 10)
            {
                timeoutTicks = 0;
                setState(State::ERROR);
            }
        }
        break;

    case State::INIT:
    {
        setState(State::WEATHER);
        break;
    }
    case State::WEATHER:
        weatherData = openMeteo.getTodayWeatherData();
        if(weatherData.isEmpty()){
            LOG_ERROR("No weather data received!");
            setState(State::ERROR);
            break;
        }
        setState(State::LLM);
        break;
    case State::LLM:
    {
        String errDesc = "";
        weatherDesc = llm.askLLM(userInstruction + weatherData, errDesc);
        LOG_DEBUG(weatherDesc);
        if(errDesc != ""){
            LOG_ERROR(errDesc);
            setState(State::ERROR);
            break;
        }
        setState(State::FETCH_AUDIO);
    }
        break;
    case State::FETCH_AUDIO:
        if(audio.data != nullptr){
            free(audio.data);
        }

        audio = elevenLabs.getSpeechAudio(weatherDesc);

        if(audio.data != nullptr && audio.size > 0){
            setState(State::PLAY_AUDIO);
        }
        else{
            LOG_ERROR("Nie udało sie wygenerowac dzwieku");
            setState(State::ERROR);
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

        setState(State::DONE);
        break;
    }
    case State::DONE:
        break;

    case State::ERROR:
        break;
    }

    // Deep sleep: jeśli system jest w stanie DONE lub ERROR przez DEEP_SLEEP_TIMEOUT_MS
    if (currentState == State::DONE || currentState == State::ERROR)
    {
        if (!terminalStateActive)
        {
            terminalStateActive = true;
            terminalStateSince = millis();
        }
        else if (millis() - terminalStateSince >= DEEP_SLEEP_TIMEOUT_MS)
        {
            goToDeepSleep();
        }
    }
    else
    {
        // Opuściliśmy stan końcowy – reset licznika bezczynności
        terminalStateActive = false;
    }
}
