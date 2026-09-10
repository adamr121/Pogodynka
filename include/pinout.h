#ifndef __PINOUT_H__
#define __PINOUT_H__

#include <Arduino.h>
#include "config.h"

#ifdef ESP_C3_MINI
namespace Pins{
    constexpr uint8_t LRC =  7;
    constexpr uint8_t BCLK =  6; // GPIO8 nie wypuszcza BCLK (0 V) -> przeniesione na GPIO6
    constexpr uint8_t DIN =  9;
    constexpr uint8_t SD_MODE =  10;
    constexpr uint8_t LED =  3;
    constexpr uint8_t Button =  4  ;
    constexpr uint8_t Battery =  1;
}
#endif


#ifdef ESP_WROOM_32D
namespace Pins{
    constexpr uint8_t BCLK =  26;
    constexpr uint8_t LRC =  25;
    constexpr uint8_t DIN =  27;
    constexpr uint8_t SD_MODE =  23;
    constexpr uint8_t LED =  13;
    constexpr uint8_t Button =  21;
    constexpr uint8_t Battery =  1;
}
#endif

#endif // __PINOUT_H__