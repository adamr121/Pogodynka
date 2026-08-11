#ifndef __PINOUT_H__
#define __PINOUT_H__

#include <Arduino.h>

namespace Pins{
    constexpr uint8_t BCLK =  26;
    constexpr uint8_t LRC =  25;
    constexpr uint8_t DIN =  27;
    constexpr uint8_t SD_MODE =  23;
    constexpr uint8_t LED =  13;
}

#endif // __PINOUT_H__