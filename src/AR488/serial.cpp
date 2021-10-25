#include <Arduino.h>
#include "AR488_Config.h"

#ifdef AR_BT_EN
#include "AR488_BT.h"
#endif

/**********************************/
/***** SERIAL PORT MANAGEMENT *****/
/***** vvvvvvvvvvvvvvvvvvvvvv *****/


#ifdef AR_CDC_SERIAL
Serial_ *arSerial_ = &(AR_SERIAL_PORT);
#endif

#ifdef AR_HW_SERIAL
HardwareSerial *arSerial_ = &(AR_SERIAL_PORT);
#endif

// Note: SoftwareSerial support conflicts with PCINT support
#ifdef AR_SW_SERIAL
#include <SoftwareSerial.h>
SoftwareSerial swArSerial(AR_SW_SERIAL_RX, AR_SW_SERIAL_TX);
SoftwareSerial *arSerial_ = &swArSerial;
#endif

Stream *arSerial = (Stream*) arSerial_;

Stream& getSerial() {
	return *arSerial;
}

/***** Debug Port - if DB_SERIAL_PORT is set *****/

#if defined(DB_SERIAL_PORT)
#if defined(DB_CDC_SERIAL)
Serial_ *dbSerial_ = &(DB_SERIAL_PORT);
#elif define(DB_HW_SERIAL)
HardwareSerial *dbSerial_ = &(DB_SERIAL_PORT);
#elif defined(DB_SW_SERIAL)
// Note: SoftwareSerial support conflicts with PCINT support
#include <SoftwareSerial.h>
SoftwareSerial swDbSerial(DB_SW_SERIAL_RX, DB_SW_SERIAL_TX);
SoftwareSerial *dbSerial_ = &swDbSerial;
#else
// DB_SERIAL_PORT is defined but no dedicated port is configured, use arSerial
Stream *dbSerial_ = (Stream*) arSerial_;
#endif
Stream *dbSerial = (Stream*) dbSerial_;

Stream& getDbSerial() {
	return *dbSerial;
}
#endif


void initSerial() {
// Initialise debug port
#ifdef DB_SERIAL_PORT
  if (dbSerial_ != arSerial_) dbSerial_->begin(DB_SERIAL_BAUD);
#endif

 // Initialise serial comms over USB or Bluetooth
#ifdef AR_BT_EN
  // Initialise Bluetooth
  btInit();
  arSerial_->begin(AR_BT_BAUD);
#else
  // Start the serial port
  #ifdef AR_SW_SERIAL
    arSerial_->begin(AR_SW_SERIAL_BAUD);
  #else
    arSerial_->begin(AR_SERIAL_BAUD);
  #endif
#endif
	//arSerial->println(F("AR488 starting."));
}
