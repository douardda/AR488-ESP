#include <Arduino.h>
#include "controller.h"
#include "gpib.h"
#include "commands.h"
#include "serial.h"

#ifdef ESP32
#include <Preferences.h>
#elif defined(E2END)
#include "AR488_Eeprom.h"
#include <EEPROM.h>
#endif

#ifdef AR488_WIFI_ENABLE
#include <WiFi.h>
#include <WiFiMulti.h>
#endif

#if defined(USE_MACROS)
#include "macros.h"
#endif

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
   ++auto         - automatically request talk and read response (0=off; 1=Prologix; 2=onquery; 3=continuous)
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


Controller::Controller():
	serialstream(NULL), btstream(NULL), tcpstream(NULL)
#ifdef AR488_WIFI_ENABLE
	, wifimulti(), wifiserver(23), serverclient()
#endif
{
  serialstream = getSerialStream();
  cmdstream = serialstream;
#ifdef AR488_BT_ENABLE
  btstream = getBTSerialStream();
#endif
}

/***** Add character to the buffer and parse *****/
uint8_t Controller::parseInput(char c) {

  uint8_t r = 0;

  // Read until buffer full (buffer-size - 2 characters)
  if (pbPtr < PBSIZE) {
    // Actions on specific characters
    switch (c) {
      // Carriage return or newline? Then process the line
      case LF:
		// just ignore LF for now...
		break;
      case CR:
        // If escaped just add to buffer
        if (isEsc) {
          addPbuf(c);
          isEsc = false;
        } else {
          // Carriage return on blank line?
          // Note: for data CR and LF will always be escaped
          if (pbPtr == 0) {
#ifdef USE_MACROS
			if (editMacro < NUM_MACROS) {
			  flushPbuf();
			  return 4;  //
			}
#endif
            flushPbuf();
            return 0;
          } else {
            // Buffer starts with ++ and contains at least 3 characters - command?
            if (pbPtr>2 && isCmd(pBuf) && !isPlusEscaped) {
              // Exclamation mark (break read loop command)
              if (pBuf[2]==0x21) {
                r = 3;
                flushPbuf();
              // Otherwise flag command received and ready to process
              }else{
                r = 1;
              }
            // Buffer contains *idn? query and interface to respond
            }else if (pbPtr>3 && config.idn>0 && isIdnQuery(pBuf)){
              sendIdn = true;
              flushPbuf();
            // Buffer has at least 1 character = instrument data to send to gpib bus
            }else if (pbPtr > 0) {
              r = 2;
            }
            isPlusEscaped = false;
          }
        }
        break;
      case ESC:
        // Handle the escape character
        if (isEsc) {
          // Add character to buffer and cancel escape
          addPbuf(c);
          isEsc = false;
        } else {
          // Set escape flag
          isEsc  = true;  // Set escape flag
        }
        break;
      case PLUS:
        if (isEsc) {
          isEsc = false;
          if (pbPtr < 2) isPlusEscaped = true;
        }
        addPbuf(c);
        break;
      // Something else?
      default: // any char other than defined above
        // Buffer contains '++' (start of command). Stop sending data to serial port by halting GPIB receive.
        addPbuf(c);
        isEsc = false;
    }
  }
  if (pbPtr >= PBSIZE) {
    if (isCmd(pBuf) && !r) {  // Command without terminator and buffer full
      if (verbose()) {
        cmdstream->println(F("ERROR - Command buffer overflow!"));
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
bool Controller::isCmd(char *buffr) {
  if (buffr[0] == PLUS && buffr[1] == PLUS) {
#ifdef DEBUG1
    dbSerial->println(F("isCmd: Command detected."));
#endif
    return true;
  }
  return false;
}


/***** Is this an *idn? query? *****/
bool Controller::isIdnQuery(char *buffr) {
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
void Controller::addPbuf(char c) {
  pBuf[pbPtr] = c;
  pbPtr++;
}


/***** Clear the parse buffer *****/
void Controller::flushPbuf() {
  memset(pBuf, '\0', PBSIZE);
  pbPtr = 0;
  lnRdy = 0;
  dataBufferFull = false;
}


/***** Show a prompt *****/
void Controller::showPrompt() {
  if(prompt())
#ifdef USE_MACROS
	if (editMacro < NUM_MACROS)
	  cmdstream->print("| ");
	else
#endif
	  cmdstream->print("> ");
}


/***** Serial event handler *****/
/*
 * Note: the Arduino serial buffer is 64 characters long. Characters are stored in
 * the pBuf buffer until serialEvent_h() is called. parsedInput() takes a character at
 * a time and places it into the 256 character parse buffer whereupon it is parsed
 * to determine whether a command or data are present.
 * lnRdy=0: terminator not detected yet
 * lnRdy=1: terminator detected, sequence in parse buffer is a ++ command
 * lnRdy=2: terminator detected, sequence in parse buffer is data or direct instrument command
 * lnRdy=3: terminator detected, sequence in parse buffer is data or direct instrument command
 * lnRdy=4: terminator detected, sequence in parse buffer is line of the currently edited macro
 */
uint8_t Controller::serialIn_h() {
  uint8_t bufferStatus = 0;
  // Parse serial input until we have detected a line terminator
  while (cmdstream->available() && bufferStatus==0) {   // Parse while characters available and line is not complete
	bufferStatus = parseInput(cmdstream->read());
  }

#ifdef DEBUG1
  if (bufferStatus) {
    dbSerial->print(F("BufferStatus: "));
    dbSerial->println(bufferStatus);
  }
#endif

  lnRdy = bufferStatus;
#if defined(USE_MACROS)
  if (lnRdy > 0 && editMacro < NUM_MACROS)
	// we have a parsed line, and a macro being edited
	lnRdy = 4;
#endif
  return lnRdy;
}


void Controller::reset() {
#ifdef WDTO_1S  // Where defined, reset controller using watchdog timeout
  unsigned long tout;
  tout = millis() + 2000;
  wdt_enable(WDTO_1S);
  while (millis() < tout) {};
  // Should never reach here....
  if (verbose()) {
    arSerial->println(F("Reset FAILED."));
  };
#else  // Otherwise restart program (soft reset)
#if defined(__AVR__)
  asm volatile ("  jmp 0");
#elif defined(ESP32)
  ESP.restart();
#endif
#endif
}

void Controller::resetConfig() {
  // Set default values ({'\0'} sets version string array to null)
  config = {false, false, 2, 0, 1, 0, 0, 0, 0, 1200, 0, {'\0'}, 0, 0, {'\0'}, 0, 0, false, false
#ifdef AR488_WIFI_ENABLE
			,{'\0'}, {'\0'}
#endif
  };
}

void Controller::initConfig()
{
  /***** Initialise the interface *****/
  resetConfig();
#ifdef ESP32
  Preferences pref;
  pref.begin("ar488", true);
  config.eot_en = pref.getBool("eot_en", config.eot_en);
  config.eoi = pref.getBool("eoi", config.eoi);
  config.cmode = pref.getUChar("cmode", config.cmode);
  config.caddr = pref.getUChar("caddr", config.caddr);
  config.paddr = pref.getUChar("paddr", config.paddr);
  config.saddr = pref.getUChar("saddr", config.saddr);
  config.eos = pref.getUChar("eos", config.eos);
  config.stat = pref.getUChar("stat", config.stat);
  config.amode = pref.getUChar("amode", config.amode);
  config.rtmo = pref.getInt("rtmo", config.rtmo);
  config.eot_ch = pref.getChar("eot_ch", config.eot_ch);
  config.tmbus = pref.getUShort("tmbus", config.tmbus);
  config.eor = pref.getUChar("eor", config.eor);
  config.serial = pref.getUInt("serial", config.serial);
  config.idn = pref.getUInt("idn", config.idn);
  config.isVerb = pref.getBool("isVerb", config.isVerb);
  config.showPrompt = pref.getBool("showPrompt", config.showPrompt);
  if (pref.isKey("vstr")) {
	pref.getBytes("vstr", config.vstr, 48);
  }
  if (pref.isKey("sname")) {
	pref.getBytes("sname", config.sname, 16);
  }
#ifdef AR488_WIFI_ENABLE
  if (pref.isKey("ssid")) {
	pref.getBytes("ssid", config.ssid, 64);
  }
  if (pref.isKey("passkey")) {
	pref.getBytes("passkey", config.passkey, 64);
  }
#endif
  pref.end();

#elif defined(E2END)
  // Read data from non-volatile memory
  //(will only read if previous config has already been saved)
  //  epGetCfg();
  if (!epGet(0, config)) {
	resetConfig();
  }
#endif

  /*
#ifdef AR488_WIFI_ENABLE
  if (strlen(config.ssid) > 0)
	setupWifi();
#endif
  */
  // Initialize the interface in device mode
  if (config.cmode == 1)
	gpib->initDevice();
  // Initialize the interface in controller mode
  else
	gpib->initController();

  gpib->clearATN();
  gpib->clearSRQ();

}

void Controller::saveConfig()
{
#ifdef ESP32
  Preferences pref;
  pref.begin("ar488", false);
  pref.putBool("eot_en", config.eot_en);
  pref.putBool("eoi", config.eoi);
  pref.putBool("isVerb", config.isVerb);
  pref.putBool("showPrompt", config.showPrompt);
  pref.putUChar("cmode", config.cmode);
  pref.putUChar("caddr", config.caddr);
  pref.putUChar("paddr", config.paddr);
  pref.putUChar("saddr", config.saddr);
  pref.putUChar("eos", config.eos);
  pref.putUChar("stat", config.stat);
  pref.putUChar("amode", config.amode);
  pref.putInt("rtmo", config.rtmo);
  pref.putChar("eot_ch", config.eot_ch);
  pref.putUShort("tmbus", config.tmbus);
  pref.putUChar("eor", config.eor);
  pref.putUInt("serial", config.serial);
  pref.putUInt("idn", config.idn);

  pref.putBytes("vstr", config.vstr, 48);
  pref.putBytes("sname", config.sname, 16);

#ifdef AR488_WIFI_ENABLE
  pref.putBytes("ssid", config.ssid, 64);
  pref.putBytes("passkey", config.passkey, 64);
#endif

  pref.end();
  if (verbose()) cmdstream->println(F("Settings saved."));

#elif defined(E2END)
  epPut(0, config);
  if (verbose()) cmdstream->println(F("Settings saved."));

#else
  cmdstream->println(F("EEPROM not supported."));
#endif
}

void Controller::sendToInstrument()
{
  if (isRO) return;
  if (pbPtr == 0) return;
  if (pBuf[pbPtr-1] == '?') gpib->isQuery = true;
  gpib->gpibSendData(pBuf, pbPtr, dataBufferFull);
  flushPbuf();
}


/***** Execute a command *****/
void Controller::execCmd()
{
  char line[PBSIZE];
  int dsize = pbPtr;
  // Copy collected chars to line buffer
  memcpy(line, pBuf, pbPtr);

  // Flush the parse buffer
  flushPbuf();

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
  getCmd(line);

  showPrompt();
}

#ifdef AR488_WIFI_ENABLE
void Controller::setupWifi()
{
  if (strlen(config.ssid) > 0)
  {
	serialstream->print(F("Connecting to Wifi: "));
	serialstream->println(config.ssid);
	wifimulti.addAP(config.ssid, config.passkey);

	for (int loops = 10; loops > 0; loops--) {
    if (wifimulti.run() == WL_CONNECTED) {
      serialstream->println("");
      serialstream->print(F("WiFi connected "));
      serialstream->print(F("IP address: "));
      serialstream->println(WiFi.localIP());
	  wifiserver.begin();
	  wifiserver.setNoDelay(true);
      break;
    } else {
      serialstream->println(loops);
      delay(1000);
    }
	}
  if (wifimulti.run() != WL_CONNECTED) {
    serialstream->println(F("WiFi connect failed"));
    delay(1000);
  }
  }
}

void Controller::connectWifi()
{
  if (WiFi.status() == WL_CONNECTED) {
    //check if there are any new clients
    if (wifiserver.hasClient()) {
	  if (!serverclient || !serverclient.connected())
	  {
		if (serverclient) serverclient.stop();
		serverclient = wifiserver.available();
		if (!serverclient)
		  serialstream->println(F("available broken"));
		serialstream->print(F("New TCP client: "));
		serialstream->println(serverclient.remoteIP());
		tcpstream = (Stream*) &serverclient;
	  }
	  else
	  {
        //no free/disconnected spot so reject
        wifiserver.available().stop();
		tcpstream = NULL;
      }
    } else {
	  if (!serverclient.connected() && (cmdstream == (Stream*)&serverclient)) {
		serverclient.stop();
		// client got disconnected
		tcpstream = NULL;
	  }
	}
  }
}

void Controller::scanWifi()
{

  int n = WiFi.scanNetworks();
  serialstream->println("scan done");
  if (n == 0) {
	serialstream->println("no networks found");
  } else {
	serialstream->print(n);
	serialstream->println(" networks found");
	for (int i = 0; i < n; ++i) {
	  // Print SSID and RSSI for each network found
	  serialstream->print(i + 1);
	  serialstream->print(": ");
	  serialstream->print(WiFi.SSID(i));
	  serialstream->print(" (");
	  serialstream->print(WiFi.RSSI(i));
	  serialstream->print(")");
	  serialstream->println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
	  delay(10);
	}
  }
  serialstream->println("");
}
#endif

#if defined (USE_MACROS)
void Controller::displayMacros()
{
  String macro;

  for(int i=0; i<NUM_MACROS; i++) {
	macro = getMacro(i);
	if (macro.length() > 0) {
	  cmdstream->println(String("Macro ") + i + ":");
	  cmdstream->println(macro);
	}
  }
  cmdstream->println();
}

void Controller::appendToMacro()
{
  String macro;
  macro = pBuf;
  macro.trim();

  if (macro.length() > 0)
  {
	macro = getMacro(editMacro) + macro + "\n";
	if (macro.length() >= MACRO_MAX_LEN) {
	  if (verbose())
		cmdstream->println(F("Macro max length exceeded. Deleting."));
	  deleteMacro(editMacro);
	  editMacro = 255;
	}
	else
	  saveMacro(editMacro, macro);
  }
  else
	// blank line: end of macro edit mode
	editMacro = 255;
  flushPbuf();
  showPrompt();
}

#endif

void Controller::selectStream()
{
#ifdef AR488_WIFI_ENABLE
  // check for new tcp cnx
  if (strlen(config.ssid) > 0)
	connectWifi();
#endif
  Stream *current = cmdstream;

  String nextstream = "";
  if (serialstream->available()) {
	if (cmdstream != serialstream) {
	  nextstream = "serial";
	  cmdstream = serialstream;
	}
  }
  else if ((btstream != NULL) &&  btstream->available()) {
	if (cmdstream != btstream) {
	  nextstream = "BT";
	  cmdstream = btstream;
	}
  }
  else if ((tcpstream != NULL) &&  tcpstream->available()) {
	if (cmdstream != tcpstream) {
	  nextstream = "TCP";
	  cmdstream = tcpstream;
	}
  }

  if (nextstream.length() > 0) {
	current->println("Moving serial console to " + nextstream);
	showPrompt();
  }
}
