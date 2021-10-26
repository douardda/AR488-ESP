//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wtype-limits"
//#pragma GCC diagnostic ignored "-Wunused-variable"
#include <Arduino.h>

#ifdef __AVR__
  #include <avr/wdt.h>
#endif

//#pragma GCC diagnostic pop

#include "AR488_Config.h"
#include "AR488_Layouts.h"

#ifdef USE_INTERRUPTS
  #ifdef __AVR__
    #include <avr/interrupt.h>
  #endif
#endif

#ifdef E2END
  #include "AR488_Eeprom.h"
  #include <EEPROM.h>
#endif

#ifdef AR_BT_EN
  #include "AR488_BT.h"
#endif

/***** FWVER "AR488 GPIB controller, ver. 0.49.14, 02/03/2021" *****/

/*
  Arduino IEEE-488 implementation by John Chajecki

  Inspired by the original work of Emanuele Girlando, licensed under a Creative
  Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
  Any code in common with the original work is reproduced here with the explicit
  permission of Emanuele Girlando, who has kindly reviewed and tested this code.

  Thanks also to Luke Mester for comparison testing against the Prologix interface.
  AR488 is Licenced under the GNU Public licence.

  Thanks to 'maxwell3e10' on the EEVblog forum for suggesting additional auto mode
  settings and the macro feature.

  Thanks to 'artag' on the EEVblog forum for providing code for the 32u4.

	For information regarding the GPIB firmware by Emanualle Girlando see:
	http://egirland.blogspot.com/2014/03/arduino-uno-as-usb-to-gpib-controller.html
*/


/*
   Pin mapping between the Arduino pins and the GPIB connector.
   NOTE:
   GPIB pins 10 and 18-24 are connected to GND
   GPIB pin 12 should be connected to the cable shield (might be n/c)
   Pin mapping follows the layout originally used by Emanuelle Girlando, but adds
   the SRQ line (GPIB 10) on pin 2 and the REN line (GPIB 17) on pin 13. The program
   should therefore be compatible with the original interface design but for full
   functionality will need the remaining two pins to be connected.
   For further information about the AR488 see the documentation.
*/


/*********************************/
/***** CONFIGURATION SECTION *****/
/***** vvvvvvvvvvvvvvvvvvvvv *****/
// SEE >>>>> Config.h <<<<<
/***** ^^^^^^^^^^^^^^^^^^^^^ *****/
/***** CONFIGURATION SECTION *****/
/*********************************/


#include "AR488.h"
#include "serial.h"
#include "controller.h"
#include "commands.h"
#include "gpib.h"
#include "macros.h"

/****** Global variables with volatile values related to controller state *****/
Controller controller(getSerial());
GPIB gpib(controller);

/***** ^^^^^^^^^^^^^^^^^^^^^^^^ *****/
/***** COMMON VARIABLES SECTION *****/
/************************************/


/*******************************/
/***** COMMON CODE SECTION *****/
/***** vvvvvvvvvvvvvvvvvvv *****/


/******  Arduino standard SETUP procedure *****/
void setup() {

  // Disable the watchdog (needed to prevent WDT reset loop)
#ifdef __AVR__
  wdt_disable();
#endif

  // Turn off internal LED (set OUPTUT/LOW)
#ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
#endif

#ifdef USE_INTERRUPTS
  // Turn on interrupts on port
  interruptsEn();
#endif

  // Initialise parse buffer
  controller.flushPbuf();
	initSerial();

// Un-comment for diagnostic purposes
/*
  #if defined(__AVR_ATmega32U4__)
    while(!*arSerial)
    ;
//    Serial.print(F("Starting "));
    for(int i = 0; i < 20; ++i) {  // this gives you 10 seconds to start programming before it crashes
      Serial.print(".");
      delay(500);
    }
    Serial.println("@>");
  #endif // __AVR_ATmega32U4__
*/
// Un-comment for diagnostic purposes


  // Initialise
  controller.initConfig();
/*
#ifdef E2END
  // Read data from non-volatile memory
  //(will only read if previous config has already been saved)
//  epGetCfg();
  if (!isEepromClear()) {
		uint8_t *conf = (uint8_t*) &AR488;
    if (!epReadData(conf, AR_CFG_SIZE)) {
      // CRC check failed - config data does not match EEPROM
      epErase();
      initAR488();
      epWriteData(conf, AR_CFG_SIZE);
    }
  }
#endif
*/

  // Initialize the interface in device mode
  if (controller.config.cmode == 1) gpib.initDevice();
  // Initialize the interface in controller mode
	else gpib.initController();

  gpib.clearATN();
  gpib.clearSRQ();

#if defined(USE_MACROS) && defined(RUN_STARTUP)
  // Run startup macro
  execMacro(0, gpib);
#endif

#ifdef SAY_HELLO
  controller.stream.println(F("AR488 ready."));
#endif

}
/****** End of Arduino standard SETUP procedure *****/


