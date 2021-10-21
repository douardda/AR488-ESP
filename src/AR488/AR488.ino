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

/***** Debug Port *****/

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


/***** PARSE BUFFERS *****/
/*
 * Note: Arduino serial input buffer size is 64
 */
// Serial input parsing buffer
static const uint8_t PBSIZE = 128;
char pBuf[PBSIZE];
uint8_t pbPtr = 0;


/***** ^^^^^^^^^^^^^^^^^^^^^^ *****/
/***** SERIAL PORT MANAGEMENT *****/
/**********************************/

#include "AR488.h"

/****** Global variables with volatile values related to controller state *****/
union AR488conf AR488;
struct AR488state AR488st;

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
  digitalWrite(LED_BUILTIN, LOW);
#endif

#ifdef USE_INTERRUPTS
  // Turn on interrupts on port
  interruptsEn();
#endif

  // Initialise parse buffer
  flushPbuf();

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

#ifdef E2END
  // Read data from non-volatile memory
  //(will only read if previous config has already been saved)
//  epGetCfg();
  if (!isEepromClear()) {
    if (!epReadData(AR488.db, AR_CFG_SIZE)) {
      // CRC check failed - config data does not match EEPROM
      epErase();
      initAR488();
      epWriteData(AR488.db, AR_CFG_SIZE);
    }
  }
#endif

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
  if (AR488.cmode == 1) initDevice();

  // Initialize the interface in controller mode
  if (AR488.cmode == 2) initController();

  AR488st.isATN = false;
  AR488st.isSRQ = false;

#if defined(USE_MACROS) && defined(RUN_STARTUP)
  // Run startup macro
  execMacro(0);
#endif

#ifdef SAY_HELLO
  arSerial->println(F("AR488 ready."));
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
    execCmd(pBuf, pbPtr);
  }

  // Controller mode:
  if (AR488.cmode == 2) {
    // lnRdy=2: received data - send it to the instrument...
    if (AR488st.lnRdy == 2) {
      sendToInstrument(pBuf, pbPtr);
      // Auto-read data from GPIB bus following any command
      if (AR488.amode == 1) {
        //        delay(10);
        gpibReceiveData();
      }
      // Auto-receive data from GPIB bus following a query command
      if (AR488.amode == 2 && AR488st.isQuery) {
        //        delay(10);
        gpibReceiveData();
        AR488st.isQuery = false;
      }
    }

    // Check status of SRQ and SPOLL if asserted
    if (AR488st.isSRQ && AR488st.isSrqa) {
      spoll_h(NULL);
      AR488st.isSRQ = false;
    }

    // Continuous auto-receive data from GPIB bus
    if (AR488.amode == 3 && AR488st.aRead) gpibReceiveData();
  }

  // Device mode:
  if (AR488.cmode == 1) {
    if (AR488st.isTO) {
      if (AR488st.lnRdy == 2) sendToInstrument(pBuf, pbPtr);
    }else if (AR488st.isRO) {
      lonMode();
    }else{
      if (AR488st.isATN) attnRequired();
      if (AR488st.lnRdy == 2) sendToInstrument(pBuf, pbPtr);
    }
  }

  // Reset line ready flag
//  lnRdy = 0;

  // IDN query ?
  if (AR488st.sendIdn) {
    if (AR488.idn==1) arSerial->println(AR488.sname);
    if (AR488.idn==2) {arSerial->print(AR488.sname);arSerial->print("-");arSerial->println(AR488.serial);}
    AR488st.sendIdn = false;
  }

  // Check serial buffer
  AR488st.lnRdy = serialIn_h();

  delayMicroseconds(5);
}
/***** END MAIN LOOP *****/


/***** Initialise the interface *****/
void initAR488() {
  // Set default values ({'\0'} sets version string array to null)
  AR488 = {false, false, 2, 0, 1, 0, 0, 0, 0, 1200, 0, {'\0'}, 0, 0, {'\0'}, 0, 0};
}


/***** Initialise device mode *****/
void initDevice() {
  // Set GPIB control bus to device idle mode
  setGpibControls(DINI);

  // Initialise GPIB data lines (sets to INPUT_PULLUP)
  readGpibDbus();
}


/***** Initialise controller mode *****/
void initController() {
  // Set GPIB control bus to controller idle mode
  setGpibControls(CINI);  // Controller initialise state
  // Initialise GPIB data lines (sets to INPUT_PULLUP)
  readGpibDbus();
  // Assert IFC to signal controller in charge (CIC)
  ifc_h();
}


/***** Serial event handler *****/
/*
 * Note: the Arduino serial buffer is 64 characters long. Characters are stored in
 * this buffer until serialEvent_h() is called. parsedInput() takes a character at
 * a time and places it into the 256 character parse buffer whereupon it is parsed
 * to determine whether a command or data are present.
 * lnRdy=0: terminator not detected yet
 * lnRdy=1: terminator detected, sequence in parse buffer is a ++ command
 * lnRdy=2: terminator detected, sequence in parse buffer is data or direct instrument command
 */
