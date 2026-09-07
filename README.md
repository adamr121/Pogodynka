# Pogodynka

Stacja pogodowa na **ESP32**, która na żądanie (przycisk) pobiera prognozę, zamienia ją na naturalny opis po polsku (LLM Gemini), a następnie **odtwarza ten opis głosowo** przez wzmacniacz MAX98357A. Zasilana z baterii LiPo z deep sleep w celu oszczędzania energii.

---

## Spis treści

- [Funkcje](#funkcje)
- [Architektura sprzętowa](#architektura-sprzętowa)
- [Konfiguracja pinów](#konfiguracja-pinów)
- [Architektura oprogramowania](#architektura-oprogramowania)
- [Przepływ pracy (stany)](#przepływ-pracy-stany)
- [Tryby oszczędzania energii](#tryby-oszczędzania-energii)
- [Konfiguracja](#konfiguracja)
- [Sekrety / klucze API](#sekrety--klucze-api)
- [Budowa i wgrywanie (PlatformIO)](#budowa-i-wgrywanie-platformio)
- [Zależności (biblioteki)](#zależności-biblioteki)
- [Rozwiązywanie problemów](#rozwiązywanie-problemów)

---

## Funkcje

- Pobiera **prognozę pogody na 24 h** z serwisu Open-Meteo (bez klucza API).
- Generuje **zwięzły, naturalny opis pogody po polsku** przez model Gemini (LLM).
- Zamienia tekst na **mowę (TTS)** przez ElevenLabs i odtwarza ją jako MP3 przez głośnik.
- Sterowanie **przyciskiem**: odpalenie prognozy, ponowne odtworzenie, ponowna próba po błędzie.
- **NeoPixel** sygnalizuje bieżący stan kolorami.
- **Deep sleep** po bezczynności + budzenie przyciskiem (oszczędność baterii).

---

## Architektura sprzętowa

| Element | Rola |
|---|---|
| ESP32 (WROOM-32D lub ESP32-C3 mini) | sterowanie całością |
| Moduł audio **MAX98357A** (I2S) + głośnik | odtwarzanie MP3 |
| **NeoPixel** (WS2812B, 1 dioda) | wskaźnik stanu |
| **Przycisk** | interakcja użytkownika |
| **LiPo 400 mAh** + moduł ładowania | zasilanie |
| Open-Meteo / Gemini / ElevenLabs (REST) | dane pogodowe / LLM / TTS |

> ⚠️ Projekt obsługuje **dwie konfiguracje sprzętowe** wybierane makrem kompilacji: `ESP_WROOM_32D` oraz `ESP_C3_MINI` (patrz `platformio.ini`).

---

## Konfiguracja pinów

Definicje pinów są w pliku `include/pinout.h` (osobno dla każdej płytki).

### ESP32 WROOM-32D (`ESP_WROOM_32D`)
| Funkcja | GPIO |
|---|---|
| I2S BCLK | 26 |
| I2S LRC (WS) | 25 |
| I2S DIN | 27 |
| SD_MODE (MAX98357A shutdown) | 23 |
| LED (NeoPixel) | 13 |
| Przycisk | 21 |
| Bateria (ADC pomiar) | 1 |

### ESP32-C3 mini (`ESP_C3_MINI`)
| Funkcja | GPIO |
|---|---|
| I2S LRC | 7 |
| I2S BCLK | 8 |
| I2S DIN | 9 |
| SD_MODE | 10 |
| LED (NeoPixel) | 3 |
| Przycisk | 4 |
| Bateria (ADC) | 1 |

---

## Architektura oprogramowania

```
src/
  main.cpp                – logika domenowa, pętla stanów, deep sleep
lib/
  StateMachine/           – klasa maszyny stanów + callback na zmianę stanu
  OpenMeteo/              – pobieranie prognozy (REST)
  Gemini/                 – generowanie opisu tekstowego (LLM)
  ElevenLabs/             – generowanie mowy (TTS) -> AudioBuffer
  AudioFileSourceRAM/     – źródło MP3 z pamięci RAM
  ESP8266Audio/           – dekodowanie i odtwarzanie audio
include/
  config.h                – konfiguracja (modele, czasy, poziomy logów)
  pinout.h                – przypisanie pinów per płytka
  logger.h                – makra logowania LOG_* z poziomami
  secret.h                – POUFNE: WiFi SSID/hasło + klucze API
```

### Klasa `StateMachine`
Przechowuje bieżący stan, centralizuje przejścia, loguje zmiany i wywołuje **callback** przy realnej zmianie stanu (`if (currentState != state)` + guard `callback != nullptr`). Callback rejestruje się przez `setChangeStateCallback(...)` — w `main.cpp` jest nim funkcja aktualizująca NeoPixel.

---

## Przepływ pracy (stany)

Stany zdefiniowane w `StateMachine.h`: `INIT`, `WiFi_CONNECTION`, `WEATHER`, `LLM`, `FETCH_AUDIO`, `PLAY_AUDIO`, `DONE`, `ERROR`.

```
         (start)
            │
            ▼
   WiFi_CONNECTION ──połączono──► INIT ──► WEATHER
            │                         ▲        │ (brak danych)
            ▼ (błąd 10 prób)          │        ▼
          ERROR ◄─────────────────────┘       LLM ──► FETCH_AUDIO ──► PLAY_AUDIO ──► DONE
            ▲                                                                (czeka na przycisk)
            └────────────── przycisk (retry/ponów) ◄───────────────────────────────┘
```

1. **WiFi_CONNECTION** — próba połączenia z WiFi (do 10 prób, timer 500 ms); po sukcesie → `INIT`.
2. **WEATHER** — pobranie danych z Open-Meteo; błąd → `ERROR`.
3. **LLM** — Gemini generuje polski opis prognozy; błąd → `ERROR`.
4. **FETCH_AUDIO** — ElevenLabs zamienia tekst na MP3 (bufor RAM).
5. **PLAY_AUDIO** — dekodowanie i odtwarzanie MP3 przez I2S.
6. **DONE** — oczekiwanie; przycisk odtwarza ponownie (replay).
7. **ERROR** — przycisk: jeśli WiFi działa → retry od `INIT`; jeśli nie → ponów `WiFi_CONNECTION`.

Po bezczynności w `DONE`/`ERROR` przez `DEEP_SLEEP_TIMEOUT_MS` (1 min) układ przechodzi w deep sleep.

---

## Tryby oszczędzania energii

- **Deep sleep** po 1 min bezczynności w stanie końcowym (`goToDeepSleep()`).
- Przed snem: **NeoPixel wygaszony**, **MAX98357A w shutdown** (SD_MODE = LOW), **WiFi wyłączone**.
- **Budzenie przyciskiem** (EXT0 / GPIO wakeup, poziom LOW).

> Prąd w deep sleep zależy mocno od płytki (LDO + dioda power-LED na devkicie potrafią ciągnąć kilka mA). C3-mini jest dużo oszczędniejszy. Szczegóły szacowania czasu pracy z baterii — patrz sekcja *Rozwiązywanie problemów / bateria*.

---

## Konfiguracja

`include/config.h`:

| Parametr | Opis |
|---|---|
| `CURRENT_LOG_LEVEL` | poziom logów (`LOG_LEVEL_ERROR..DEBUG`) |
| `elevenLabsModelId` / `elevenLabsVoiceId` / `elevenLabsOutPutFormat` | model/format TTS |
| `DEEP_SLEEP_TIMEOUT_MS` | czas bezczynności do deep sleep |
| `model` | model Gemini |
| `userInstruction` | instrukcja (prompt) dla LLM — jak opisywać pogodę |

---

## Sekrety / klucze API

Wrażliwe dane trzymaj w **`include/secret.h`** (nie jest wersjonowany ani pokazywany w tym dokumencie). Zwykle zawiera:

- `ssid`, `password` — dane WiFi
- `geminiApiKey`, `elevenLabsApiKey`

> ⚠️ **Nigdy nie publikuj** `secret.h`. Modele i voice ID nie są sekretami (są w `config.h`).

---

## Budowa i wgrywanie (PlatformIO)

`platformio.ini` definiuje dwa środowiska:

```ini
[env:esp32dev]   ; ESP32 WROOM-32D
[env:esp32-c3]   ; ESP32-C3 mini (lolin_c3_mini)
```

- Kompilacja i wgrywanie: `pio run -t upload -e <env>`
- Monitor szeregowy: `pio device monitor -e <env>` (115200 bodów)

Wspólne zależności: `ArduinoJson`, `Adafruit NeoPixel`, `EasyButton`.

---

## Zależności (biblioteki)

| Biblioteka | Zastosowanie |
|---|---|
| `bblanchon/ArduinoJson` | parsowanie JSON (dane pogodowe / odpowiedzi API) |
| `adafruit/Adafruit NeoPixel` | sterowanie diodą statusu |
| `evert-arias/EasyButton` | obsługa przycisku (debounce, zdarzenia) |
| `ESP8266Audio` (lokalna w `lib/`) | dekoder i wyjście audio (MP3, I2S) |
| `AudioFileSourceRAM` (lokalna) | odtwarzanie z bufora RAM |

`lib_extra_dirs` wskazuje na zewnętrzny katalog bibliotek (`Rakieta\Libs`).

---

## Rozwiązywanie problemów

**WiFi niestabilne / zrywanie (AUTH_EXPIRE, brak IP).**
- Loguj `WiFi.disconnectReason()` przez `WiFi.onEvent` (dwuargumentowy callback).
- Sprawdź RSSI, unikalność MAC, test na hotspocie telefonu (izolacja płyta vs router).
- Część płyt ESP32-C3 mini z tej samej partii bywa wadliwa na poziomie RF — sprawdzaj na WROOM-32D.

**Głośny/brak dźwięku.**
- Sprawdź okablowanie I2S (BCLK/LRC/DIN) i zasilanie MAX98357A.
- Potwierdź, że SD_MODE jest HIGH podczas odtwarzania.

**Bateria — czas pracy.**
- Prąd deep sleep mierz multimetrem szeregowo z baterią.
- Na devkicie odetnij power-LED / zasilaj przez pin 3.3 V (pominięcie LDO) dla niższego poboru.

---

© Projekt edukacyjny ESP32 / PlatformIO (Arduino C++).
