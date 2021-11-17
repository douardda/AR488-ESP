#if !defined(SERIALCOMM_H)
#define SERIALCOMM_H

#include <Arduino.h>
#include "AR488.h"
//#include "gpib.h"
#include <Vector.h>

#ifdef AR488_WIFI_ENABLE
#include <WiFi.h>
#include <WiFiMulti.h>
#endif

class GPIB;

#define PBSIZE 256

/***** Controller configuration *****/
/*
 * Default values set for controller mode
 */
typedef struct {
  bool eot_en;      // Enable/disable append EOT char to string received from GPIB bus before sending to USB
  bool eoi;         // Assert EOI on last data char written to GPIB - 0-disable, 1-enable
  uint8_t cmode;    // Controller/device mode (0=unset, 1=device, 2=controller)
  uint8_t caddr;    // Controller address
  uint8_t paddr;    // Primary device address
  uint8_t saddr;    // Secondary device address
  uint8_t eos;      // EOS (end of send to GPIB) characters [0=CRLF, 1=CR, 2=LF, 3=None]
  uint8_t stat;     // Status byte to return in response to a serial poll
  uint8_t amode;    // Auto mode setting (0=off; 1=Prologix; 2=onquery; 3=continuous);
  int rtmo;         // Read timout (read_tmo_ms) in milliseconds - 0-3000 - value depends on instrument
  char eot_ch;      // EOT character to append to USB output when EOI signal detected
  char vstr[48];    // Custom version string
  uint16_t tmbus;   // Delay to allow the bus control/data lines to settle (1-30,000 microseconds)
  uint8_t eor;      // EOR (end of receive from GPIB instrument) characters [0=CRLF, 1=CR, 2=LF, 3=None, 4=LFCR, 5=ETX, 6=CRLF+ETX, 7=SPACE]
  char sname[16];   // Interface short name
  uint32_t serial;  // Serial number
  uint8_t idn;      // Send ID in response to *idn? 0=disable, 1=send name; 2=send name+serial
  bool isVerb;      // Verbose mode
  bool showPrompt;  // Show a prompt (when ready to accept commands)
#ifdef AR488_WIFI_ENABLE
  char ssid[32];    // max size for WiFiMulti.addAp is 31
  char passkey[64]; // same
#endif
} AR488Conf;

class Controller {
public:
  Controller();
  uint8_t parseInput(char c);
  bool isCmd(char *buffr);
  bool isIdnQuery(char *buffr);
  void addPbuf(char c);
  void flushPbuf();
  void showPrompt();
  uint8_t serialIn_h();
  void reset();
  void initConfig();
  void resetConfig();
  void saveConfig();
  bool verbose() {return config.isVerb;};
  bool prompt() {return config.showPrompt;};
  void sendToInstrument();
  void setGPIB(GPIB *gpib) {this->gpib = gpib;};
  void execCmd();
#ifdef AR488_WIFI_ENABLE
  void setupWifi();
  void connectWifi();
  void scanWifi();
#endif
#if defined (USE_MACROS)
  void displayMacros();
  void appendToMacro();
#endif
  void selectStream();

public:
  AR488Conf config;
  Stream *serialstream;
  Stream *btstream;
  Stream *tcpstream;
  Stream *cmdstream;
  GPIB *gpib = NULL;

private:
#ifdef AR488_WIFI_ENABLE
  WiFiMulti wifimulti;
  WiFiServer wifiserver;
  WiFiClient serverclient;
#endif

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
  uint8_t runMacro = 0;         // Macro to run next loop
  uint8_t editMacro = 255;      // Macro beinf edited
  bool sendIdn = false;         // Send response to *idn?

};
#endif