uint8_t serialIn_h() {
  uint8_t bufferStatus = 0;
  // Parse serial input until we have detected a line terminator
  while (arSerial->available() && bufferStatus==0) {   // Parse while characters available and line is not complete
    bufferStatus = parseInput(arSerial->read());
  }

#ifdef DEBUG1
  if (bufferStatus) {
    dbSerial->print(F("BufferStatus: "));
    dbSerial->println(bufferStatus);
  }
#endif

  return bufferStatus;
}


/***** Detect ATN state *****/
/*
 * When interrupts are being used the state is automatically flagged when
 * the ATN interrupt is triggered. Where the interrupt cannot be used the
 * state of the ATN line needs to be checked.
 */
bool isAtnAsserted() {
#ifdef USE_INTERRUPTS
  if (AR488st.isATN) return true;
#else
  // ATN is LOW when asserted
  if (digitalRead(ATN) == LOW) return true;
#endif
  return false;
}


/*************************************/
/***** Device operation routines *****/
/*************************************/


/***** Unrecognized command *****/
void errBadCmd() {
  arSerial->println(F("Unrecognized command"));
}


/***** Add character to the buffer and parse *****/
uint8_t parseInput(char c) {

  uint8_t r = 0;

  // Read until buffer full (buffer-size - 2 characters)
  if (pbPtr < PBSIZE) {
    if (AR488st.isVerb) arSerial->print(c);  // Humans like to see what they are typing...
    // Actions on specific characters
    switch (c) {
      // Carriage return or newline? Then process the line
      case CR:
      case LF:
        // If escaped just add to buffer
        if (AR488st.isEsc) {
          addPbuf(c);
          AR488st.isEsc = false;
        } else {
          // Carriage return on blank line?
          // Note: for data CR and LF will always be escaped
          if (pbPtr == 0) {
            flushPbuf();
            if (AR488st.isVerb) showPrompt();
            return 0;
          } else {
            if (AR488st.isVerb) arSerial->println();  // Move to new line
#ifdef DEBUG1
            dbSerial->print(F("parseInput: Received ")); dbSerial->println(pBuf);
#endif
            // Buffer starts with ++ and contains at least 3 characters - command?
            if (pbPtr>2 && isCmd(pBuf) && !AR488st.isPlusEscaped) {
              // Exclamation mark (break read loop command)
              if (pBuf[2]==0x21) {
                r = 3;
                flushPbuf();
              // Otherwise flag command received and ready to process
              }else{
                r = 1;
              }
            // Buffer contains *idn? query and interface to respond
            }else if (pbPtr>3 && AR488.idn>0 && isIdnQuery(pBuf)){
              AR488st.sendIdn = true;
              flushPbuf();
            // Buffer has at least 1 character = instrument data to send to gpib bus
            }else if (pbPtr > 0) {
              r = 2;
            }
            AR488st.isPlusEscaped = false;
#ifdef DEBUG1
            dbSerial->print(F("R: "));dbSerial->println(r);
#endif
//            return r;
          }
        }
        break;
      case ESC:
        // Handle the escape character
        if (AR488st.isEsc) {
          // Add character to buffer and cancel escape
          addPbuf(c);
          AR488st.isEsc = false;
        } else {
          // Set escape flag
          AR488st.isEsc  = true;  // Set escape flag
        }
        break;
      case PLUS:
        if (AR488st.isEsc) {
          AR488st.isEsc = false;
          if (pbPtr < 2) AR488st.isPlusEscaped = true;
        }
        addPbuf(c);
//        if (isVerb) arSerial->print(c);
        break;
      // Something else?
      default: // any char other than defined above
//        if (isVerb) arSerial->print(c);  // Humans like to see what they are typing...
        // Buffer contains '++' (start of command). Stop sending data to serial port by halting GPIB receive.
        addPbuf(c);
        AR488st.isEsc = false;
    }
  }
  if (pbPtr >= PBSIZE) {
    if (isCmd(pBuf) && !r) {  // Command without terminator and buffer full
      if (AR488st.isVerb) {
        arSerial->println(F("ERROR - Command buffer overflow!"));
      }
      flushPbuf();
    }else{  // Buffer contains data and is full, so process the buffer (send data via GPIB)
      AR488st.dataBufferFull = true;
      r = 2;
    }
  }
  return r;
}


/***** Is this a command? *****/
bool isCmd(char *buffr) {
  if (buffr[0] == PLUS && buffr[1] == PLUS) {
#ifdef DEBUG1
    dbSerial->println(F("isCmd: Command detected."));
#endif
    return true;
  }
  return false;
}


/***** Is this an *idn? query? *****/
bool isIdnQuery(char *buffr) {
  // Check for upper or lower case *idn?
  if (strncasecmp(buffr, "*idn?", 5)==0) {
#ifdef DEBUG1
    dbSerial->println(F("isIdnQuery: Detected IDN query."));
#endif
    return true;
  }
  return false;
}


