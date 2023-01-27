#if !defined(MACROS_H)

#ifdef USE_MACROS
#include <Arduino.h>
#include "AR488_Config.h"
#include "controller.h"

// make sure
//   NUM_MACROS x (2+MACRO_MAX_LEN) + (2+sizeof(AR488Conf))
// fits in available EEPROM
#ifndef NUM_MACROS
#define NUM_MACROS 10
#endif
#ifndef MACRO_MAX_LEN
#ifdef ESP32
#define MACRO_MAX_LEN 128
#else
#define MACRO_MAX_LEN 32
#endif
#endif

String getMacro(uint8_t idx);
void saveMacro(uint8_t, String&);
void deleteMacro(uint8_t);
void execMacro(uint8_t idx, Controller& controller);
void execMacro(String& macro, Controller& controller);

bool isMacro(uint8_t idx);

#ifndef ESP32
int addressForMacro(uint8_t macro);
#endif

#endif

#define MACROS_H
#endif
