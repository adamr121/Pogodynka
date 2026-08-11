#include "AudioFileSourceRAM.h"

AudioFileSourceRAM::AudioFileSourceRAM(const uint8_t *data, uint32_t len): _data(data), _len(len) , _pos(0)
{
}

bool AudioFileSourceRAM::open(const char *filename)
{
    return false;
}

uint32_t AudioFileSourceRAM::read(void *data, uint32_t len)
{
    uint32_t n;
    if( _pos + len < _len){
        n = len;
    }
    else{
        n = _len- _pos;
    }

    memcpy(data, _data + _pos, n);

    _pos += n;

    return n;
}

bool AudioFileSourceRAM::seek(int32_t pos, int dir)
{
    return false;
}

bool AudioFileSourceRAM::close(void)
{
    _pos=0;
    return true;
}

bool AudioFileSourceRAM::isOpen(void)
{
    return (_data != nullptr && _len > 0);
}

uint32_t AudioFileSourceRAM::getSize(void)
{
    return _len;
}

uint32_t AudioFileSourceRAM::getPos(void)
{
    return _pos;
}
