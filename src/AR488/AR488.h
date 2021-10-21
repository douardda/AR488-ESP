#ifndef AR488_H
#define AR488_H

#include <Arduino.h>
#include "commands.h"
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
union AR488conf{
  struct{
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
  };
  uint8_t db[AR_CFG_SIZE];
};

struct AR488state{
  uint8_t cstate = 0; // GPIB control state
  bool isVerb = false; // Verbose mode
  uint8_t lnRdy = 0; // CR/LF terminated line ready to process
  // GPIB data receive flags
  //bool isReading = false; // Is a GPIB read in progress?
  bool aRead = false;     // GPIB data read in progress
  bool rEoi = false;      // Read eoi requested
  bool rEbt = false;      // Read with specified terminator character
  bool isQuery = false;   // Direct instrument command is a query
  uint8_t tranBrk = 0;    // Transmission break on 1=++, 2=EOI, 3=ATN 4=UNL
  uint8_t eByte = 0;      // Termination character
  // Device mode - send data
  bool snd = false;
  // Escaped character flag
  bool isEsc = false;           // Charcter escaped
  bool isPlusEscaped = false;   // Plus escaped
  // Read only mode flag
  bool isRO = false;
  // Talk only mode flag
  bool isTO = false;
  // GPIB command parser
  bool aTt = false;
  bool aTl = false;
  // Data send mode flags
  bool deviceAddressing = true;   // Suppress sending commands to address the instrument
  bool dataBufferFull = false;    // Flag when parse buffer is full
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
};

void initDevice();
void setup();
void loop();
void initAR488();
void initController();
uint8_t serialIn_h();
bool isAtnAsserted();
void errBadCmd();
uint8_t parseInput(char c);
bool isCmd(char *buffr);
bool isIdnQuery(char *buffr);
bool isRead(char *buffr);
void addPbuf(char c);
void flushPbuf();
void showPrompt();
void sendToInstrument(char *buffr, uint8_t dsize);
void execCmd(char *buffr, uint8_t dsize);
void getCmd(char *buffr);
void printHex(char *buffr, int dsize);
bool notInRange(char *param, uint16_t lowl, uint16_t higl, uint16_t &rval);
#ifdef USE_MACROS
void execMacro(uint8_t idx);
#endif
void attnRequired();
void mla_h();
void mta_h();
void sdc_h();
void spd_h();
void spe_h();
void unl_h();
void unt_h();
void setSrqSig();
void clrSrqSig();
void lonMode();
bool gpibSendCmd(uint8_t cmdByte);
void gpibSendStatus();
void gpibSendData(char *data, uint8_t dsize);
bool gpibReceiveData();
bool isTerminatorDetected(uint8_t bytes[3], uint8_t eor_sequence);
uint8_t gpibReadByte(uint8_t *db, bool *eoi);
bool gpibWriteByte(uint8_t db);
bool gpibWriteByteHandshake(uint8_t db);
bool addrDev(uint8_t addr, bool dir);
bool uaddrDev();

bool Wait_on_pin_state(uint8_t state, uint8_t pin, int interval);
void setGpibControls(uint8_t state);


#endif