/***** ARDUINO MAIN LOOP *****/
void loop() {

/*** Macros ***/
/*
 * Run the startup macro if enabled
 */
#ifdef USE_MACROS
  // Run user macro if flagged
  if (controller.runMacro > 0) {
			execMacro(controller.runMacro, gpib);
    controller.runMacro = 0;
  }
#endif

/*** Pin Hooks ***/
/*
 * Not all boards support interrupts or have PCINTs. In this
 * case, use in-loop checking to detect when SRQ and ATN have
 * been signalled
 */
#ifndef USE_INTERRUPTS
  gpib.setATN(digitalRead(ATN)==LOW ? true : false);
  gpib.setSRQ(digitalRead(SRQ)==LOW ? true : false);
#endif

/*** Process the buffer ***/
/* Each received char is passed through parser until an un-escaped
 * CR is encountered. If we have a command then parse and execute.
 * If the line is data (inclding direct instrument commands) then
 * send it to the instrument.
 * NOTE: parseInput() sets lnRdy in serialEvent, readBreak or in the
 * above loop
 * lnRdy=1: process command;
 * lnRdy=2: send data to Gpib
 */

  // lnRdy=1: received a command so execute it...
  if (controller.lnRdy == 1) {
    execCmd(controller.pBuf, controller.pbPtr, gpib);
  }

  // Controller mode:
  if (controller.config.cmode == 2) {
    // lnRdy=2: received data - send it to the instrument...
    if (controller.lnRdy == 2) {
      gpib.sendToInstrument(controller.pBuf, controller.pbPtr);
      // Auto-read data from GPIB bus following any command
      if (controller.config.amode == 1) {
        //        delay(10);
        gpib.gpibReceiveData();
      }
      // Auto-receive data from GPIB bus following a query command
      if (controller.config.amode == 2 && gpib.isQuery) {
        //        delay(10);
        gpib.gpibReceiveData();
        gpib.isQuery = false;
      }
    }

    // Check status of SRQ and SPOLL if asserted
    if (gpib.isSRQ() && controller.isSrqa) {
			spoll_h(NULL, gpib);
      gpib.clearSRQ();
    }

    // Continuous auto-receive data from GPIB bus
    if (controller.config.amode == 3 && controller.aRead) gpib.gpibReceiveData();
  }

  // Device mode:
  if (controller.config.cmode == 1) {
    if (controller.isTO) {
      if (controller.lnRdy == 2) gpib.sendToInstrument(controller.pBuf, controller.pbPtr);
    }else if (controller.isRO) {
      gpib.lonMode();
    }else{
			if (gpib.isATN()) gpib.attnRequired();
      if (controller.lnRdy == 2) gpib.sendToInstrument(controller.pBuf, controller.pbPtr);
    }
  }

  // Reset line ready flag
//  lnRdy = 0;

  // IDN query ?
  if (controller.sendIdn) {
    if (controller.config.idn==1) controller.stream.println(controller.config.sname);
    if (controller.config.idn==2) {
				controller.stream.print(controller.config.sname);
				controller.stream.print("-");
				controller.stream.println(controller.config.serial);
		}
    controller.sendIdn = false;
  }

  // Check serial buffer
  controller.serialIn_h();

  delayMicroseconds(5);
}
/***** END MAIN LOOP *****/





/*************************************/
/***** Device operation routines *****/
/*************************************/



#ifdef DEBUG1
/***** Prints charaters as hex bytes *****/
void printHex(char *buffr, int dsize) {
  for (int i = 0; i < dsize; i++) {
    dbSerial->print(buffr[i], HEX); dbSerial->print(" ");
  }
  dbSerial->println();
}
#endif
