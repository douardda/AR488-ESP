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
*/

/*
   Implements most of the CONTROLLER functions;
   Substantially compatible with 'standard' Prologix "++" commands
   (see +savecfg command in the manual for differences)

   Principle of operation:
   - Commands received from USB are buffered and whole terminated lines processed
   - Interface commands prefixed with "++" are passed to the command handler
   - Instrument commands and data not prefixed with '++' are sent directly to the GPIB bus.
   - To receive from the instrument, issue a ++read command or put the controller in auto mode (++auto 1|2)
   - Characters received over the GPIB bus are unbuffered and sent directly to USB
   NOTES:
   - GPIB line in a HIGH state is un-asserted
   - GPIB line in a LOW state is asserted
   - The ATMega processor control pins have a high impedance when set as inputs
   - When set to INPUT_PULLUP, a 10k pull-up (to VCC) resistor is applied to the input
*/

/*
   Standard commands

   ++addr         - display/set device address
   ++auto         - automatically request talk and read response
   ++clr          - send Selected Device Clear to current GPIB address
   ++eoi          - enable/disable assertion of EOI signal
   ++eos          - specify GPIB termination character
   ++eot_enable   - enable/disable appending user specified character to USB output on EOI detection
   ++eot_char     - set character to append to USB output when EOT enabled
   ++ifc          - assert IFC signal for 150 miscoseconds - make AR488 controller in charge
   ++llo          - local lockout - disable front panel operation on instrument
   ++loc          - enable front panel operation on instrument
   ++lon          - put controller in listen-only mode (listen to all traffic)
   ++mode         - set the interface mode (0=controller/1=device)
   ++read         - read data from instrument
   ++read_tmo_ms  - read timeout specified between 1 - 3000 milliseconds
   ++rst          - reset the controller
   ++savecfg      - save configration
   ++spoll        - serial poll the addressed host or all instruments
   ++srq          - return status of srq signal (1-srq asserted/0-srq not asserted)
   ++status       - set the status byte to be returned on being polled (bit 6 = RQS, i.e SRQ asserted)
   ++trg          - send trigger to selected devices (up to 15 addresses)
   ++ver          - display firmware version
*/

/*
   Proprietry commands:

   ++aspoll       - serial poll all instruments (alias: ++spoll all)
   ++default      - set configuration to controller default settings
   ++dcl          - send unaddressed (all) device clear  [power on reset] (is the rst?)
   ++id name      - show/set the name of the interface
   ++id serial    - show/set the serial number of the interface
   ++id verstr    - show/set the version string (replaces setvstr)
   ++idn          - enable/disable reply to *idn? (disabled by default)
   ++ren          - assert or unassert the REN signal
   ++ppoll        - conduct a parallel poll
   ++setvstr      - set custom version string (to identify controller, e.g. "GPIB-USB"). Max 47 chars, excess truncated.
   ++srqauto      - automatically condiuct serial poll when SRQ is asserted
   ++ton          - put controller in talk-only mode (send data only)
   ++verbose      - verbose (human readable) mode
*/

/*
   NOT YET IMPLEMENTED

   ++help     - show summary of commands
   ++myaddr   - set the controller address
*/

/*
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
   For further information about the AR488 see:
*/


/*********************************/
/***** CONFIGURATION SECTION *****/
/***** vvvvvvvvvvvvvvvvvvvvv *****/
// SEE >>>>> Config.h <<<<<
/***** ^^^^^^^^^^^^^^^^^^^^^ *****/
/***** CONFIGURATION SECTION *****/
/*********************************/


/***************************************/
/***** MACRO CONFIGURATION SECTION *****/
/***** vvvvvvvvvvvvvvvvvvvvvvvvvvv *****/
// SEE >>>>> Config.h <<<<<
/***** ^^^^^^^^^^^^^^^^^^^^^^^^^^^ *****/
/***** MACRO CONFIGURATION SECTION *****/
/***************************************/


/*************************************/
/***** MACRO STRUCTRURES SECTION *****/
/***** vvvvvvvvvvvvvvvvvvvvvvvvv *****/
#ifdef USE_MACROS

/*** DO NOT MODIFY ***/
/*** vvvvvvvvvvvvv ***/

/***** STARTUP MACRO *****/
const char startup_macro[] PROGMEM = {MACRO_0};

/***** Consts holding USER MACROS 1 - 9 *****/
const char macro_1 [] PROGMEM = {MACRO_1};
const char macro_2 [] PROGMEM = {MACRO_2};
const char macro_3 [] PROGMEM = {MACRO_3};
const char macro_4 [] PROGMEM = {MACRO_4};
const char macro_5 [] PROGMEM = {MACRO_5};
const char macro_6 [] PROGMEM = {MACRO_6};
const char macro_7 [] PROGMEM = {MACRO_7};
const char macro_8 [] PROGMEM = {MACRO_8};
const char macro_9 [] PROGMEM = {MACRO_9};


