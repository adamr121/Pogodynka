#ifndef GEMINI_H
#define GEMINI_H

#include <Arduino.h>

class Gemini{
    public:
    Gemini(const char * model, const char* apiKey);
    String askLLM(const String& prompt);

    private:
    String model;
    String apiKey;
};

#endif