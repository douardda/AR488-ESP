/***** If enabled, executes a macro *****/
#ifdef USE_MACROS
#include "macros.h"
#include "AR488.h"
#include "gpib.h"
#include "commands.h"


#if defined(ESP32)

#include <Preferences.h>

bool isMacro(uint8_t idx) {
  Preferences pref;
  const char key[] = {(char)(48+idx), '\x00'};
  bool iskey;

  pref.begin("macros", true);
  iskey = pref.isKey(key);
  pref.end();
  return iskey;
}

String getMacro(uint8_t idx) {
  Preferences pref;
  const char key[] = {(char)(48+idx), '\x00'};
  String macro;

  pref.begin("macros", true);
  if (pref.isKey(key))
	macro = pref.getString(key);
  pref.end();
  return macro;
}

void saveMacro(uint8_t idx, String& macro) {
  Preferences pref;
  const char key[] = {(char)(48+idx), '\x00'};

  pref.begin("macros", false);
  pref.putString(key, macro);
  pref.end();
}

void deleteMacro(uint8_t idx) {
  Preferences pref;
  const char key[] = {(char)(48+idx), '\x00'};
  pref.begin("macros", false);
  pref.remove(key);
  pref.end();
}

#else  // non ESP32: EEPROM based macro implementation

#include "AR488_Eeprom.h"
#include "controller.h"


int addressForMacro(uint8_t idx) {
  return EESTART + sizeof(AR488Conf) + (2 + MACRO_MAX_LEN) * idx;
}

bool isMacro(uint8_t idx) {
  return (getMacro(idx).length() > 0);
}

String getMacro(uint8_t idx) {
  char cmacro[MACRO_MAX_LEN];
  if (epGet(addressForMacro(idx), cmacro))
	return String(cmacro);
  else
	return String();
}

void saveMacro(uint8_t idx, String& macro) {
  char cmacro[MACRO_MAX_LEN];
  strncpy(cmacro, macro.c_str(), MACRO_MAX_LEN);
  epPut(addressForMacro(idx), cmacro);
}

void deleteMacro(uint8_t idx) {
  epPut(addressForMacro(idx), "");
}

#endif  // non ESP32

void execMacro(uint8_t idx, Controller& controller) {
  String macro;
  macro = getMacro(idx);
  execMacro(macro, controller);
}

void execMacro(String& macro, Controller& controller) {
  char c;

  macro.trim();
  int ssize = macro.length();
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

#endif