/***** ++read command detected? *****/
bool isRead(char *buffr) {
  char cmd[4];
  // Copy 2nd to 5th character
  for (int i = 2; i < 6; i++) {
    cmd[i - 2] = buffr[i];
  }
  // Compare with 'read'
  if (strncmp(cmd, "read", 4) == 0) return true;
  return false;
}


/***** Add character to the buffer *****/
void addPbuf(char c) {
  pBuf[pbPtr] = c;
  pbPtr++;
}


/***** Clear the parse buffer *****/
void flushPbuf() {
  memset(pBuf, '\0', PBSIZE);
  pbPtr = 0;
}



/***** Show a prompt *****/
void showPrompt() {
  // Print prompt
  arSerial->println();
  arSerial->print("> ");
}


/****** Send data to instrument *****/
/* Processes the parse buffer whenever a full CR or LF
 * and sends data to the instrument
 */
void sendToInstrument(char *buffr, uint8_t dsize) {

#ifdef DEBUG1
  dbSerial->print(F("sendToInstrument: Received for sending: ")); printHex(buffr, dsize);
#endif

  // Is this an instrument query command (string ending with ?)
  if (buffr[dsize-1] == '?') AR488st.isQuery = true;

  // Send string to instrument
  gpibSendData(buffr, dsize);
  // Clear data buffer full flag
  if (AR488st.dataBufferFull) AR488st.dataBufferFull = false;

  // Show a prompt on completion?
  if (AR488st.isVerb) showPrompt();

  // Flush the parse buffer
  flushPbuf();
  AR488st.lnRdy = 0;

#ifdef DEBUG1
  dbSerial->println(F("sendToInstrument: Sent."));
#endif

}


/***** Execute a command *****/
void execCmd(char *buffr, uint8_t dsize) {
  char line[PBSIZE];

  // Copy collected chars to line buffer
  memcpy(line, buffr, dsize);

  // Flush the parse buffer
  flushPbuf();
  AR488st.lnRdy = 0;

#ifdef DEBUG1
  dbSerial->print(F("execCmd: Command received: ")); printHex(line, dsize);
#endif

  // Its a ++command so shift everything two bytes left (ignore ++) and parse
  for (int i = 0; i < dsize-2; i++) {
    line[i] = line[i + 2];
  }
  // Replace last two bytes with a null (\0) character
  line[dsize - 2] = '\0';
  line[dsize - 1] = '\0';
#ifdef DEBUG1
  dbSerial->print(F("execCmd: Sent to the command processor: ")); printHex(line, dsize-2);
#endif
  // Execute the command
  if (AR488st.isVerb) arSerial->println(); // Shift output to next line
  getCmd(line);

  // Show a prompt on completion?
  if (AR488st.isVerb) showPrompt();
}


/***** Prints charaters as hex bytes *****/
void printHex(char *buffr, int dsize) {
  for (int i = 0; i < dsize; i++) {
    dbSerial->print(buffr[i], HEX); dbSerial->print(" ");
  }
  dbSerial->println();
}


/***** Check whether a parameter is in range *****/
/* Convert string to integer and check whether value is within
 * lowl to higl inclusive. Also returns converted text in param
 * to a uint16_t integer in rval. Returns true if successful,
 * false if not
*/
bool notInRange(char *param, uint16_t lowl, uint16_t higl, uint16_t &rval) {

  // Null string passed?
  if (strlen(param) == 0) return true;

  // Convert to integer
  rval = 0;
  rval = atoi(param);

  // Check range
  if (rval < lowl || rval > higl) {
    errBadCmd();
    if (AR488st.isVerb) {
      arSerial->print(F("Valid range is between "));
      arSerial->print(lowl);
      arSerial->print(F(" and "));
      arSerial->println(higl);
    }
    return true;
  }
  return false;
}


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
          flushPbuf();
          return;
        }
      }
      if (isCmd(pBuf)){
        execCmd(pBuf, strlen(pBuf));
      }else{
        sendToInstrument(pBuf, strlen(pBuf));
      }
      // Done - clear the buffer
      flushPbuf();
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
  flushPbuf();
}
#endif



/******************************************************/
/***** Device mode GPIB command handling routines *****/
/******************************************************/

/***** Attention handling routine *****/
/*
 * In device mode is invoked whenever ATN is asserted
 */
