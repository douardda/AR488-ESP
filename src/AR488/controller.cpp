#include <Arduino.h>
#include "controller.h"
//#include "AR488.h"


// TODO: dbSerial

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


CommandComm::CommandComm(Stream& stream, AR488Conf& conf, AR488State& state):
		verbose(false),
		stream(stream),
		AR488(conf),
		AR488st(state)
{
}

/***** Add character to the buffer and parse *****/
uint8_t CommandComm::parseInput(char c) {

  uint8_t r = 0;

  // Read until buffer full (buffer-size - 2 characters)
  if (pbPtr < PBSIZE) {
		//if (AR488.isVerb) stream.print(c);  // Humans like to see what they are typing...
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
            //if (AR488.isVerb) showPrompt();
            return 0;
          } else {
						//if (AR488.isVerb) stream.println();  // Move to new line
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
//        if (isVerb) stream.print(c);
        break;
      // Something else?
      default: // any char other than defined above
//        if (isVerb) stream.print(c);  // Humans like to see what they are typing...
        // Buffer contains '++' (start of command). Stop sending data to serial port by halting GPIB receive.
        addPbuf(c);
        AR488st.isEsc = false;
    }
  }
  if (pbPtr >= PBSIZE) {
    if (isCmd(pBuf) && !r) {  // Command without terminator and buffer full
      if (AR488.isVerb) {
        stream.println(F("ERROR - Command buffer overflow!"));
      }
      flushPbuf();
    }else{  // Buffer contains data and is full, so process the buffer (send data via GPIB)
      dataBufferFull = true;
      r = 2;
    }
  }
  return r;
}


/***** Is this a command? *****/
bool CommandComm::isCmd(char *buffr) {
  if (buffr[0] == PLUS && buffr[1] == PLUS) {
#ifdef DEBUG1
    dbSerial->println(F("isCmd: Command detected."));
#endif
    return true;
  }
  return false;
}


/***** Is this an *idn? query? *****/
bool CommandComm::isIdnQuery(char *buffr) {
  // Check for upper or lower case *idn?
  if (strncasecmp(buffr, "*idn?", 5)==0) {
#ifdef DEBUG1
    dbSerial->println(F("isIdnQuery: Detected IDN query."));
#endif
    return true;
  }
  return false;
}


/***** Add character to the buffer *****/
void CommandComm::addPbuf(char c) {
  pBuf[pbPtr] = c;
  pbPtr++;
}


/***** Clear the parse buffer *****/
void CommandComm::flushPbuf() {
  memset(pBuf, '\0', PBSIZE);
  pbPtr = 0;
}

/***** Show a prompt *****/
void CommandComm::showPrompt() {
  // Print prompt
  // stream.println();
  stream.print("> ");
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
uint8_t CommandComm::serialIn_h() {
  uint8_t bufferStatus = 0;
  // Parse serial input until we have detected a line terminator
  while (stream.available() && bufferStatus==0) {   // Parse while characters available and line is not complete
	bufferStatus = parseInput(stream.read());
  }

#ifdef DEBUG1
  if (bufferStatus) {
    dbSerial->print(F("BufferStatus: "));
    dbSerial->println(bufferStatus);
  }
#endif

  return bufferStatus;
}


void CommandComm::reset() {
#ifdef WDTO_1S
  // Where defined, reset controller using watchdog timeout
  unsigned long tout;
  tout = millis() + 2000;
  wdt_enable(WDTO_1S);
  while (millis() < tout) {};
  // Should never reach here....
  if (AR488.isVerb) {
    arSerial->println(F("Reset FAILED."));
  };
#else
  // Otherwise restart program (soft reset)
#if defined(__AVR__)
  asm volatile ("  jmp 0");
#endif
#endif

}
