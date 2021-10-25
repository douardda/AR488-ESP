#if !defined(SERIALCOMM_H)
#define SERIALCOMM_H

#include <Arduino.h>
#include "AR488.h"

#define PBSIZE 128

class CommandComm {
public:
  CommandComm(Stream&, AR488Conf&, AR488State&);
  uint8_t parseInput(char c);
  bool isCmd(char *buffr);
  bool isIdnQuery(char *buffr);
  void addPbuf(char c);
  void flushPbuf();
  void showPrompt();
  uint8_t serialIn_h();
  void reset();

private:
  bool verbose;
  Stream &stream;
  AR488Conf &AR488;
  AR488State &AR488st;

/***** PARSE BUFFERS *****/
/*
 * Note: Arduino serial input buffer size is 64
 */
// communication stream input parsing buffer
public:  // TODO: better than this...
  char pBuf[PBSIZE];
  uint8_t pbPtr = 0;
  bool dataBufferFull = false;
  uint8_t lnRdy = 0;  // CR/LF terminated line ready to process
  bool aRead = false; // GPIB data read in progress

  // Escaped character flag
  bool isEsc = false;           // Charcter escaped
  bool isPlusEscaped = false;   // Plus escaped
  bool isRO = false;            // Read only mode flag
  bool isTO = false;            // Talk only mode flag
  bool isSrqa = false;          // SRQ auto mode

};
#endif
