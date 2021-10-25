#if !defined(SERIAL_H)

#include <Arduino.h>

Stream& getSerial();

#if defined(DB_SERIAL_PORT)
Stream& getDbSerial();
#endif

void initSerial();

#define SERIAL_H
#endif
