#ifndef _AUDIOFILESOURCERAM_H
#define _AUDIOFILESOURCERAM_H

#include <Arduino.h>
#include "AudioFileSource.h"

class AudioFileSourceRAM : public AudioFileSource
{
public:
    AudioFileSourceRAM(const uint8_t *data, uint32_t len);

    virtual bool open(const char *filename) override;
    virtual uint32_t read(void *dest, uint32_t bytes) override;
    virtual bool seek(int32_t pos, int dir) override;
    virtual bool close() override;
    virtual bool isOpen() override;
    virtual uint32_t getSize() override;
    virtual uint32_t getPos() override;

private:
    const uint8_t *_data;
    uint32_t _len;
    uint32_t _pos;
};

#endif
