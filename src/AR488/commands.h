#if !defined(COMMANDS_H)
#define COMMANDS_H

/***** Comand function record *****/
struct cmdRec {
  const char* token;
  int opmode;
  void (*handler)(char *);
};

void getCmd(char *buffr);

// command callbacks
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


#endif
