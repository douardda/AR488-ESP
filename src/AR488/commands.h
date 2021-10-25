#if !defined(COMMANDS_H)
#define COMMANDS_H

#include "AR488.h"
#include "gpib.h"

/***** Command function record *****/
struct cmdRec {
  const char* token;
  int opmode;
  void (*handler)(char *, GPIB&);
};

void getCmd(char *buffr, GPIB& gpib);
void execCmd(char *buffr, uint8_t dsize, GPIB& gpib);

bool notInRange(char *param, uint16_t lowl, uint16_t higl, uint16_t &rval, GPIB& gpib);
void errBadCmd(GPIB&);

void setSrqSig();
void clrSrqSig();

// command callbacks
void addr_h(char *params, GPIB&);
void rtmo_h(char *params, GPIB&);
void eos_h(char *params, GPIB&);
void eoi_h(char *params, GPIB&);
void cmode_h(char *params, GPIB&);
void eot_en_h(char *params, GPIB&);
void eot_char_h(char *params, GPIB&);
void amode_h(char *params, GPIB&);
void ver_h(char *params, GPIB&);
void read_h(char *params, GPIB&);
void clr_h(char *params, GPIB&);
void llo_h(char *params, GPIB&);
void loc_h(char *params, GPIB&);
void ifc_h(char *params, GPIB&);
void trg_h(char *params, GPIB&);
void rst_h(char *params, GPIB&);
void spoll_h(char *params, GPIB&);
void srq_h(char *params, GPIB&);
void stat_h(char *params, GPIB&);
void save_h(char *params, GPIB&);
void lon_h(char *params, GPIB&);
void aspoll_h(char *params, GPIB&);
void dcl_h(char *params, GPIB&);
void default_h(char *params, GPIB&);
void eor_h(char *params, GPIB&);
void ppoll_h(char *params, GPIB&);
void ren_h(char *params, GPIB&);
void verb_h(char *params, GPIB&);
void setvstr_h(char *params, GPIB&);
void ton_h(char *params, GPIB&);
void srqa_h(char *params, GPIB&);
void repeat_h(char *params, GPIB&);
void macro_h(char *params, GPIB&);
void xdiag_h(char *params, GPIB&);
void tmbus_h(char *params, GPIB&);
void id_h(char *params, GPIB&);
void idn_h(char * params, GPIB&);


#endif
