#if !defined(MACROS_H)

/*************************************/
/***** MACRO STRUCTRURES SECTION *****/
/***** vvvvvvvvvvvvvvvvvvvvvvvvv *****/
#ifdef USE_MACROS
#include <Arduino.h>
#include "AR488_Config.h"
#include "controller.h"

// make sure this fits in available EEPROM (with the general AR488Config structure)
#ifndef NUM_MACROS
#define NUM_MACROS 10
#endif
#ifndef MACRO_MAX_LEN
#define MACRO_MAX_LEN 32
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
