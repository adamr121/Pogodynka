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
#include "StateMachine.h"
#include <ArduinoOTA.h>

// --- Pomocnik: liczy całkowitą liczbę próbek audio w buforze MP3 ---
// Potrzebne, bo dekoder (AudioGeneratorMP3) na ESP32-C3 nie kończy sam odtwarzania.
static uint32_t mp3TotalSamples(const uint8_t *data, uint32_t size)
{
    static const int kbpsV1[16] = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0};
    static const int kbpsV2[16] = {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0};
    uint32_t total = 0;
    uint32_t i = 0;
    while (i + 4 <= size) {
        if (data[i] != 0xFF) { i++; continue; }
        uint8_t b1 = data[i+1];
        if ((b1 & 0xE0) != 0xE0) { i++; continue; }   // sync
        int version = (b1 >> 3) & 3;                   // 3=MPEG1, 2=MPEG2, 0=MPEG2.5
        int layer   = (b1 >> 1) & 3;                   // 1 = Layer III
        if (layer != 1) { i++; continue; }
        uint8_t b2 = data[i+2];
        int bitrateIdx = (b2 >> 4) & 0xF;
        int srIdx      = (b2 >> 2) & 3;
        int padding    = (b2 >> 1) & 1;
        if (bitrateIdx == 0 || bitrateIdx == 15 || srIdx == 3) { i++; continue; }

        int sr, bitrate, spf;
        if (version == 3) {
            sr = (srIdx==0)?44100 : (srIdx==1)?48000 : 32000;
            bitrate = kbpsV1[bitrateIdx];
            spf = 1152;
        } else {
            sr = (version==2) ? ((srIdx==0)?22050:(srIdx==1)?24000:16000)
                              : ((srIdx==0)?11025:(srIdx==1)?12000:8000);
            bitrate = kbpsV2[bitrateIdx];
            spf = 576;
        }
        if (bitrate == 0) { i++; continue; }

        int frameLen = (version==3) ? (144 * bitrate * 1000) / sr + padding
                                    : (72  * bitrate * 1000) / sr + padding;
        if (frameLen <= 0) { i++; continue; }
        total += spf;
        i += frameLen;
    }
    return total;
}

Gemini llm(model, geminiApiKey);
OpenMeteo openMeteo;
ElevenLabs elevenLabs(elevenLabsApiKey, elevenLabsVoiceId, elevenLabsModelId, elevenLabsOutPutFormat);
AudioOutputI2S audioOutI2S(0, AudioOutputI2S::EXTERNAL_I2S, 8, AudioOutputI2S::APLL_ENABLE);

String weatherData;
String weatherDesc;
AudioBuffer audio;

Adafruit_NeoPixel led(1, Pins::LED, NEO_GRB + NEO_KHZ800);
EasyButton btn (Pins::Button);

Timer wifiTimer(500);
StateMachine stateMachine;

// --- Deep sleep ---
unsigned long terminalStateSince = 0; // millis() w momencie wejścia w DONE/ERROR
bool terminalStateActive = false;     // czy obecnie przebywamy w DONE/ERROR

void goToDeepSleep()
{
    LOG_INFO("Wchodzę w deep sleep (bezczynność w DONE/ERROR)");
    led.setPixelColor(0,0,0,0);
    led.show();

    // Wyłącz wzmacniacz audio (MAX98357A) – SD w stan niski = shutdown
    
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
    case State::SERVICE:
        led.setPixelColor(0, 0, 125, 255);
        break;
    }
    led.show();
}

