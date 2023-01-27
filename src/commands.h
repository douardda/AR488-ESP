#if !defined(COMMANDS_H)
#define COMMANDS_H

#include "AR488.h"
#include "controller.h"

typedef void (Controller::*command_t)(char*);
/***** Command function record *****/
struct cmdRec {
  const char* token;
  int opmode;
  command_t handler;
};

#endif
