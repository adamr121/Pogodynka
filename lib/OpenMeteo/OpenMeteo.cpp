#include "OpenMeteo.h"
#include <logger.h>
#include <HTTPClient.h>

OpenMeteo::OpenMeteo()
{

}

String OpenMeteo::getTodayWeatherData()
{
    LOG_INFO("OpenMeteo::getWeatherData()");
    
    if (WiFi.status() != WL_CONNECTED)
    {
        LOG_ERROR("No WiFi connection!");
        return "";
    }

    LOG_DEBUG("Sending GET request");
    HTTPClient http;
    http.begin("https://api.open-meteo.com/v1/forecast?latitude=52.403313671965535&longitude=16.90674009040985&hourly=temperature_2m,wind_speed_10m,cloud_cover,precipitation&timezone=Europe%2FBerlin&forecast_days=2&forecast_hours=24");
    int httpCode = http.GET();

    LOG_DEBUG("httpCode= " << httpCode);
    
    String payload = "";
    if (httpCode > 0)
    {
        payload = http.getString();
    }
    http.end();
    LOG_DEBUG("Response: " << payload);
    
    return payload;
}
