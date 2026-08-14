#ifndef __ELEVENLABS_H__
#define __ELEVENLABS_H__
#include <Arduino.h>

struct AudioBuffer {
    uint8_t* data = nullptr;
    size_t size = 0;
};

class ElevenLabs
{
public:
    ElevenLabs(String apiKey, String voiceId, String modelId, String outputFormat = "mp3_22050_32");
    AudioBuffer getSpeechAudio(String text);
private:
    String apiKey;
    String voiceId;
    String modelId;
    String outputFormat;
};

#endif // __ELEVENLABS_H__