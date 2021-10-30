#if !defined(GPIB_H)

#include <Arduino.h>
#include "controller.h"

class GPIB {
public:
  GPIB(Controller&);

  void initDevice();
  void initController();
  void initPins();

  bool gpibSendCmd(uint8_t cmdByte);
  void gpibSendStatus();
  void gpibSendData(char *data, uint8_t dsize, bool bufferFull);
  bool gpibReceiveData();
  uint8_t gpibReadByte(uint8_t *db, bool *eoi);
  bool gpibWriteByte(uint8_t db);
  bool gpibWriteByteHandshake(uint8_t db);

  bool addrDev(uint8_t addr, bool dir);
  bool uaddrDev();

  bool isTerminatorDetected(uint8_t bytes[3], uint8_t eor_sequence);
  bool isAtnAsserted();
  void assertIfc();

  //void sendToInstrument(char *buffr, uint8_t dsize);

  /*****  GPIB CONTROL ROUTINES *****/
  bool Wait_on_pin_state(uint8_t state, uint8_t pin, int interval);
  void setGpibControls(uint8_t state);

  /***** Device mode GPIB command handling routines *****/
  void attnRequired();
  void mla_h();
  void mta_h();
  void sdc_h();
  void spd_h();
  void spe_h();
  void unl_h();
  void unt_h();
  void lonMode();

  bool isATN() {return ATNasserted;}
  bool isSRQ() {return SRQasserted;}
  void clearATN() {ATNasserted = false;}
  void clearSRQ() {SRQasserted = false;}
  void setATN(bool atn) {ATNasserted = atn;}
  void setSRQ(bool srq) {SRQasserted = srq;}

  bool verbose() {return controller.config.isVerb;}

private:
  Controller &controller;
  AR488Conf &config;

public:
  // XXX should not be public...
  bool deviceAddressing = true;
  uint8_t cstate = 0;     // GPIB control state
  uint8_t tranBrk = 0;    // Transmission break on 1=++, 2=EOI, 3=ATN 4=UNL
  bool aTt = false;       // currently unused
  bool aTl = false;       // currently unused

  bool ATNasserted = false;  // has ATN been asserted?
  bool SRQasserted = false;  // has SRQ been asserted?

public:  // TODO: fix this
  bool rEoi = false;      // Read eoi requested
  bool rEbt = false;      // Read with specified terminator character
  uint8_t eByte = 0;      // Termination character
  bool isQuery = false;   // Direct instrument command is a query


};


#define GPIB_H
#endif
