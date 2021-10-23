#if !defined(COMMANDS_H)
#define COMMANDS_H

#include "AR488.h"
#include "serialcomm.h"

/***** Command function record *****/
struct cmdRec {
  const char* token;
  int opmode;
  void (*handler)(char *, AR488Conf&, AR488State&);
};

void getCmd(char *buffr, AR488Conf& AR488, AR488State& AR488st);
void execCmd(char *buffr, uint8_t dsize, AR488Conf& AR488, AR488State& AR488st, CommandComm& comm);

bool notInRange(char *param, uint16_t lowl, uint16_t higl, uint16_t &rval, bool verbose);
void errBadCmd();

void setSrqSig();
void clrSrqSig();

// command callbacks
void addr_h(char *params, AR488Conf&, AR488State&);
void rtmo_h(char *params, AR488Conf&, AR488State&);
void eos_h(char *params, AR488Conf&, AR488State&);
void eoi_h(char *params, AR488Conf&, AR488State&);
void cmode_h(char *params, AR488Conf&, AR488State&);
void eot_en_h(char *params, AR488Conf&, AR488State&);
void eot_char_h(char *params, AR488Conf&, AR488State&);
void amode_h(char *params, AR488Conf&, AR488State&);
void ver_h(char *params, AR488Conf&, AR488State&);
void read_h(char *params, AR488Conf&, AR488State&);
void clr_h(char *params, AR488Conf&, AR488State&);
void llo_h(char *params, AR488Conf&, AR488State&);
void loc_h(char *params, AR488Conf&, AR488State&);
void ifc_h(char *params, AR488Conf&, AR488State&);
void trg_h(char *params, AR488Conf&, AR488State&);
void rst_h(char *params, AR488Conf&, AR488State&);
void spoll_h(char *params, AR488Conf&, AR488State&);
void srq_h(char *params, AR488Conf&, AR488State&);
void stat_h(char *params, AR488Conf&, AR488State&);
void save_h(char *params, AR488Conf&, AR488State&);
void lon_h(char *params, AR488Conf&, AR488State&);
void aspoll_h(char *params, AR488Conf&, AR488State&);
void dcl_h(char *params, AR488Conf&, AR488State&);
void default_h(char *params, AR488Conf&, AR488State&);
void eor_h(char *params, AR488Conf&, AR488State&);
void ppoll_h(char *params, AR488Conf&, AR488State&);
void ren_h(char *params, AR488Conf&, AR488State&);
void verb_h(char *params, AR488Conf&, AR488State&);
void setvstr_h(char *params, AR488Conf&, AR488State&);
void ton_h(char *params, AR488Conf&, AR488State&);
void srqa_h(char *params, AR488Conf&, AR488State&);
void repeat_h(char *params, AR488Conf&, AR488State&);
void macro_h(char *params, AR488Conf&, AR488State&);
void xdiag_h(char *params, AR488Conf&, AR488State&);
void tmbus_h(char *params, AR488Conf&, AR488State&);
void id_h(char *params, AR488Conf&, AR488State&);
void idn_h(char * params, AR488Conf&, AR488State&);


#endif
