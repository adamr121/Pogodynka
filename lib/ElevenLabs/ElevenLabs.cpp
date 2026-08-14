#include "ElevenLabs.h"
#include <WiFi.h>
#include <logger.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

ElevenLabs::ElevenLabs(String apiKey, String voiceId, String modelId, String outputFormat): apiKey(apiKey), voiceId(voiceId), modelId(modelId), outputFormat(outputFormat)
{
    
}

AudioBuffer ElevenLabs::getSpeechAudio(String text)
{
    LOG_INFO("ElevenLabs::getSpeechAudio");
    AudioBuffer result;
    if(WiFi.status() != WL_CONNECTED){
        LOG_ERROR("No WiFi connection!");
        return result;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    String url = "https://api.elevenlabs.io/v1/text-to-speech/" + voiceId + "?output_format=" + outputFormat;

    http.begin(client, url);
    http.setTimeout(10000);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("xi-api-key", apiKey);

    String jsonPayload = "{"
        "\"text\":\"" + text + "\","
        "\"model_id\":\"" + modelId + "\","
        "\"language_code\":\"pl\","
        "\"speed\":1.85"
        "}";

    LOG_DEBUG("JsonPayload=" << jsonPayload);
    int httpCode = http.POST(jsonPayload);
    LOG_DEBUG("httpCode=" << httpCode);

    if(httpCode == HTTP_CODE_OK){
        int contentLength = http.getSize();

        if(contentLength < 0) {
            LOG_ERROR("contentLenght < 0");
            http.end();
            return result;
        }

        LOG_INFO("Downloading audio, size in KB = " << contentLength);

        result.data = (uint8_t*) malloc(contentLength);

        if(result.data == nullptr){
            LOG_ERROR("Not enough memory to allocate!");
            http.end();
            return result;
        }
        
        WiFiClient* stream = http.getStreamPtr();
        size_t totalBytesRead = 0;

        while(http.connected() && totalBytesRead < contentLength){
            size_t availableBytes = stream->available();
            if(availableBytes > 0) {
                int bytesRead = stream->readBytes(result.data + totalBytesRead, availableBytes);
                totalBytesRead += bytesRead;    
            }
            yield();
        }
        result.size = totalBytesRead;
        LOG_DEBUG("Audio loaded in RAM! TotalBytesRead=" << totalBytesRead);
        LOG_DEBUG("Memory left=" << ESP.getFreeHeap());
    }
    else{
        LOG_ERROR("Http POST error=" << httpCode);
        if(httpCode>0){
            LOG_ERROR(http.getString());
        }
    }
    
    http.end();
    return result;
}