#ifndef AR488_BT_H
#define AR488_BT_H

#include <Arduino.h>
#include "AR488_Config.h"

#ifdef AR488_BT_HC05
void hc05Init(Stream *);
#endif  // AR488_BT_HC05

#endif // AR488_BT_H
