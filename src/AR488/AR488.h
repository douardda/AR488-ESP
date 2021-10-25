#ifndef AR488_H
#define AR488_H

#include <Arduino.h>
#include "AR488_Config.h"

#ifdef E2END
  #include "AR488_Eeprom.h"
  #include <EEPROM.h>
#endif

/*********************************************/
/***** GPIB COMMAND & STATUS DEFINITIONS *****/
/***** vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv *****/

// Universal Multiline commands (apply to all devices)

#define GC_LLO 0x11
#define GC_DCL 0x14
#define GC_PPU 0x15
#define GC_SPE 0x18
#define GC_SPD 0x19
#define GC_UNL 0x3F
#define GC_TAD 0x40
#define GC_PPE 0x60
#define GC_PPD 0x70
#define GC_UNT 0x5F
// Address commands
#define GC_LAD 0x20
// Addressed commands
#define GC_GTL 0x01
#define GC_SDC 0x04
#define GC_PPC 0x05
#define GC_GET 0x08

/***** GPIB control states *****/
// Controller mode
#define CINI 0x01 // Controller idle state
#define CIDS 0x02 // Controller idle state
#define CCMS 0x03 // Controller command state
#define CTAS 0x04 // Controller talker active state
#define CLAS 0x05 // Controller listner active state
// Listner/device mode
#define DINI 0x06 // Device initialise state
#define DIDS 0x07 // Device idle state
#define DLAS 0x08 // Device listener active (listening/receiving)
#define DTAS 0x09 // Device talker active (sending) state

/***** ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ *****/
/***** GPIB COMMAND & STATUS DEFINITIONS *****/
/*********************************************/



/************************************/
/***** COMMON VARIABLES SECTION *****/
/***** vvvvvvvvvvvvvvvvvvvvvvvv *****/

/****** Process status values *****/
#define OK 0
#define ERR 1

/***** Control characters *****/
#define ESC  0x1B   // the USB escape char
#define CR   0xD    // Carriage return
#define LF   0xA    // Newline/linefeed
#define PLUS 0x2B   // '+' character

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
} AR488Conf;

#define AR_CFG_SIZE 96  // ie. sizeof(AR488Conf) XXX recompute this


typedef struct {
  // State flags set by interrupt being triggered
  volatile bool isATN = false;  // has ATN been asserted?
  volatile bool isSRQ = false;  // has SRQ been asserted?
  // SRQ auto mode
  bool isSrqa = false;
  // Interrupt without handler fired
  //volatile bool isBAD = false;
  // Whether to run Macro 0 (macros must be enabled)
  uint8_t runMacro = 0;
  // Send response to *idn?
  bool sendIdn = false;
} AR488State;

void setup();
void loop();
void initAR488();
void printHex(char *buffr, int dsize);
#ifdef USE_MACROS
void execMacro(uint8_t idx);
#endif

#endif
