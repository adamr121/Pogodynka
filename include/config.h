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

// GEMINI (model to nie sekret)
const char* model = "gemini-3.5-flash-lite";
const char* userInstruction = "Przeanalizuj poniższy JSON z danymi pogodowymi i przygotuj zwięzłe podsumowanie w maksymalnie 3 zdaniach. "
                            "Wymagania dotyczące odpowiedzi:"
                            "1. Odpowiedź musi być krótka i zwięzła – maksymalnie 3 zdania."
                            "2. Podaj przewidywaną temperaturę na najblizsze rano, po południe i wieczor."
                            "2. Opisz tez jaka pogoda jest obecnie, wpis w najnowszej godziny."
                            "3. Podaj informację o ewentualnych opadach (czy wystąpią deszcz/śnieg i kiedy) ALE TYLKO WTEDY GDY MAJA WYSTAPIC. Jesli sa zapowiedziane podaj w jakich godzinach"
                            "4. NIE WSPOMINAJ O BRAKU OPADOW"
                            "4. Zapomnij o podawaniu konkretnych godzin (np. o 7:00, o 16:00) – używaj wyłącznie ogólnych pór dnia (rano,po południu, wieczorem)."
                            "5. Pisz prostym językiem, bez zbędnych wstępów (np. Oto prognoza:)."
                            "5. Powiedz jakie będzie zachmurzenie ogolnie danego dnia, bez szczegolow"
                            "6. Przedstaw liczby i symbole w postaci tekstowej, alfabetycznej.Np 23 °C = dwadziescia trzy stopnie, nie dodawaj nie potrzbnie słowa celsjusza"
                            "Dane pogodowe (JSON):";


#endif