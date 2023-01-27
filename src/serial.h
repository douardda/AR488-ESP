#if !defined(SERIAL_H)

#include <Arduino.h>

Stream* getSerialStream();
#ifdef AR488_BT_ENABLE
Stream* getBTSerialStream();
#endif

#if defined(DB_SERIAL_PORT)
Stream* getDbSerialStream();
#endif

#define SERIAL_H
#endif