void btnOnPressed(){
    switch (stateMachine.getState())
    {
    case State::INIT:
        stateMachine.setState(State::WEATHER);
        break;
    case State::DONE:
        stateMachine.setState(State::PLAY_AUDIO);
        break;
    case State::ERROR:
        if(WiFi.status() == WL_CONNECTED){
            stateMachine.setState(State::INIT);
        }
        else{
            stateMachine.setState(State::WiFi_CONNECTION);
            WiFi.disconnect();
            WiFi.begin(ssid, password);
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

void debugMode(){
    LOG_INFO("DebugMode");
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	int cnt = 0;
	while(WiFi.status() != WL_CONNECTED && cnt < 10){
		delay(500);
		cnt++;
	}
	Serial.begin(115200);
	Serial.println("\nPołączono z Wi-Fi!");
	Serial.print("Adres IP: ");
	Serial.println(WiFi.localIP());

	ArduinoOTA.setHostname("Pogodynka-ESP32"); // Nazwa widoczna w sieci
	// ArduinoOTA.setPassword("admin123");     // Opcjonalne hasło do wgrywania kodu

	ArduinoOTA.onStart([]() {
		Serial.println("Rozpoczęto wgrywanie kodu OTA...");
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("\nWgrywanie zakończone pomyślnie!");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Postęp: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Błąd OTA [%u]: ", error);
	});

	// 3. Start usługi OTA
	ArduinoOTA.begin();

    stateMachine.setState(State::SERVICE);    
}

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(ssid, password);

    audioOutI2S.SetPinout(Pins::BCLK, Pins::LRC, Pins::DIN);
    audioOutI2S.begin();

    pinMode(Pins::SD_MODE, OUTPUT);
    digitalWrite(Pins::SD_MODE, HIGH);

    led.begin();
    led.setBrightness(5);
    led.setPixelColor(0, 0, 0, 0); 
    led.show();
    btn.begin();
    btn.onPressed(btnOnPressed);
    btn.onPressedFor(3000, debugMode);
    

    WiFi.onEvent(onWiFi);

    stateMachine.setChangeStateCallback(updateLed);
    stateMachine.setState(State::WiFi_CONNECTION);
}

void loop()
{
    btn.read();

    switch (stateMachine.getState())
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
                stateMachine.setState(State::INIT);
            }
            if (timeoutTicks >= 10)
            {
                timeoutTicks = 0;
                stateMachine.setState(State::ERROR);
            }
        }
        break;

    case State::INIT:
    {
        stateMachine.setState(State::WEATHER);
        break;
    }
    case State::WEATHER:
        weatherData = openMeteo.getTodayWeatherData();
        if(weatherData.isEmpty()){
            LOG_ERROR("No weather data received!");
            stateMachine.setState(State::ERROR);
            break;
        }
        stateMachine.setState(State::LLM);
        break;
    case State::LLM:
    {
        String errDesc = "";
        weatherDesc = llm.askLLM(userInstruction + weatherData, errDesc);
        LOG_DEBUG(weatherDesc);
        if(errDesc != ""){
            LOG_ERROR(errDesc);
            stateMachine.setState(State::ERROR);
            break;
        }
        stateMachine.setState(State::FETCH_AUDIO);
    }
        break;
    case State::FETCH_AUDIO:
        if(audio.data != nullptr){
            free(audio.data);
        }

        audio = elevenLabs.getSpeechAudio(weatherDesc);

        if(audio.data != nullptr && audio.size > 0){
            stateMachine.setState(State::PLAY_AUDIO);
        }
        else{
            LOG_ERROR("Nie udało sie wygenerowac dzwieku");
            stateMachine.setState(State::ERROR);
            break;
        }
        break;
    case State::PLAY_AUDIO:
    {
        AudioFileSourceRAM audioMp3(audio.data, audio.size);
        AudioGeneratorMP3 mp3Conventer;
        if(!mp3Conventer.begin(&audioMp3, &audioOutI2S)){
            LOG_ERROR("MP3 begin() failed");
            stateMachine.setState(State::ERROR);
            break;
        }
        LOG_INFO("begin() OK, odtwarzam... size=" << audio.size);

        while(mp3Conventer.loop()){
            yield();
        }

        audioOutI2S.flush();
        mp3Conventer.stop();
        stateMachine.setState(State::DONE);
        break;
    }
    case State::DONE:
        break;

    case State::ERROR:
        break;

    case State::SERVICE:
        ArduinoOTA.handle();
        break;
    }

    // Deep sleep: jeśli system jest w stanie DONE lub ERROR przez DEEP_SLEEP_TIMEOUT_MS
    if (stateMachine.getState() == State::DONE || stateMachine.getState() == State::ERROR)
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