void attnRequired() {

  uint8_t db = 0;
  uint8_t stat = 0;
  bool mla = false;
  bool mta = false;
  bool spe = false;
  bool spd = false;
  bool eoiDetected = false;

  // Set device listner active state (assert NDAC+NRFD (low), DAV=INPUT_PULLUP)
  setGpibControls(DLAS);

#ifdef DEBUG5
  dbSerial->println(F("Answering attention!"));
#endif

  // Read bytes
//  while (isATN) {
  while (digitalRead(ATN)==LOW) {
    stat = gpibReadByte(&db, &eoiDetected);
    if (!stat) {

#ifdef DEBUG5
      dbSerial->println(db, HEX);
#endif

      // Device is addressed to listen
      if (AR488.paddr == (db ^ 0x20)) { // MLA = db^0x20
#ifdef DEBUG5
        dbSerial->println(F("attnRequired: Controller wants me to data accept data <<<"));
#endif
        mla = true;
      }

      // Device is addressed to talk
      if (AR488.paddr == (db ^ 0x40)) { // MLA = db^0x40
          // Call talk handler to send data
          mta = true;
#ifdef DEBUG5
          if (!spe) dbSerial->println(F("attnRequired: Controller wants me to send data >>>"));
#endif
      }

      // Serial poll enable request
      if (db==GC_SPE) spe = true;

      // Serial poll disable request
      if (db==GC_SPD) spd = true;

      // Unlisten
      if (db==GC_UNL) unl_h();

      // Untalk
      if (db==GC_UNT) unt_h();

    }

  }

#ifdef DEBUG5
  dbSerial->println(F("End ATN loop."));
#endif

  if (mla) {
#ifdef DEBUG5
    dbSerial->println(F("Listening..."));
#endif
    // Call listen handler (receive data)
    mla_h();
    mla = false;
  }

  // Addressed to listen?
  if (mta) {
    // Serial poll enabled
    if (spe) {
#ifdef DEBUG5
      dbSerial->println(F("attnRequired: Received serial poll enable."));
#endif
      spe_h();
      spe = false;
    // Otherwise just send data
    }else{
      mta_h();
      mta = false;
    }
  }

  // Serial poll disable received
  if (spd) {
#ifdef DEBUG5
    dbSerial->println(F("attnRequired: Received serial poll disable."));
#endif
    spd_h();
    mta = false;
    spe = false;
    spd = false;
  }

  // Finished attention - set controls to idle
  setGpibControls(DIDS);

#ifdef DEBUG5
  dbSerial->println(F("attnRequired: END attnReceived."));
#endif

}


/***** Device is addressed to listen - so listen *****/
void mla_h(){
  gpibReceiveData();
}


/***** Device is addressed to talk - so send data *****/
void mta_h(){
  if (AR488st.lnRdy == 2) sendToInstrument(pBuf, pbPtr);
}


/***** Selected Device Clear *****/
void sdc_h() {
  // If being addressed then reset
  if (AR488st.isVerb) arSerial->println(F("Resetting..."));
#ifdef DEBUG5
  dbSerial->print(F("Reset adressed to me: ")); dbSerial->println(aTl);
#endif
  if (AR488st.aTl) rst_h();
  if (AR488st.isVerb) arSerial->println(F("Reset failed."));
}


/***** Serial Poll Disable *****/
void spd_h() {
  if (AR488st.isVerb) arSerial->println(F("<- serial poll request ended."));
}


/***** Serial Poll Enable *****/
void spe_h() {
  if (AR488st.isVerb) arSerial->println(F("Serial poll request received from controller ->"));
  gpibSendStatus();
  if (AR488st.isVerb) arSerial->println(F("Status sent."));
  // Clear the SRQ bit
  AR488.stat = AR488.stat & ~0x40;
  // Clear the SRQ signal
  clrSrqSig();
  if (AR488st.isVerb) arSerial->println(F("SRQ bit cleared (if set)."));
}


/***** Unlisten *****/
void unl_h() {
  // Stop receiving and go to idle
#ifdef DEBUG5
  dbSerial->println(F("Unlisten received."));
#endif
  AR488st.rEoi = false;
  AR488st.tranBrk = 3;  // Stop receving transmission
}


/***** Untalk *****/
void unt_h() {
  // Stop sending data and go to idle
#ifdef DEBUG5
  dbSerial->println(F("Untalk received."));
#endif
}


void lonMode(){

  gpibReceiveData();

  // Clear the buffer to prevent it getting blocked
  if (AR488st.lnRdy==2) flushPbuf();

}


/***************************************/
/***** GPIB DATA HANDLING ROUTINES *****/
/***************************************/

/*****  Send a single byte GPIB command *****/
bool gpibSendCmd(uint8_t cmdByte) {

  bool stat = false;

  // Set lines for command and assert ATN
  setGpibControls(CCMS);

  // Send the command
  stat = gpibWriteByte(cmdByte);
  if (stat && AR488st.isVerb) {
    arSerial->print(F("gpibSendCmd: failed to send command "));
    arSerial->print(cmdByte, HEX);
    arSerial->println(F(" to device"));
  }

  // Return to controller idle state
  //  setGpibControls(CIDS);
  // NOTE: this breaks serial poll

  return stat ? ERR : OK;
}


/***** Send the status byte *****/
void gpibSendStatus() {
  // Have been addressed and polled so send the status byte
  if (AR488st.isVerb) {
    arSerial->print(F("Sending status byte: "));
    arSerial->println(AR488.stat);
  };
  setGpibControls(DTAS);
  gpibWriteByte(AR488.stat);
  setGpibControls(DIDS);
}


