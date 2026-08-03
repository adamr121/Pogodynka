#ifndef OPENMETEO_H
#define OPENMETEO_H

#include <Arduino.h>


class OpenMeteo {
public:
    OpenMeteo();
    String getTodayWeatherData();

private:
    String apiKey;
};

#endif
