/***** If enabled, executes a macro *****/
#ifdef USE_MACROS
#include "macros.h"
#include "AR488.h"
#include "gpib.h"
#include "commands.h"


void execMacro(uint8_t idx, GPIB& gpib) {
  char c;
  const char * macro = (char*)pgm_read_word(macros + idx);
  int ssize = strlen_P(macro);

  // Read characters from macro character array
  for (int i = 0; i < ssize; i++) {
    c = pgm_read_byte_near(macro + i);
    if (c == CR || c == LF || i == (ssize - 1)) {
      // Reached last character before NL. Add to buffer before processing
      if (i == ssize-1) {
        // Check buffer and add character
        if (gpib.controller.pbPtr < (PBSIZE - 2)){
          gpib.controller.addPbuf(c);
        }else{
          // Buffer full - clear and exit
          gpib.controller.flushPbuf();
          return;
        }
      }
      if (gpib.controller.isCmd(gpib.controller.pBuf)){
		execCmd(gpib.controller.pBuf, strlen(gpib.controller.pBuf), gpib);
      }else{
        gpib.sendToInstrument(gpib.controller.pBuf, strlen(gpib.controller.pBuf));
      }
      // Done - clear the buffer
      gpib.controller.flushPbuf();
    } else {
      // Check buffer and add character
      if (gpib.controller.pbPtr < (PBSIZE - 2)) {
        gpib.controller.addPbuf(c);
      } else {
        // Exceeds buffer size - clear buffer and exit
        i = ssize;
        return;
      }
    }
  }

  // Clear the buffer ready for serial input
  gpib.controller.flushPbuf();
}
#endif