/***** Send a series of characters as data to the GPIB bus *****/
void gpibSendData(char *data, uint8_t dsize) {

  bool err = false;

  // If lon is turned on we cannot send data so exit
  if (AR488st.isRO) return;

  // Controler can unlisten bus and address devices
  if (AR488.cmode == 2) {

    if (AR488st.deviceAddressing) {
      // Address device to listen
      if (addrDev(AR488.paddr, 0)) {
        if (AR488st.isVerb) {
          arSerial->print(F("gpibSendData: failed to address device "));
          arSerial->print(AR488.paddr);
          arSerial->println(F(" to listen"));
        }
        return;
      }
    }

    AR488st.deviceAddressing = AR488st.dataBufferFull ? false : true;

#ifdef DEBUG3
    dbSerial->println(F("Device addressed."));
#endif

    // Set control lines to write data (ATN unasserted)
    setGpibControls(CTAS);

  } else {
    setGpibControls(DTAS);
  }
#ifdef DEBUG3
  dbSerial->println(F("Set write data mode."));
  dbSerial->print(F("Send->"));
#endif

  // Write the data string
  for (int i = 0; i < dsize; i++) {
    // If EOI asserting is on
    if (AR488.eoi) {
      // Send all characters
      err = gpibWriteByte(data[i]);
    } else {
      // Otherwise ignore non-escaped CR, LF and ESC
      if ((data[i] != CR) || (data[i] != LF) || (data[i] != ESC)) err = gpibWriteByte(data[i]);
    }
#ifdef DEBUG3
    dbSerial->print(data[i]);
#endif
    if (err) break;
  }

#ifdef DEBUG3
  dbSerial->println("<-End.");
#endif

  if (!err) {
    // Write terminators according to EOS setting
    // Do we need to write a CR?
    if ((AR488.eos & 0x2) == 0) {
      gpibWriteByte(CR);
#ifdef DEBUG3
      dbSerial->println(F("Appended CR"));
#endif
    }
    // Do we need to write an LF?
    if ((AR488.eos & 0x1) == 0) {
      gpibWriteByte(LF);
#ifdef DEBUG3
      dbSerial->println(F("Appended LF"));
#endif
    }
  }

  // If EOI enabled and no more data to follow then assert EOI
  if (AR488.eoi && !AR488st.dataBufferFull) {
    setGpibState(0b00000000, 0b00010000, 0);
    //    setGpibState(0b00010000, 0b00000000, 0b00010000);
    delayMicroseconds(40);
    setGpibState(0b00010000, 0b00010000, 0);
    //    setGpibState(0b00010000, 0b00010000, 0b00010000);
#ifdef DEBUG3
    dbSerial->println(F("Asserted EOI"));
#endif
  }

  if (AR488.cmode == 2) {   // Controller mode
    if (!err) {
      if (AR488st.deviceAddressing) {
        // Untalk controller and unlisten bus
        if (uaddrDev()) {
          if (AR488st.isVerb) arSerial->println(F("gpibSendData: Failed to unlisten bus"));
        }

#ifdef DEBUG3
        dbSerial->println(F("Unlisten done"));
#endif
      }
    }

    // Controller - set lines to idle?
    setGpibControls(CIDS);

  }else{    // Device mode
    // Set control lines to idle
    setGpibControls(DIDS);
  }

#ifdef DEBUG3
    dbSerial->println(F("<- End of send."));
#endif

}


/***** Receive data from the GPIB bus ****/
/*
 * Readbreak:
 * 5 - EOI detected
 * 7 - command received via serial
 */
