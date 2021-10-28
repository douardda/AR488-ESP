#if !defined(COMMANDS_H)
#define COMMANDS_H

#include "AR488.h"
#include "controller.h"

/***** Command function record *****/
struct cmdRec {
  const char* token;
  int opmode;
  void (*handler)(char *, Controller&);
};

void getCmd(char *buffr, Controller& gpib);

bool notInRange(char *param, uint16_t lowl, uint16_t higl, uint16_t &rval, Controller& gpib);
void errBadCmd(Controller&);

void setSrqSig();
void clrSrqSig();

// command callbacks
void addr_h(char *params, Controller&);
void rtmo_h(char *params, Controller&);
void eos_h(char *params, Controller&);
void eoi_h(char *params, Controller&);
void cmode_h(char *params, Controller&);
void eot_en_h(char *params, Controller&);
void eot_char_h(char *params, Controller&);
void amode_h(char *params, Controller&);
void ver_h(char *params, Controller&);
void read_h(char *params, Controller&);
void clr_h(char *params, Controller&);
void llo_h(char *params, Controller&);
void loc_h(char *params, Controller&);
void ifc_h(char *params, Controller&);
void trg_h(char *params, Controller&);
void rst_h(char *params, Controller&);
void spoll_h(char *params, Controller&);
void srq_h(char *params, Controller&);
void stat_h(char *params, Controller&);
void save_h(char *params, Controller&);
void lon_h(char *params, Controller&);
void aspoll_h(char *params, Controller&);
void dcl_h(char *params, Controller&);
void default_h(char *params, Controller&);
void eor_h(char *params, Controller&);
void ppoll_h(char *params, Controller&);
void ren_h(char *params, Controller&);
void verb_h(char *params, Controller&);
void setvstr_h(char *params, Controller&);
void ton_h(char *params, Controller&);
void srqa_h(char *params, Controller&);
void repeat_h(char *params, Controller&);
void macro_h(char *params, Controller&);
void xdiag_h(char *params, Controller&);
void tmbus_h(char *params, Controller&);
void id_h(char *params, Controller&);
void idn_h(char * params, Controller&);


#endif
