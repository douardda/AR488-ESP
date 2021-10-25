#if !defined(GPIB_H)

#include <Arduino.h>
#include "AR488.h"
#include "controller.h"

class GPIB {
public:
  GPIB(Stream&, AR488Conf&, AR488State&, CommandComm&);

  void initDevice();
  void initController();

  bool gpibSendCmd(uint8_t cmdByte);
  void gpibSendStatus();
  void gpibSendData(char *data, uint8_t dsize);
  bool gpibReceiveData();
  uint8_t gpibReadByte(uint8_t *db, bool *eoi);
  bool gpibWriteByte(uint8_t db);
  bool gpibWriteByteHandshake(uint8_t db);

  bool addrDev(uint8_t addr, bool dir);
  bool uaddrDev();

  bool isTerminatorDetected(uint8_t bytes[3], uint8_t eor_sequence);
  bool isAtnAsserted();
  void assertIfc();

  void sendToInstrument(char *buffr, uint8_t dsize);

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


private:
  Stream &outstream;
  AR488Conf &AR488;
  AR488State &AR488st;
  CommandComm &comm;
  bool verbose;

  //
  bool deviceAddressing = true;
  uint8_t cstate = 0; // GPIB control state

public:  // TODO: fix this
  bool rEoi = false;      // Read eoi requested
  bool rEbt = false;      // Read with specified terminator character
  uint8_t eByte = 0;      // Termination character

};


#define GPIB_H
#endif