bool gpibReceiveData() {

  uint8_t r = 0; //, db;
  uint8_t bytes[3] = {0};
  uint8_t eor = AR488.eor&7;
  int x = 0;
  bool eoiStatus;
  bool eoiDetected = false;

  // Reset transmission break flag
  AR488st.tranBrk = 0;

  // Set status of EOI detection
  eoiStatus = AR488st.rEoi; // Save status of rEoi flag
  if (AR488.eor==7) AR488st.rEoi = true;    // Using EOI as terminator

  // Set up for reading in Controller mode
  if (AR488.cmode == 2) {   // Controler mode
    // Address device to talk
    if (addrDev(AR488.paddr, 1)) {
      if (AR488st.isVerb) {
        arSerial->print(F("Failed to address the device"));
        arSerial->print(AR488.paddr);
        arSerial->println(F(" to talk"));
      }
    }
    // Wait for instrument ready
    Wait_on_pin_state(HIGH, NRFD, AR488.rtmo);
    // Set GPIB control lines to controller read mode
    setGpibControls(CLAS);

  // Set up for reading in Device mode
  } else {  // Device mode
    // Set GPIB controls to device read mode
    setGpibControls(DLAS);
    AR488st.rEoi = true;  // In device mode we read with EOI by default
  }

#ifdef DEBUG7
    dbSerial->println(F("gpibReceiveData: Start listen ->"));
    dbSerial->println(F("Before loop flags:"));
    dbSerial->print(F("TRNb: "));
    dbSerial->println(tranBrk);
    dbSerial->print(F("rEOI: "));
    dbSerial->println(rEoi);
    dbSerial->print(F("ATN:  "));
    dbSerial->println(isAtnAsserted() ? 1 : 0);
#endif

  // Ready the data bus
  readyGpibDbus();

  // Perform read of data (r=0: data read OK; r>0: GPIB read error);
  while (r == 0) {

    // Tranbreak > 0 indicates break condition
    if (AR488st.tranBrk > 0) break;

    // ATN asserted
    if (isAtnAsserted()) break;

    // Read the next character on the GPIB bus
    r = gpibReadByte(&bytes[0], &eoiDetected);

    // When reading with amode=3 or EOI check serial input and break loop if neccessary
    if ((AR488.amode==3) || AR488st.rEoi) AR488st.lnRdy = serialIn_h();

    // Line terminator detected (loop breaks on command being detected or data buffer full)
    if (AR488st.lnRdy > 0) {
      AR488st.aRead = false;  // Stop auto read
      break;
    }

#ifdef DEBUG7
    if (eoiDetected) dbSerial->println(F("\r\nEOI detected."));
#endif

    // If break condition ocurred or ATN asserted then break here
    if (isAtnAsserted()) break;

#ifdef DEBUG7
    dbSerial->print(bytes[0], HEX), dbSerial->print(' ');
#else
    // Output the character to the serial port
    arSerial->print((char)bytes[0]);
#endif

    // Byte counter
    x++;

    // EOI detection enabled and EOI detected?
    if (AR488st.rEoi) {
      if (eoiDetected) break;
    }else{
      // Has a termination sequence been found ?
      if (isTerminatorDetected(bytes, eor)) break;
    }

    // Stop on timeout
    if (r > 0) break;

    // Shift last three bytes in memory
    bytes[2] = bytes[1];
    bytes[1] = bytes[0];
  }

#ifdef DEBUG7
  dbSerial->println();
  dbSerial->println(F("After loop flags:"));
  dbSerial->print(F("ATN: "));
  dbSerial->println(isAtnAsserted());
  dbSerial->print(F("TMO:  "));
  dbSerial->println(r);
#endif

  // End of data - if verbose, report how many bytes read
  if (AR488st.isVerb) {
    arSerial->print(F("Bytes read: "));
    arSerial->println(x);
  }

  // Detected that EOI has been asserted
  if (eoiDetected) {
    if (AR488st.isVerb) arSerial->println(F("EOI detected!"));
    // If eot_enabled then add EOT character
    if (AR488.eot_en) arSerial->print(AR488.eot_ch);
  }

  // Return rEoi to previous state
  AR488st.rEoi = eoiStatus;

  // Verbose timeout error
  if (r > 0) {
    if (AR488st.isVerb && r == 1) arSerial->println(F("Timeout waiting for sender!"));
    if (AR488st.isVerb && r == 2) arSerial->println(F("Timeout waiting for transfer to complete!"));
  }

  // Return controller to idle state
  if (AR488.cmode == 2) {

    // Untalk bus and unlisten controller
    if (uaddrDev()) {
      if (AR488st.isVerb) arSerial->print(F("gpibSendData: Failed to untalk bus"));
    }

    // Set controller back to idle state
    setGpibControls(CIDS);

  } else {
    // Set device back to idle state
    setGpibControls(DIDS);
  }

#ifdef DEBUG7
    dbSerial->println(F("<- End listen."));
#endif

  // Reset flags
//  isReading = false;
  if (AR488st.tranBrk > 0) AR488st.tranBrk = 0;

  if (r > 0) return ERR;

  return OK;
}


/***** Check for terminator *****/
bool isTerminatorDetected(uint8_t bytes[3], uint8_t eor_sequence){
  if (AR488st.rEbt) {
    // Stop on specified <char> if appended to ++read command
    if (bytes[0] == AR488st.eByte) return true;
  }else{
    // Look for specified terminator (CR+LF by default)
    switch (eor_sequence) {
      case 0:
          // CR+LF terminator
          if (bytes[0]==LF && bytes[1]==CR) return true;
          break;
      case 1:
          // CR only as terminator
          if (bytes[0]==CR) return true;
          break;
      case 2:
          // LF only as terminator
          if (bytes[0]==LF) return true;
          break;
      case 3:
          // No terminator (will rely on timeout)
          break;
      case 4:
          // Keithley can use LF+CR instead of CR+LF
          if (bytes[0]==CR && bytes[1]==LF) return true;
          break;
      case 5:
          // Solarton (possibly others) can also use ETX (0x03)
          if (bytes[0]==0x03) return true;
          break;
      case 6:
          // Solarton (possibly others) can also use CR+LF+ETX (0x03)
          if (bytes[0]==0x03 && bytes[1]==LF && bytes[2]==CR) return true;
          break;
      default:
          // Use CR+LF terminator by default
          if (bytes[0]==LF && bytes[1]==CR) return true;
          break;
      }
  }
  return false;
}