/* Macro pointer array */
const char * const macros[] PROGMEM = {
  startup_macro,
  macro_1,
  macro_2,
  macro_3,
  macro_4,
  macro_5,
  macro_6,
  macro_7,
  macro_8,
  macro_9
};

/*** ^^^^^^^^^^^^^ ***/
/*** DO NOT MODIFY ***/

#endif
/***** ^^^^^^^^^^^^^^^^^^^^ *****/
/***** MACRO CONFIG SECTION *****/
/********************************/



/**********************************/
/***** SERIAL PORT MANAGEMENT *****/
/***** vvvvvvvvvvvvvvvvvvvvvv *****/


#ifdef AR_CDC_SERIAL
  Serial_ *arSerial_ = &(AR_SERIAL_PORT);
  #ifndef DB_SERIAL_PORT
    Serial_ *dbSerial_ = arSerial_;
  #endif
#endif
#ifdef AR_HW_SERIAL
  HardwareSerial *arSerial_ = &(AR_SERIAL_PORT);
  #ifndef DB_SERIAL_PORT
    HardwareSerial *dbSerial_ = arSerial_;
  #endif
#endif
// Note: SoftwareSerial support conflicts with PCINT support
#ifdef AR_SW_SERIAL
  #include <SoftwareSerial.h>
  SoftwareSerial swArSerial(AR_SW_SERIAL_RX, AR_SW_SERIAL_TX);
  SoftwareSerial *arSerial_ = &swArSerial;
  #ifndef DB_SERIAL_PORT
    SoftwareSerial *dbSerial_ = arSerial;
  #endif
#endif

Stream *arSerial = (Stream*) arSerial_;

/***** Debug Port - if DB_SERIAL_PORT is set *****/

#ifdef DB_SERIAL_PORT
  #ifdef DB_CDC_SERIAL
    Serial_ *dbSerial_ = &(DB_SERIAL_PORT);
  #endif
  #ifdef DB_HW_SERIAL
    HardwareSerial *dbSerial_ = &(DB_SERIAL_PORT);
  #endif
  // Note: SoftwareSerial support conflicts with PCINT support
  #ifdef DB_SW_SERIAL
    #include <SoftwareSerial.h>
    SoftwareSerial swDbSerial(DB_SW_SERIAL_RX, DB_SW_SERIAL_TX);
    SoftwareSerial *dbSerial_ = &swDbSerial;
  #endif
#endif

Stream *dbSerial = (Stream*) dbSerial_;


#include "AR488.h"
#include "serialcomm.h"
#include "commands.h"
#include "gpib.h"


/***** ^^^^^^^^^^^^^^^^^^^^^^ *****/
/***** SERIAL PORT MANAGEMENT *****/
/**********************************/

/****** Global variables with volatile values related to controller state *****/
AR488Conf AR488;
AR488State AR488st;
CommandComm comm(*arSerial, AR488, AR488st);
GPIB gpib(*arSerial, AR488, AR488st, comm);

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
//#ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
//#endif

#ifdef USE_INTERRUPTS
  // Turn on interrupts on port
  interruptsEn();
#endif

  // Initialise parse buffer
  comm.flushPbuf();

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
  initAR488();
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
  // SN7516x IC support
#ifdef SN7516X
  pinMode(SN7516X_TE, OUTPUT);
  #ifdef SN7516X_DC
    pinMode(SN7516X_DC, OUTPUT);
  #endif
  if (AR488.cmode==2) {
    // Set controller mode on SN75161/2
    digitalWrite(SN7516X_TE, LOW);
    #ifdef SN7516X_DC
      digitalWrite(SN7516X_DC, LOW);
    #endif
    #ifdef SN7516X_SC
      digitalWrite(SN7516X_SC, HIGH);
    #endif
  }else{
    // Set listen mode on SN75161/2 (default)
    digitalWrite(SN7516X_TE, HIGH);
    #ifdef SN7516X_DC
      digitalWrite(SN7516X_DC, HIGH);
    #endif
    #ifdef SN7516X_SC
      digitalWrite(SN7516X_SC, LOW);
    #endif
  }
#endif

  // Initialize the interface in device mode
  if (AR488.cmode == 1) gpib.initDevice();

  // Initialize the interface in controller mode
  if (AR488.cmode == 2) gpib.initController();

  AR488st.isATN = false;
  AR488st.isSRQ = false;

