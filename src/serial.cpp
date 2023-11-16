#include <Arduino.h>
#include "AR488_Config.h"

#ifdef AR488_BT_ENABLE

#ifdef AR488_BT_HC05
#include "AR488_HC05.h"
#endif

#ifdef ESP32
#include "BluetoothSerial.h"
#endif

#endif

#ifdef AR_ESP32S2_USB_CDC
#include "USB.h"
USBCDC UsbCdcSerial;
#endif

/********** BT SERIAL PORT DECLARATIONS **********/
/* on ESP32, this comes as an extra serial port  */
/* on Arduino, this is done using an HC05 like   */
/* module plugged on the a serial port (possibly */
/* the main one).                                 */

Stream *arSerial = NULL;

Stream* getSerialStream() {
  if (arSerial == NULL) {
#if defined(AR_ESP32S2_USB_CDC)
	arSerial =(Stream*) &(UsbCdcSerial);
	UsbCdcSerial.begin();
	USB.begin();

#else
#if defined(AR_CDC_SERIAL)
	Serial_ *serial = &(AR_SERIAL_PORT);
#elif defined(AR_HW_SERIAL)
	HardwareSerial *serial = &(AR_SERIAL_PORT);
#elif defined(AR_SW_SERIAL)
// Note: SoftwareSerial support conflicts with PCINT support
#include <SoftwareSerial.h>
	SoftwareSerial *serial = new SoftwareSerial(AR_SW_SERIAL_RX, AR_SW_SERIAL_TX);
#endif
	serial->begin(AR_SERIAL_BAUD);
	arSerial = (Stream*) serial;
#endif

  }
  return arSerial;
}

#ifdef AR488_BT_ENABLE
Stream *btSerial = NULL;

Stream* getBTSerialStream() {
  if(btSerial == NULL) {
#if defined(ESP32)
	BluetoothSerial *serial = new BluetoothSerial();
	serial->begin(AR_BT_NAME);
#else
#if defined(AR_CDC_SERIAL)
	Serial_ *serial = &(AR_BT_SERIAL_PORT);
#elif defined(AR_HW_SERIAL)
	HardwareSerial *serial = &(AR_BT_SERIAL_PORT);
#elif defined(AR_SW_SERIAL)
// Note: SoftwareSerial support conflicts with PCINT support
#include <SoftwareSerial.h>
	SoftwareSerial *serial = new SoftwareSerial(AR_SW_BT_SERIAL_RX, AR_SW_BT_SERIAL_TX);
#endif
	serial->begin(AR_BT_BAUD);
#if defined(AR488_BT_HC05)
	hc05Init((Stream*) serial);
#endif

#endif // !ESP32
	btSerial = (Stream*) serial;
  }
  return btSerial;
}

#endif

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
