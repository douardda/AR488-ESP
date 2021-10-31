/***** If enabled, executes a macro *****/
#ifdef USE_MACROS
#include "macros.h"
#include "AR488.h"
#include "gpib.h"
#include "commands.h"

#if defined(ESP32)
void execMacro(uint8_t idx, Controller& controller) {
  Preferences pref;
  String macro;
  char key[] = {'\x00', '\x00'};
  char c;
  int ssize;

  key[0] = 48 + idx;
  pref.begin("macros", true);
  macro = pref.getString(key);
  pref.end();

  macro.trim();
  ssize = macro.length();
  if (ssize) {
	for (int i=0; i<ssize; ++i) {
	  c = macro[i];
	  if (c == CR || c == LF || i == (ssize - 1)) {
		// Reached last character before NL. Add to buffer before processing
		if (i == ssize-1) {
		  // Check buffer and add character
		  if (controller.pbPtr < (PBSIZE - 2)) {
			controller.addPbuf(c);
		  } else {
			// Buffer full - clear and exit
			controller.flushPbuf();
			controller.showPrompt();
			return;
		  }
		}
		if (controller.isCmd(controller.pBuf)) {
		  controller.execCmd();
		} else {
		  controller.sendToInstrument();
		}
		// Done - clear the buffer
		controller.flushPbuf();
	  } else {
		// Check buffer and add character
		if (controller.pbPtr < (PBSIZE - 2)) {
		  controller.addPbuf(c);
		} else {
		  // Exceeds buffer size - clear buffer and exit
		  i = ssize;
		  controller.showPrompt();
		}
	  }
	}
	controller.flushPbuf();
	controller.showPrompt();
  }
}

void saveMacro(uint8_t idx, char *macro, Controller& controller) {
  Preferences pref;

  pref.begin("macros", true);
}

void deleteMacro(uint8_t idx, Controller& controller) {
  Preferences pref;

  pref.begin("macros", true);
}

#else
void execMacro(uint8_t idx, Controller& controller) {
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
        if (controller.pbPtr < (PBSIZE - 2)){
          controller.addPbuf(c);
        }else{
          // Buffer full - clear and exit
          controller.flushPbuf();
		  controller.showPrompt();
          return;
        }
      }
      if (controller.isCmd(controller.pBuf)){
		controller.execCmd();
      }else{
        controller.sendToInstrument();
      }
      // Done - clear the buffer
      controller.flushPbuf();
    } else {
      // Check buffer and add character
      if (controller.pbPtr < (PBSIZE - 2)) {
        controller.addPbuf(c);
      } else {
        // Exceeds buffer size - clear buffer and exit
        i = ssize;
		controller.showPrompt();
      }
    }
  }

  // Clear the buffer ready for serial input
  controller.flushPbuf();
  controller.showPrompt();

}
#endif  // non ESP32

#endif
