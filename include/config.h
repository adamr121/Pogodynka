#ifndef CONFIG_H
#define CONFIG_H

#include "secret.h"

#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG

//ElevenLabs (dane nie-poufne)
//const char* elevenLabsModelId = "eleven_multilingual_v2";
const char* elevenLabsModelId = "eleven_flash_v2_5";
const char* elevenLabsOutPutFormat = "mp3_22050_32";
//const char* elevenLabsVoiceId = "pFZP5JQG7iQjIQuC4Bku"; //dziala
const char* elevenLabsVoiceId = "JBFqnCBsd6RMkjVDRZzb"; //dziala

// ===== Deep sleep =====
// Po jakim czasie bezczynności w stanie DONE lub ERROR przejść w deep sleep
constexpr unsigned long DEEP_SLEEP_TIMEOUT_MS = 1UL * 60UL * 1000UL; // 5 minut

// GEMINI (model to nie sekret)
const char* model = "gemini-3.5-flash-lite";
const char* userInstruction = "Masz dane pogodowe godzinowe na najbliższe 24 godziny (JSON poniżej). "
                            "1. Opisz pogodę OBECNIE (najnowsza godzina z danych) oraz najbliższe pory dnia, które są w danych – np. popołudnie/wieczór dzisiaj, a potem rano/popołudnie/wieczór jutra, w zależności od tego, co obejmuje okno 24h. "
                            "2. Opisuj TYLKO te pory, które są w danych. Jeśli okno 24h nie sięga np. jutrzejszego wieczoru, NIE wspominaj o pogodzie na ten czas. "
                            "3. Nie podawaj, która jest teraz godzina ani która pora dnia trwa – to oczywiste, pomiń to w odpowiedzi. "
                            "4. Podaj temperaturę wprost i prosto, np. 'będzie dwadzieścia sześć stopni' albo 'jest piętnaście stopni'. Nie używaj ozdobników typu 'słupki rtęci pokażą', 'termometry wskażą' itp. "
                            "5. Podaj informację o opadach (deszcz/śnieg) TYLKO wtedy, gdy mają wystąpić – wtedy określ w jakiej porze. NIE WSPOMINAJ O BRAKU OPADÓW. "
                            "6. Nie podawaj konkretnych godzin (np. 7:00, 16:00) – używaj ogólnych pór dnia (rano, po południu, wieczorem). "
                            "7. Pisz prostym językiem, bez wstępów (np. 'Oto prognoza:'). Odpowiedź trzymaj zwięźle – maksymalnie 3-4 zdania. "
                            "8. Podaj ogólne zachmurzenie na dany dzień, bez szczegółów. "
                            "9. Przedstaw liczby tekstowo, np. 23 °C = dwadzieścia trzy stopnie, bez słowa 'celsjusza'. "
                            "Dane pogodowe (JSON):";


#endif