#if !defined(COMMANDS_H)
#define COMMANDS_H

#include "AR488.h"
#include "controller.h"

/***** Command function record *****/
struct cmdRec {
  const char* token;
  int opmode;
  void (*handler)(char *, AR488Conf&);
};

void getCmd(char *buffr, AR488Conf& AR488);
void execCmd(char *buffr, uint8_t dsize, AR488Conf& AR488, CommandComm& comm);

bool notInRange(char *param, uint16_t lowl, uint16_t higl, uint16_t &rval, bool verbose);
void errBadCmd();

void setSrqSig();
void clrSrqSig();

// command callbacks
void addr_h(char *params, AR488Conf&);
void rtmo_h(char *params, AR488Conf&);
void eos_h(char *params, AR488Conf&);
void eoi_h(char *params, AR488Conf&);
void cmode_h(char *params, AR488Conf&);
void eot_en_h(char *params, AR488Conf&);
void eot_char_h(char *params, AR488Conf&);
void amode_h(char *params, AR488Conf&);
void ver_h(char *params, AR488Conf&);
void read_h(char *params, AR488Conf&);
void clr_h(char *params, AR488Conf&);
void llo_h(char *params, AR488Conf&);
void loc_h(char *params, AR488Conf&);
void ifc_h(char *params, AR488Conf&);
void trg_h(char *params, AR488Conf&);
void rst_h(char *params, AR488Conf&);
void spoll_h(char *params, AR488Conf&);
void srq_h(char *params, AR488Conf&);
void stat_h(char *params, AR488Conf&);
void save_h(char *params, AR488Conf&);
void lon_h(char *params, AR488Conf&);
void aspoll_h(char *params, AR488Conf&);
void dcl_h(char *params, AR488Conf&);
void default_h(char *params, AR488Conf&);
void eor_h(char *params, AR488Conf&);
void ppoll_h(char *params, AR488Conf&);
void ren_h(char *params, AR488Conf&);
void verb_h(char *params, AR488Conf&);
void setvstr_h(char *params, AR488Conf&);
void ton_h(char *params, AR488Conf&);
void srqa_h(char *params, AR488Conf&);
void repeat_h(char *params, AR488Conf&);
void macro_h(char *params, AR488Conf&);
void xdiag_h(char *params, AR488Conf&);
void tmbus_h(char *params, AR488Conf&);
void id_h(char *params, AR488Conf&);
void idn_h(char * params, AR488Conf&);


#endif
