#ifndef AR488_H
#define AR488_H

#include <Arduino.h>

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
void addr_h(char *params);
void rtmo_h(char *params);
void eos_h(char *params);
void eoi_h(char *params);
void cmode_h(char *params);
void eot_en_h(char *params);
void eot_char_h(char *params);
void amode_h(char *params);
void ver_h(char *params);
void read_h(char *params);
void clr_h();
void llo_h(char *params);
void loc_h(char *params);
void ifc_h();
void trg_h(char *params);
void rst_h();
void spoll_h(char *params);
void srq_h();
void stat_h(char *params);
void save_h();
void lon_h(char *params);
void setSrqSig();
void clrSrqSig();
void aspoll_h();
void dcl_h();
void default_h();
void eor_h(char *params);
void ppoll_h();
void ren_h(char *params);
void verb_h();
void setvstr_h(char *params);
void ton_h(char *params);
void srqa_h(char *params);
void repeat_h(char *params);
void macro_h(char *params);
void xdiag_h(char *params);
void tmbus_h(char *params);
void id_h(char *params);
void idn_h(char * params);
void attnRequired();
void mla_h();
void mta_h();
void sdc_h();
void spd_h();
void spe_h();
void unl_h();
void unt_h();
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
