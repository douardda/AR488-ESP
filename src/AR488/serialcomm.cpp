#include <Arduino.h>
#include "serialcomm.h"
//#include "AR488.h"


// TODO: dbSerial


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
      AR488st.dataBufferFull = true;
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