/***** Read a SINGLE BYTE of data from the GPIB bus using 3-way handshake *****/
/*
 * (- this function is called in a loop to read data    )
 * (- the GPIB bus must already be configured to listen )
 */
uint8_t gpibReadByte(uint8_t *db, bool *eoi) {
  bool atnStat = (digitalRead(ATN) ? false : true); // Set to reverse, i.e. asserted=true; unasserted=false;
  *eoi = false;

  // Unassert NRFD (we are ready for more data)
  setGpibState(0b00000100, 0b00000100, 0);

  // ATN asserted and just got unasserted - abort - we are not ready yet
  if (atnStat && (digitalRead(ATN)==HIGH)) {
    setGpibState(0b00000000, 0b00000100, 0);
    return 3;
  }

  // Wait for DAV to go LOW indicating talker has finished setting data lines..
  if (Wait_on_pin_state(LOW, DAV, AR488.rtmo))  {
    if (AR488st.isVerb) arSerial->println(F("gpibReadByte: timeout waiting for DAV to go LOW"));
    setGpibState(0b00000000, 0b00000100, 0);
    // No more data for you?
    return 1;
  }

  // Assert NRFD (NOT ready - busy reading data)
  setGpibState(0b00000000, 0b00000100, 0);

  // Check for EOI signal
  if (AR488st.rEoi && digitalRead(EOI) == LOW) *eoi = true;

  // read from DIO
  *db = readGpibDbus();

  // Unassert NDAC signalling data accepted
  setGpibState(0b00000010, 0b00000010, 0);

  // Wait for DAV to go HIGH indicating data no longer valid (i.e. transfer complete)
  if (Wait_on_pin_state(HIGH, DAV, AR488.rtmo))  {
    if (AR488st.isVerb) arSerial->println(F("gpibReadByte: timeout waiting DAV to go HIGH"));
    return 2;
  }

  // Re-assert NDAC - handshake complete, ready to accept data again
  setGpibState(0b00000000, 0b00000010, 0);

  // GPIB bus DELAY
  delayMicroseconds(AR488.tmbus);

  return 0;

}


/***** Write a SINGLE BYTE onto the GPIB bus using 3-way handshake *****/
/*
 * (- this function is called in a loop to send data )
 */
bool gpibWriteByte(uint8_t db) {

  bool err;

  err = gpibWriteByteHandshake(db);

  // Unassert DAV
  setGpibState(0b00001000, 0b00001000, 0);

  // Reset the data bus
  setGpibDbus(0);

  // GPIB bus DELAY
  delayMicroseconds(AR488.tmbus);

  // Exit successfully
  return err;
}


/***** GPIB send byte handshake *****/
bool gpibWriteByteHandshake(uint8_t db) {

    // Wait for NDAC to go LOW (indicating that devices are at attention)
  if (Wait_on_pin_state(LOW, NDAC, AR488.rtmo)) {
    if (AR488st.isVerb) arSerial->println(F("gpibWriteByte: timeout waiting for receiver attention [NDAC asserted]"));
    return true;
  }
  // Wait for NRFD to go HIGH (indicating that receiver is ready)
  if (Wait_on_pin_state(HIGH, NRFD, AR488.rtmo))  {
    if (AR488st.isVerb) arSerial->println(F("gpibWriteByte: timeout waiting for receiver ready - [NRFD unasserted]"));
    return true;
  }

  // Place data on the bus
  setGpibDbus(db);

  // Assert DAV (data is valid - ready to collect)
  setGpibState(0b00000000, 0b00001000, 0);

  // Wait for NRFD to go LOW (receiver accepting data)
  if (Wait_on_pin_state(LOW, NRFD, AR488.rtmo))  {
    if (AR488st.isVerb) arSerial->println(F("gpibWriteByte: timeout waiting for data to be accepted - [NRFD asserted]"));
    return true;
  }

  // Wait for NDAC to go HIGH (data accepted)
  if (Wait_on_pin_state(HIGH, NDAC, AR488.rtmo))  {
    if (AR488st.isVerb) arSerial->println(F("gpibWriteByte: timeout waiting for data accepted signal - [NDAC unasserted]"));
    return true;
  }

  return false;
}


/***** Untalk bus then address a device *****/
/*
 * dir: 0=listen; 1=talk;
 */
bool addrDev(uint8_t addr, bool dir) {
  if (gpibSendCmd(GC_UNL)) return ERR;
  if (dir) {
    // Device to talk, controller to listen
    if (gpibSendCmd(GC_TAD + addr)) return ERR;
    if (gpibSendCmd(GC_LAD + AR488.caddr)) return ERR;
  } else {
    // Device to listen, controller to talk
    if (gpibSendCmd(GC_LAD + addr)) return ERR;
    if (gpibSendCmd(GC_TAD + AR488.caddr)) return ERR;
  }
  return OK;
}


/***** Unaddress a device (untalk bus) *****/
bool uaddrDev() {
  // De-bounce
  delayMicroseconds(30);
  // Utalk/unlisten
  if (gpibSendCmd(GC_UNL)) return ERR;
  if (gpibSendCmd(GC_UNT)) return ERR;
  return OK;
}