#if defined(USE_MACROS) && defined(RUN_STARTUP)
  // Run startup macro
  execMacro(0);
#endif

//#ifdef SAY_HELLO
  arSerial->println(F("AR488 ready."));
//#endif

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
  if (AR488st.runMacro > 0) {
    execMacro(AR488st.runMacro);
    AR488st.runMacro = 0;
  }
#endif

/*** Pin Hooks ***/
/*
 * Not all boards support interrupts or have PCINTs. In this
 * case, use in-loop checking to detect when SRQ and ATN have
 * been signalled
 */
#ifndef USE_INTERRUPTS
  AR488st.isATN = (digitalRead(ATN)==LOW ? true : false);
  AR488st.isSRQ = (digitalRead(SRQ)==LOW ? true : false);
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
  if (AR488st.lnRdy == 1) {
		execCmd(comm.pBuf, comm.pbPtr, AR488, AR488st, comm);
  }

  // Controller mode:
  if (AR488.cmode == 2) {
    // lnRdy=2: received data - send it to the instrument...
    if (AR488st.lnRdy == 2) {
      gpib.sendToInstrument(comm.pBuf, comm.pbPtr);
      // Auto-read data from GPIB bus following any command
      if (AR488.amode == 1) {
        //        delay(10);
        gpib.gpibReceiveData();
      }
      // Auto-receive data from GPIB bus following a query command
      if (AR488.amode == 2 && AR488st.isQuery) {
        //        delay(10);
        gpib.gpibReceiveData();
        AR488st.isQuery = false;
      }
    }

    // Check status of SRQ and SPOLL if asserted
    if (AR488st.isSRQ && AR488st.isSrqa) {
			spoll_h(NULL, AR488, AR488st);
      AR488st.isSRQ = false;
    }

    // Continuous auto-receive data from GPIB bus
    if (AR488.amode == 3 && AR488st.aRead) gpib.gpibReceiveData();
  }

  // Device mode:
  if (AR488.cmode == 1) {
    if (AR488st.isTO) {
      if (AR488st.lnRdy == 2) gpib.sendToInstrument(comm.pBuf, comm.pbPtr);
    }else if (AR488st.isRO) {
      gpib.lonMode();
    }else{
      if (AR488st.isATN) gpib.attnRequired();
      if (AR488st.lnRdy == 2) gpib.sendToInstrument(comm.pBuf, comm.pbPtr);
    }
  }

  // Reset line ready flag
//  lnRdy = 0;

  // IDN query ?
  if (AR488st.sendIdn) {
    if (AR488.idn==1) arSerial->println(AR488.sname);
    if (AR488.idn==2) {
				arSerial->print(AR488.sname);
				arSerial->print("-");
				arSerial->println(AR488.serial);
		}
    AR488st.sendIdn = false;
  }

  // Check serial buffer
  AR488st.lnRdy = comm.serialIn_h();

  delayMicroseconds(5);
}
/***** END MAIN LOOP *****/


/***** Initialise the interface *****/
void initAR488() {
  // Set default values ({'\0'} sets version string array to null)
  AR488 = {false, false, 2, 0, 1, 0, 0, 0, 0, 1200, 0, {'\0'}, 0, 0, {'\0'}, 0, 0, false};
}



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

/***** If enabled, executes a macro *****/
#ifdef USE_MACROS
void execMacro(uint8_t idx) {
  char c;
  const char * macro = pgm_read_word(macros + idx);
  int ssize = strlen_P(macro);

  // Read characters from macro character array
  for (int i = 0; i < ssize; i++) {
    c = pgm_read_byte_near(macro + i);
    if (c == CR || c == LF || i == (ssize - 1)) {
      // Reached last character before NL. Add to buffer before processing
      if (i == ssize-1) {
        // Check buffer and add character
        if (pbPtr < (PBSIZE - 2)){
          addPbuf(c);
        }else{
          // Buffer full - clear and exit
          comm.flushPbuf();
          return;
        }
      }
      if (isCmd(pBuf)){
				execCmd(comm.pBuf, comm.strlen(pBuf), AR488, AR488st, comm);
      }else{
        gpib.sendToInstrument(pBuf, strlen(pBuf));
      }
      // Done - clear the buffer
      comm.flushPbuf();
    } else {
      // Check buffer and add character
      if (pbPtr < (PBSIZE - 2)) {
        addPbuf(c);
      } else {
        // Exceeds buffer size - clear buffer and exit
        i = ssize;
        return;
      }
    }
  }

  // Clear the buffer ready for serial input
  comm.flushPbuf();
}
#endif