/**********************************/
/*****  GPIB CONTROL ROUTINES *****/
/**********************************/


/***** Wait for "pin" to reach a specific state *****/
/*
 * Returns false on success, true on timeout.
 * Pin MUST be set as INPUT_PULLUP otherwise it will not change and simply time out!
 */
bool Wait_on_pin_state(uint8_t state, uint8_t pin, int interval) {

  unsigned long timeout = millis() + interval;
  bool atnStat = (digitalRead(ATN) ? false : true); // Set to reverse - asserted=true; unasserted=false;

  while (digitalRead(pin) != state) {
    // Check timer
    if (millis() >= timeout) return true;
    // ATN status was asserted but now unasserted so abort
    if (atnStat && (digitalRead(ATN)==HIGH)) return true;
    //    if (digitalRead(EOI)==LOW) tranBrk = 2;
  }
  return false;        // = no timeout therefore succeeded!
}

/***** Control the GPIB bus - set various GPIB states *****/
/*
 * state is a predefined state (CINI, CIDS, CCMS, CLAS, CTAS, DINI, DIDS, DLAS, DTAS);
 * Bits control lines as follows: 8-ATN, 7-SRQ, 6-REN, 5-EOI, 4-DAV, 3-NRFD, 2-NDAC, 1-IFC
 * setGpibState byte1 (databits) : State - 0=LOW, 1=HIGH/INPUT_PULLUP; Direction - 0=input, 1=output;
 * setGpibState byte2 (mask)     : 0=unaffected, 1=enabled
 * setGpibState byte3 (mode)     : 0=set pin state, 1=set pin direction
 */
void setGpibControls(uint8_t state) {

  // Switch state
  switch (state) {
    // Controller states

    case CINI:  // Initialisation
      // Set pin direction
      setGpibState(0b10111000, 0b11111111, 1);
      // Set pin state
      setGpibState(0b11011111, 0b11111111, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
  #ifdef SN7516X_DC
        digitalWrite(SN7516X_DC,LOW);
  #endif
  #ifdef SN7516X_SC
        digitalWrite(SN7516X_SC,HIGH);
  #endif
#endif
#ifdef DEBUG2
      dbSerial->println(F("Initialised GPIB control mode"));
#endif
      break;

    case CIDS:  // Controller idle state
      setGpibState(0b10111000, 0b10011110, 1);
      setGpibState(0b11011111, 0b10011110, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
#endif
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines to idle state"));
#endif
      break;

    case CCMS:  // Controller active - send commands
      setGpibState(0b10111001, 0b10011111, 1);
      setGpibState(0b01011111, 0b10011111, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines for sending a command"));
#endif
      break;

    case CLAS:  // Controller - read data bus
      // Set state for receiving data
      setGpibState(0b10100110, 0b10011110, 1);
      setGpibState(0b11011000, 0b10011110, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
#endif
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines for reading data"));
#endif
      break;

    case CTAS:  // Controller - write data bus
      setGpibState(0b10111001, 0b10011110, 1);
      setGpibState(0b11011111, 0b10011110, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines for writing data"));
#endif
      break;

    /* Bits control lines as follows: 8-ATN, 7-SRQ, 6-REN, 5-EOI, 4-DAV, 3-NRFD, 2-NDAC, 1-IFC */

    // Listener states
    case DINI:  // Listner initialisation
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
  #ifdef SN7516X_DC
        digitalWrite(SN7516X_DC,HIGH);
  #endif
  #ifdef SN7516X_SC
        digitalWrite(SN7516X_SC,LOW);
  #endif
#endif
      setGpibState(0b00000000, 0b11111111, 1);
      setGpibState(0b11111111, 0b11111111, 0);
#ifdef DEBUG2
      dbSerial->println(F("Initialised GPIB listener mode"));
#endif
      break;

    case DIDS:  // Device idle state
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif
      setGpibState(0b00000000, 0b00001110, 1);
      setGpibState(0b11111111, 0b00001110, 0);
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines to idle state"));
#endif
      break;

    case DLAS:  // Device listner active (actively listening - can handshake)
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
#endif
      setGpibState(0b00000110, 0b00001110, 1);
      setGpibState(0b11111001, 0b00001110, 0);
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines to idle state"));
#endif
      break;

    case DTAS:  // Device talker active (sending data)
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif
      setGpibState(0b00001000, 0b00001110, 1);
      setGpibState(0b11111001, 0b00001110, 0);
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines for listening as addresed device"));
#endif
      break;
#ifdef DEBUG2

    default:
      // Should never get here!
      //      setGpibState(0b00000110, 0b10111001, 0b11111111);
      dbSerial->println(F("Unknown GPIB state requested!"));
#endif
  }

  // Save state
  AR488st.cstate = state;

  // GPIB bus delay (to allow state to settle)
  delayMicroseconds(AR488.tmbus);

}
