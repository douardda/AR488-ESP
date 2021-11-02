#include <Arduino.h>
#include "AR488_Layouts.h"
#include "commands.h"
#include "controller.h"
#include "gpib.h"
#include "macros.h"


/***** Array containing index of accepted ++ commands *****/
/*
 * Commands without parameters require casting to a pointer
 * requiring a char* parameter. The functon is called with
 * NULL by the command processor.
 *
 * Format: token, mode, function_ptr
 * Mode: 1=device; 2=controller; 3=both;
 */
static cmdRec cmdHidx [] = {
  { "addr",        3, addr_h      },
  { "allspoll",    2, aspoll_h    },
  { "auto",        2, amode_h     },
  { "clr",         2, clr_h       },
  { "dcl",         2, dcl_h       },
  { "default",     3, default_h   },
  { "eoi",         3, eoi_h       },
  { "eor",         3, eor_h       },
  { "eos",         3, eos_h       },
  { "eot_char",    3, eot_char_h  },
  { "eot_enable",  3, eot_en_h    },
  { "ifc",         2, ifc_h       },
  { "id",          3, id_h        },
  { "idn",         3, idn_h       },
  { "llo",         2, llo_h       },
  { "loc",         2, loc_h       },
  { "lon",         1, lon_h       },
  { "macro",       2, macro_h     },
  { "mode" ,       3, cmode_h     },
  { "ppoll",       2, ppoll_h     },
  { "read",        2, read_h      },
  { "read_tmo_ms", 2, rtmo_h      },
  { "ren",         2, ren_h       },
  { "repeat",      2, repeat_h    },
  { "rst",         3, rst_h       },
  { "trg",         2, trg_h       },
  { "savecfg",     3, save_h      },
  { "setvstr",     3, setvstr_h   },
  { "spoll",       2, spoll_h     },
  { "srq",         2, srq_h       },
  { "srqauto",     2, srqa_h      },
  { "status",      1, stat_h      },
  { "ton",         1, ton_h       },
  { "ver",         3, ver_h       },
  { "verbose",     3, verb_h      },
  { "tmbus",       3, tmbus_h     },
#ifdef AR488_WIFI_EN
  { "wifi",        3, wifi_h      },
#endif
  { "xdiag",       3, xdiag_h     }
};



/***** Extract command and pass to handler *****/
void getCmd(char *buffr, Controller& controller) {

  char *token;  // Pointer to command token
  char *params; // Pointer to parameters (remaining buffer characters)

  int casize = sizeof(cmdHidx) / sizeof(cmdHidx[0]);
  int i = 0;

#ifdef DEBUG1
  dbSerial->print("getCmd: ");
  dbSerial->print(buffr); dbSerial->print(F(" - length:")); dbSerial->println(strlen(buffr));
#endif

  // If terminator on blank line then return immediately without processing anything
  if (buffr[0] == 0x00) return;
  if (buffr[0] == CR) return;
  if (buffr[0] == LF) return;

  // Get the first token
  token = strtok(buffr, " \t");

#ifdef DEBUG1
  dbSerial->print("getCmd: process token: "); dbSerial->println(token);
#endif

  // Check whether it is a valid command token
  i = 0;
  do {
    if (strcasecmp(cmdHidx[i].token, token) == 0) break;
    i++;
  } while (i < casize);

  if (i < casize) {
    // We have found a valid command and handler
#ifdef DEBUG1
    dbSerial->print("getCmd: found handler for: "); dbSerial->println(cmdHidx[i].token);
#endif
    // If command is relevant to mode then execute it
    if (cmdHidx[i].opmode & controller.config.cmode) {
      // If its a command with parameters
      // Copy command parameters to params and call handler with parameters
      params = token + strlen(token) + 1;

      // If command parameters were specified
      if (strlen(params) > 0) {
#ifdef DEBUG1
        dbSerial->print(F("Calling handler with parameters: ")); dbSerial->println(params);
#endif
        // Call handler with parameters specified
        cmdHidx[i].handler(params, controller);
      }else{
        // Call handler without parameters
		cmdHidx[i].handler(NULL, controller);
      }
    }else{
      errBadCmd(controller);
      if (controller.config.isVerb) controller.cmdstream->println(F("Command not available in this mode."));
    }

  } else {
    // No valid command found
    errBadCmd(controller);
  }

}

/*************************************/
/***** STANDARD COMMAND HANDLERS *****/
/*************************************/

/***** Show or change device address *****/
void addr_h(char *params, Controller& controller) {
  //  char *param, *stat;
  char *param;
  uint16_t val;
  if (params != NULL) {

    // Primary address
    param = strtok(params, " \t");
    if (notInRange(param, 1, 30, val, controller)) return;
    if (val == controller.config.caddr) {
      errBadCmd(controller);
      if (controller.config.isVerb) controller.cmdstream->println(F("That is my address! Address of a remote device is required."));
      return;
    }
    controller.config.paddr = val;
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("Set device primary address to: "));
      controller.cmdstream->println(val);
    }

    // Secondary address
    controller.config.saddr = 0;
    val = 0;
    param = strtok(NULL, " \t");
    if (param != NULL) {
      if (notInRange(param, 96, 126, val, controller)) return;
      controller.config.saddr = val;
      if (controller.config.isVerb) {
        controller.cmdstream->print("Set device secondary address to: ");
        controller.cmdstream->println(val);
      }
    }

  } else {
    controller.cmdstream->print(controller.config.paddr);
    if (controller.config.saddr > 0) {
      controller.cmdstream->print(F(" "));
      controller.cmdstream->print(controller.config.saddr);
    }
    controller.cmdstream->println();
  }
}


/***** Show or set read timout *****/
void rtmo_h(char *params, Controller& controller) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 1, 32000, val, controller)) return;
    controller.config.rtmo = val;
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("Set [read_tmo_ms] to: "));
      controller.cmdstream->print(val);
      controller.cmdstream->println(F(" milliseconds"));
    }
  } else {
    controller.cmdstream->println(controller.config.rtmo);
  }
}


/***** Show or set end of send character *****/
void eos_h(char *params, Controller& controller) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 3, val, controller)) return;
    controller.config.eos = (uint8_t)val;
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("Set EOS to: "));
      controller.cmdstream->println(val);
    };
  } else {
    controller.cmdstream->println(controller.config.eos);
  }
}


/***** Show or set EOI assertion on/off *****/
void eoi_h(char *params, Controller& controller) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val, controller)) return;
    controller.config.eoi = val ? true : false;
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("Set EOI assertion: "));
      controller.cmdstream->println(val ? "ON" : "OFF");
    };
  } else {
    controller.cmdstream->println(controller.config.eoi);
  }
}


/***** Show or set interface to controller/device mode *****/
void cmode_h(char *params, Controller& controller) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val, controller)) return;
    switch (val) {
      case 0:
        controller.config.cmode = 1;
        controller.gpib->initDevice();
        break;
      case 1:
        controller.config.cmode = 2;
        controller.gpib->initController();
        break;
    }
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("Interface mode set to: "));
      controller.cmdstream->println(val ? "CONTROLLER" : "DEVICE");
    }
  } else {
    controller.cmdstream->println(controller.config.cmode - 1);
  }
}


/***** Show or enable/disable sending of end of transmission character *****/
void eot_en_h(char *params, Controller& controller) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val, controller)) return;
    controller.config.eot_en = val ? true : false;
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("Appending of EOT character: "));
      controller.cmdstream->println(val ? "ON" : "OFF");
    }
  } else {
    controller.cmdstream->println(controller.config.eot_en);
  }
}


/***** Show or set end of transmission character *****/
void eot_char_h(char *params, Controller& controller) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 255, val, controller)) return;
    controller.config.eot_ch = (uint8_t)val;
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("EOT set to ASCII character: "));
      controller.cmdstream->println(val);
    };
  } else {
    controller.cmdstream->println(controller.config.eot_ch, DEC);
  }
}


/***** Show or enable/disable auto mode *****/
void amode_h(char *params, Controller& controller) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 3, val, controller)) return;
    if (val > 0 && controller.config.isVerb) {
      controller.cmdstream->println(F("WARNING: automode ON can cause some devices to generate"));
      controller.cmdstream->println(F("         'addressed to talk but nothing to say' errors"));
    }
    controller.config.amode = (uint8_t)val;
    if (controller.config.amode < 3) controller.aRead = false;
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("Auto mode: "));
      controller.cmdstream->println(controller.config.amode);
    }
  } else {
    controller.cmdstream->println(controller.config.amode);
  }
}


/***** Display the controller version string *****/
void ver_h(char *params, Controller& controller) {
  // If "real" requested
  if (params != NULL && strncmp(params, "real", 3) == 0) {
    controller.cmdstream->println(F(FWVER));
    // Otherwise depends on whether we have a custom string set
  } else {
    if (strlen(controller.config.vstr) > 0) {
      controller.cmdstream->println(controller.config.vstr);
    } else {
      controller.cmdstream->println(F(FWVER));
    }
  }
}


/***** Address device to talk and read the sent data *****/
void read_h(char *params, Controller& controller) {
  GPIB &gpib = *controller.gpib;
  // Clear read flags
  gpib.rEoi = false;
  gpib.rEbt = false;
  // Read any parameters
  if (params != NULL) {
    if (strlen(params) > 3) {
      if (controller.config.isVerb) controller.cmdstream->println(F("Invalid termination character - ignored!"));
    } else if (strncmp(params, "eoi", 3) == 0) { // Read with eoi detection
      gpib.rEoi = true;
    } else { // Assume ASCII character given and convert to an 8 bit byte
      gpib.rEbt = true;
      gpib.eByte = atoi(params);
    }
  }
  if (controller.config.amode == 3) {
    // In auto continuous mode we set this flag to indicate we are ready for continuous read
    controller.aRead = true;
  } else {
    // If auto mode is disabled we do a single read
    gpib.gpibReceiveData();
  }
}


/***** Send device clear (usually resets the device to power on state) *****/
void clr_h(char *params, Controller& controller) {
  GPIB &gpib = *controller.gpib;
  if (gpib.addrDev(controller.config.paddr, 0)) {
    if (controller.config.isVerb) controller.cmdstream->println(F("Failed to address device"));
    return;
  }
  if (gpib.gpibSendCmd(GC_SDC))  {
    if (controller.config.isVerb) controller.cmdstream->println(F("Failed to send SDC"));
    return;
  }
  if (gpib.uaddrDev()) {
    if (controller.config.isVerb) controller.cmdstream->println(F("Failed to untalk GPIB bus"));
    return;
  }
  // Set GPIB controls back to idle state
  gpib.setGpibControls(CIDS);
}


/***** Send local lockout command *****/
void llo_h(char *params, Controller& controller) {
  GPIB &gpib = *controller.gpib;
  // NOTE: REN *MUST* be asserted (LOW)
  if (digitalRead(REN)==LOW) {
    // For 'all' send LLO to the bus without addressing any device - devices will show REM
    if (params != NULL) {
      if (0 == strncmp(params, "all", 3)) {
        if (gpib.gpibSendCmd(GC_LLO)) {
          if (controller.config.isVerb) controller.cmdstream->println(F("Failed to send universal LLO."));
        }
      }
    } else {
      // Address current device
      if (gpib.addrDev(controller.config.paddr, 0)) {
        if (controller.config.isVerb) controller.cmdstream->println(F("Failed to address the device."));
        return;
      }
      // Send LLO to currently addressed device
      if (gpib.gpibSendCmd(GC_LLO)) {
        if (controller.config.isVerb) controller.cmdstream->println(F("Failed to send LLO to device"));
        return;
      }
      // Unlisten bus
      if (gpib.uaddrDev()) {
        if (controller.config.isVerb) controller.cmdstream->println(F("Failed to unlisten the GPIB bus"));
        return;
      }
    }
  }
  // Set GPIB controls back to idle state
  gpib.setGpibControls(CIDS);
}


/***** Send Go To Local (GTL) command *****/
void loc_h(char *params, Controller& controller) {
  GPIB &gpib = *controller.gpib;
  // REN *MUST* be asserted (LOW)
  if (digitalRead(REN)==LOW) {
    if (params != NULL) {
      if (strncmp(params, "all", 3) == 0) {
        // Un-assert REN
        setGpibState(0b00100000, 0b00100000, 0);
        delay(40);
        // Simultaneously assert ATN and REN
        setGpibState(0b00000000, 0b10100000, 0);
        delay(40);
        // Unassert ATN
        setGpibState(0b10000000, 0b10000000, 0);
      }
    } else {
      // Address device to listen
      if (gpib.addrDev(controller.config.paddr, 0)) {
        if (controller.config.isVerb) controller.cmdstream->println(F("Failed to address device."));
        return;
      }
      // Send GTL
      if (gpib.gpibSendCmd(GC_GTL)) {
        if (controller.config.isVerb) controller.cmdstream->println(F("Failed sending LOC."));
        return;
      }
      // Unlisten bus
      if (gpib.uaddrDev()) {
        if (controller.config.isVerb) controller.cmdstream->println(F("Failed to unlisten GPIB bus."));
        return;
      }
      // Set GPIB controls back to idle state
      gpib.setGpibControls(CIDS);
    }
  }
}


/***** Assert IFC for 150 microseconds *****/
/* This indicates that the AR488 the Controller-in-Charge on
 * the bus and causes all interfaces to return to their idle
 * state
 */
void ifc_h(char *params, Controller& controller) {
  controller.gpib->assertIfc();
}


/***** Send a trigger command *****/
void trg_h(char *params, Controller& controller) {
  char *param;
  uint8_t addrs[15];
  uint16_t val = 0;
  uint8_t cnt = 0;
  GPIB &gpib = *controller.gpib;

  // Initialise address array
  for (int i = 0; i < 15; i++) {
    addrs[i] = 0;
  }

  // Read parameters
  if (params == NULL) {
    // No parameters - trigger addressed device only
    addrs[0] = controller.config.paddr;
    cnt++;
  } else {
    // Read address parameters into array
    while (cnt < 15) {
      if (cnt == 0) {
        param = strtok(params, " \t");
      } else {
        param = strtok(NULL, " \t");
      }
      if (notInRange(param, 1, 30, val, controller)) return;
      addrs[cnt] = (uint8_t)val;
      cnt++;
    }
  }

  // If we have some addresses to trigger....
  if (cnt > 0) {
    for (int i = 0; i < cnt; i++) {
      // Address the device
      if (gpib.addrDev(addrs[i], 0)) {
        if (controller.config.isVerb) controller.cmdstream->println(F("Failed to address device"));
        return;
      }
      // Send GTL
      if (gpib.gpibSendCmd(GC_GET))  {
        if (controller.config.isVerb) controller.cmdstream->println(F("Failed to trigger device"));
        return;
      }
      // Unaddress device
      if (gpib.uaddrDev()) {
        if (controller.config.isVerb) controller.cmdstream->println(F("Failed to unlisten GPIB bus"));
        return;
      }
    }

    // Set GPIB controls back to idle state
    gpib.setGpibControls(CIDS);

    if (controller.config.isVerb) controller.cmdstream->println(F("Group trigger completed."));
  }
}


/***** Reset the controller *****/
/*
 * Arduinos can use the watchdog timer to reset the MCU
 * For other devices, we restart the program instead by
 * jumping to address 0x0000. This is not a hardware reset
 * and will not reset a crashed MCU, but it will re-start
 * the interface program and re-initialise all parameters.
 */
void rst_h(char *params, Controller& controller) {
  controller.reset();
}


/***** Serial Poll Handler *****/
void spoll_h(char *params, Controller& controller) {
  char *param;
  uint8_t addrs[15];
  uint8_t sb = 0;
  uint8_t r;
  //  uint8_t i = 0;
  uint8_t j = 0;
  uint16_t val = 0;
  bool all = false;
  bool eoiDetected = false;
  GPIB &gpib = *controller.gpib;

  // Initialise address array
  for (int i = 0; i < 15; i++) {
    addrs[i] = 0;
  }

  // Read parameters
  if (params == NULL) {
    // No parameters - trigger addressed device only
    addrs[0] = controller.config.paddr;
    j = 1;
  } else {
    // Read address parameters into array
    while (j < 15) {
      if (j == 0) {
        param = strtok(params, " \t");
      } else {
        param = strtok(NULL, " \t");
      }
      // The 'all' parameter given?
      if (strncmp(param, "all", 3) == 0) {
        all = true;
        j = 30;
        if (controller.config.isVerb) controller.cmdstream->println(F("Serial poll of all devices requested..."));
        break;
        // Read all address parameters
      } else if (strlen(params) < 3) { // No more than 2 characters
        if (notInRange(param, 1, 30, val, controller)) return;
        addrs[j] = (uint8_t)val;
        j++;
      } else {
        errBadCmd(controller);
        if (controller.config.isVerb) controller.cmdstream->println(F("Invalid parameter"));
        return;
      }
    }
  }

  // Send Unlisten [UNL] to all devices
  if ( gpib.gpibSendCmd(GC_UNL) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll_h: failed to send UNL"));
#endif
    return;
  }

  // Controller addresses itself as listner
  if ( gpib.gpibSendCmd(GC_LAD + controller.config.caddr) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll_h: failed to send LAD"));
#endif
    return;
  }

  // Send Serial Poll Enable [SPE] to all devices
  if ( gpib.gpibSendCmd(GC_SPE) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll_h: failed to send SPE"));
#endif
    return;
  }

  // Poll GPIB address or addresses as set by i and j
  for (int i = 0; i < j; i++) {

    // Set GPIB address in val
    if (all) {
      val = i;
    } else {
      val = addrs[i];
    }

    // Don't need to poll own address
    if (val != controller.config.caddr) {

      // Address a device to talk
      if ( gpib.gpibSendCmd(GC_TAD + val) )  {

#ifdef DEBUG4
        dbSerial->println(F("spoll_h: failed to send TAD"));
#endif
        return;
      }

      // Set GPIB control to controller active listner state (ATN unasserted)
      gpib.setGpibControls(CLAS);

      // Read the response byte (usually device status) using handshake
      r = gpib.gpibReadByte(&sb, &eoiDetected);

      // If we successfully read a byte
      if (!r) {
        if (j > 1) {
          // If all, return specially formatted response: SRQ:addr,status
          // but only when RQS bit set
          if (sb & 0x40) {
            controller.cmdstream->print(F("SRQ:")); controller.cmdstream->print(i); controller.cmdstream->print(F(",")); controller.cmdstream->println(sb, DEC);
            i = j;
          }
        } else {
          // Return decimal number representing status byte
          controller.cmdstream->println(sb, DEC);
          if (controller.config.isVerb) {
            controller.cmdstream->print(F("Received status byte ["));
            controller.cmdstream->print(sb);
            controller.cmdstream->print(F("] from device at address: "));
            controller.cmdstream->println(val);
          }
          i = j;
        }
      } else {
        if (controller.config.isVerb) controller.cmdstream->println(F("Failed to retrieve status byte"));
      }
    }
  }
  if (all) controller.cmdstream->println();

  // Send Serial Poll Disable [SPD] to all devices
  if ( gpib.gpibSendCmd(GC_SPD) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll_h: failed to send SPD"));
#endif
    return;
  }

  // Send Untalk [UNT] to all devices
  if ( gpib.gpibSendCmd(GC_UNT) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll_h: failed to send UNT"));
#endif
    return;
  }

  // Unadress listners [UNL] to all devices
  if ( gpib.gpibSendCmd(GC_UNL) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll_h: failed to send UNL"));
#endif
    return;
  }

  // Set GPIB control to controller idle state
  gpib.setGpibControls(CIDS);

  // Set SRQ to status of SRQ line. Should now be unasserted but, if it is
  // still asserted, then another device may be requesting service so another
  // serial poll will be called from the main loop
  gpib.setSRQ(digitalRead(SRQ) == LOW);
  if (controller.config.isVerb) controller.cmdstream->println(F("Serial poll completed."));

}


/***** Return status of SRQ line *****/
void srq_h(char *params, Controller& controller) {
  //NOTE: LOW=asserted, HIGH=unasserted
  controller.cmdstream->println(!digitalRead(SRQ));
}


/***** Set the status byte (device mode) *****/
void stat_h(char *params, Controller& controller) {
  uint16_t val = 0;
  // A parameter given?
  if (params != NULL) {
    // Byte value given?
    if (notInRange(params, 0, 255, val, controller)) return;
    controller.config.stat = (uint8_t)val;
    if (val & 0x40) {
      setSrqSig();
      if (controller.config.isVerb) controller.cmdstream->println(F("SRQ asserted."));
    } else {
      clrSrqSig();
      if (controller.config.isVerb) controller.cmdstream->println(F("SRQ un-asserted."));
    }
  } else {
    // Return the currently set status byte
    controller.cmdstream->println(controller.config.stat);
  }
}


/***** Save controller configuration *****/
void save_h(char *params, Controller& controller) {
  controller.saveConfig();
}


/***** Show state or enable/disable listen only mode *****/
void lon_h(char *params, Controller& controller) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val, controller)) return;
    controller.isRO = val ? true : false;
    if (controller.isTO) controller.isTO = false; // Talk-only mode must be disabled!
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("LON: "));
      controller.cmdstream->println(val ? "ON" : "OFF") ;
    }
  } else {
    controller.cmdstream->println(controller.isRO);
  }
}



/***** Set the SRQ signal *****/
void setSrqSig() {
  // Set SRQ line to OUTPUT HIGH (asserted)
  setGpibState(0b01000000, 0b01000000, 1);
  setGpibState(0b00000000, 0b01000000, 0);
}


/***** Clear the SRQ signal *****/
void clrSrqSig() {
  // Set SRQ line to INPUT_PULLUP (un-asserted)
  setGpibState(0b00000000, 0b01000000, 1);
  setGpibState(0b01000000, 0b01000000, 0);
}


/***********************************/
/***** CUSTOM COMMAND HANDLERS *****/
/***********************************/

/***** All serial poll *****/
/*
 * Polls all devices, not just the currently addressed instrument
 * This is an alias wrapper for ++spoll all
 */
void aspoll_h(char *params, Controller& controller) {
  spoll_h((char*)"all", controller);
}


/***** Send Universal Device Clear *****/
/*
 * The universal Device Clear (DCL) is unaddressed and affects all devices on the Gpib bus.
 */
void dcl_h(char *params, Controller& controller) {
  GPIB &gpib = *controller.gpib;
  if ( gpib.gpibSendCmd(GC_DCL) )  {
    if (controller.config.isVerb) controller.cmdstream->println(F("Sending DCL failed"));
    return;
  }
  // Set GPIB controls back to idle state
  gpib.setGpibControls(CIDS);
}


/***** Re-load default configuration *****/
void default_h(char *params, Controller& controller) {
  controller.initConfig();
}


/***** Show or set end of receive character(s) *****/
void eor_h(char *params, Controller& controller) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 15, val, controller)) return;
    controller.config.eor = (uint8_t)val;
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("Set EOR to: "));
      controller.cmdstream->println(val);
    };
  } else {
    if (controller.config.eor>7) controller.config.eor = 0;  // Needed to reset FF read from EEPROM after FW upgrade
    controller.cmdstream->println(controller.config.eor);
  }
}


/***** Parallel Poll Handler *****/
/*
 * Device must be set to respond on DIO line 1 - 8
 */
void ppoll_h(char *params, Controller& controller) {
  uint8_t sb = 0;
  GPIB &gpib = *controller.gpib;

  // Poll devices
  // Start in controller idle state
  gpib.setGpibControls(CIDS);
  delayMicroseconds(20);
  // Assert ATN and EOI
  setGpibState(0b00000000, 0b10010000, 0);
  //  setGpibState(0b10010000, 0b00000000, 0b10010000);
  delayMicroseconds(20);
  // Read data byte from GPIB bus without handshake
  sb = readGpibDbus();
  // Return to controller idle state (ATN and EOI unasserted)
  gpib.setGpibControls(CIDS);

  // Output the response byte
  controller.cmdstream->println(sb, DEC);

  if (controller.config.isVerb) controller.cmdstream->println(F("Parallel poll completed."));
}


/***** Assert or de-assert REN 0=de-assert; 1=assert *****/
void ren_h(char *params, Controller& controller) {
#if defined (SN7516X) && not defined (SN7516X_DC)
  params = params;
  controller.cmdstream->println(F("Unavailable")) ;
#else
  // char *stat;
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val, controller)) return;
    digitalWrite(REN, (val ? LOW : HIGH));
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("REN: "));
      controller.cmdstream->println(val ? "REN asserted" : "REN un-asserted") ;
    };
  } else {
    controller.cmdstream->println(digitalRead(REN) ? 0 : 1);
  }
#endif
}


/***** Enable verbose mode 0=OFF; 1=ON *****/
void verb_h(char *params, Controller& controller) {
  controller.config.isVerb = !controller.config.isVerb;
  controller.cmdstream->print("Verbose: ");
  controller.cmdstream->println(controller.config.isVerb ? "ON" : "OFF");
}


/***** Set version string *****/
/* Replace the standard AR488 version string with something else
 *  NOTE: some instrument software requires a sepcific version string to ID the interface
 */
void setvstr_h(char *params, Controller& controller) {
  char idparams[64];
  memset(idparams, '\0', 64);
  strncpy(idparams, "verstr ", 7);
  strncat(idparams, params, 64-7);

/*
controller.cmdstream->print(F("Plen: "));
controller.cmdstream->println(plen);
controller.cmdstream->print(F("Params: "));
controller.cmdstream->println(params);
controller.cmdstream->print(F("IdParams: "));
controller.cmdstream->println(idparams);
*/

  id_h(idparams, controller);

/*
  if (params != NULL) {
    len = strlen(params);
    if (len>47) len=47; // Ignore anything over 47 characters
    memset(controller.config.vstr, '\0', 48);
    strncpy(controller.config.vstr, params, len);
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("Changed version string to: "));
      controller.cmdstream->println(params);
    };
  }
*/
}


/***** Talk only mode *****/
void ton_h(char *params, Controller& controller) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val, controller)) return;
    controller.isTO = val ? true : false;
    if (controller.isTO) controller.isRO = false; // Read-only mode must be disabled in TO mode!
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("TON: "));
      controller.cmdstream->println(val ? "ON" : "OFF") ;
    }
  } else {
    controller.cmdstream->println(controller.isTO);
  }
}


/***** SRQ auto - show or enable/disable automatic spoll on SRQ *****/
/*
 * In device mode, when the SRQ interrupt is triggered and SRQ
 * auto is set to 1, a serial poll is conducted automatically
 * and the status byte for the instrument requiring service is
 * automatically returned. When srqauto is set to 0 (default)
 * an ++spoll command needs to be given manually to return
 * the status byte.
 */
void srqa_h(char *params, Controller& controller) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val, controller)) return;
    switch (val) {
      case 0:
        controller.isSrqa = false;
        break;
      case 1:
        controller.isSrqa = true;
        break;
    }
    if (controller.config.isVerb) controller.cmdstream->println(controller.isSrqa ? "SRQ auto ON" : "SRQ auto OFF") ;
  } else {
    controller.cmdstream->println(controller.isSrqa);
  }
}


/***** Repeat a given command and return result *****/
void repeat_h(char *params, Controller& controller) {

  uint16_t count;
  uint16_t tmdly;
  char *param;
  GPIB &gpib = *controller.gpib;

  if (params != NULL) {
    // Count (number of repetitions)
    param = strtok(params, " \t");
    if (strlen(param) > 0) {
      if (notInRange(param, 2, 255, count, controller)) return;
    }
    // Time delay (milliseconds)
    param = strtok(NULL, " \t");
    if (strlen(param) > 0) {
      if (notInRange(param, 0, 30000, tmdly, controller)) return;
    }

    // Pointer to remainder of parameters string
    param = strtok(NULL, "\n\r");
    if (strlen(param) > 0) {
      for (uint16_t i = 0; i < count; i++) {
        // Send string to instrument
		gpib.gpibSendData(param, strlen(param), false);
        delay(tmdly);
        gpib.gpibReceiveData();
      }
    } else {
      errBadCmd(controller);
      if (controller.config.isVerb) controller.cmdstream->println(F("Missing parameter"));
      return;
    }
  } else {
    errBadCmd(controller);
    if (controller.config.isVerb) controller.cmdstream->println(F("Missing parameters"));
  }

}


/***** Run a macro *****/
void macro_h(char *params, Controller& controller) {
#ifdef USE_MACROS
  uint16_t val;
  uint8_t dlen = 0;
  char * macro; // Pointer to keyword following ++id
  char * keyword; // Pointer to supplied data (remaining characters in buffer)

  if (params != NULL) {
    macro = strtok(params, " \t");
    if (notInRange(macro, 0, 9, val, controller)) return;
    keyword = macro + strlen(macro) + 1;
    dlen = strlen(keyword);
	if (dlen) {
      if (strncmp(keyword, "set", 3)==0) {
		controller.editMacro = (uint8_t)val;
	  }
	  else if (strncmp(keyword, "del", 3)==0) {
		deleteMacro((uint8_t)val);
	  }
	  else
		// invalid sub command
		return;
	} else
	  controller.runMacro = (uint8_t)val;
  } else {
	controller.displayMacros();
  }
#else
  memset(params, '\0', 5);
  controller.cmdstream->println(F("Disabled"));
#endif
}


/***** Bus diagnostics *****/
/*
 * Usage: xdiag mode byte
 * mode: 0=data bus; 1=control bus
 * byte: byte to write on the bus
 * Note: values to switch individual bits = 1,2,4,8,10,20,40,80
 */
void xdiag_h(char *params, Controller& controller){
  char *param;
  uint8_t mode = 0;
  uint8_t val = 0;
  GPIB &gpib = *controller.gpib;

  // Get first parameter (mode = 0 or 1)
  param = strtok(params, " \t");
  if (param != NULL) {
    if (strlen(param)<4){
      mode = atoi(param);
      if (mode>2) {
        controller.cmdstream->println(F("Invalid: 0=data bus; 1=control bus"));
        return;
      }
    }
  }
  // Get second parameter (8 bit byte)
  param = strtok(NULL, " \t");
  if (param != NULL) {
    if (strlen(param)<4){
      val = atoi(param);
    }

    if (mode) {   // Control bus
      // Set to required state
      setGpibState(0xFF, 0xFF, 1);  // Set direction
      setGpibState(~val, 0xFF, 0);  // Set state (low=asserted so must be inverse of value)
      // Reset after 10 seconds
      delay(10000);
      if (controller.config.cmode==2) {
        gpib.setGpibControls(CINI);
      }else{
        gpib.setGpibControls(DINI);
      }
    }else{        // Data bus
      // Set to required value
      setGpibDbus(val);
      // Reset after 10 seconds
      delay(10000);
      setGpibDbus(0);
    }
  }

}


/****** Timing parameters ******/

void tmbus_h(char *params, Controller& controller) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 30000, val, controller)) return;
    controller.config.tmbus = val;
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("TmBus set to: "));
      controller.cmdstream->println(val);
    };
  } else {
    controller.cmdstream->println(controller.config.tmbus, DEC);
  }
}


/***** Set device ID *****/
/*
 * Sets the device ID parameters including:
 * ++id verstr - version string (same as ++setvstr)
 * ++id name   - short name of device (e.g. HP3478A) up to 15 characters
 * ++id serial - serial number up to 9 digits long
 */
void id_h(char *params, Controller& controller) {
  uint8_t dlen = 0;
  char * keyword; // Pointer to keyword following ++id
  char * datastr; // Pointer to supplied data (remaining characters in buffer)
  char serialStr[10];

#ifdef DEBUG10
  controller.cmdstream->print(F("Params: "));
  controller.cmdstream->println(params);
#endif

  if (params != NULL) {
    keyword = strtok(params, " \t");
    datastr = keyword + strlen(keyword) + 1;
    dlen = strlen(datastr);
    if (dlen) {
      if (strncmp(keyword, "verstr", 6)==0) {
#ifdef DEBUG10
        controller.cmdstream->print(F("Keyword: "));
        controller.cmdstream->println(keyword);
        controller.cmdstream->print(F("DataStr: "));
        controller.cmdstream->println(datastr);
#endif
        if (dlen>0 && dlen<48) {
#ifdef DEBUG10
        controller.cmdstream->println(F("Length OK"));
#endif
          memset(controller.config.vstr, '\0', 48);
          strncpy(controller.config.vstr, datastr, dlen);
          if (controller.config.isVerb) controller.cmdstream->print(F("VerStr: "));
		  controller.cmdstream->println(controller.config.vstr);
        }else{
          if (controller.config.isVerb)
			  controller.cmdstream->println(F("Length of version string must not exceed 48 characters!"));
          errBadCmd(controller);
        }
        return;
      }
      if (strncmp(keyword, "name", 4)==0) {
        if (dlen>0 && dlen<16) {
          memset(controller.config.sname, '\0', 16);
          strncpy(controller.config.sname, datastr, dlen);
        }else{
          if (controller.config.isVerb) controller.cmdstream->println(F("Length of name must not exceed 15 characters!"));
          errBadCmd(controller);
        }
        return;
      }
      if (strncmp(keyword, "serial", 6)==0) {
        if (dlen < 10) {
          controller.config.serial = atol(datastr);
        }else{
          if (controller.config.isVerb) controller.cmdstream->println(F("Serial number must not exceed 9 characters!"));
          errBadCmd(controller);
        }
        return;
      }
//      errBadCmd(controller);
    }else{
      if (strncmp(keyword, "verstr", 6)==0) {
        controller.cmdstream->println(controller.config.vstr);
        return;
      }
      if (strncmp(keyword, "name", 4)==0) {
        controller.cmdstream->println(controller.config.sname);
        return;
      }
      if (strncmp(keyword, "serial", 6)==0) {
        memset(serialStr, '\0', 10);
        snprintf(serialStr, 10, "%09lu", controller.config.serial);  // Max str length = 10-1 i.e 9 digits + null terminator
        controller.cmdstream->println(serialStr);
        return;
      }
    }
  }
  errBadCmd(controller);
}


void idn_h(char * params, Controller& controller){
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 2, val, controller)) return;
    controller.config.idn = (uint8_t)val;
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("Sending IDN: "));
      controller.cmdstream->print(val ? "Enabled" : "Disabled");
      if (val==2) controller.cmdstream->print(F(" with serial number"));
      controller.cmdstream->println();
    };
  } else {
    controller.cmdstream->println(controller.config.idn, DEC);
  }
}


/***** Check whether a parameter is in range *****/
/* Convert string to integer and check whether value is within
 * lowl to higl inclusive. Also returns converted text in param
 * to a uint16_t integer in rval. Returns true if successful,
 * false if not
*/
bool notInRange(char *param, uint16_t lowl, uint16_t higl, uint16_t &rval, Controller& controller) {

  // Null string passed?
  if (strlen(param) == 0) return true;

  // Convert to integer
  rval = 0;
  rval = atoi(param);

  // Check range
  if (rval < lowl || rval > higl) {
    errBadCmd(controller);
    if (controller.config.isVerb) {
      controller.cmdstream->print(F("Valid range is between "));
      controller.cmdstream->print(lowl);
      controller.cmdstream->print(F(" and "));
      controller.cmdstream->println(higl);
    }
    return true;
  }
  return false;
}

/***** Unrecognized command *****/
void errBadCmd(Controller& controller) {
  controller.cmdstream->println(F("Unrecognized command"));
}


#ifdef AR488_WIFI_EN
/***** Configure Wifi connection *****/
/*
 * Sets the wifi parameterw:
 * ++wifi ssid - wifi endpoint
 * ++wifi pass - passphrase
 * ++wifi connect - to (re)start the wifi connection
 */
void wifi_h(char *params, Controller& controller) {
  uint8_t dlen = 0;
  char * keyword; // Pointer to keyword following ++id
  char * datastr; // Pointer to supplied data (remaining characters in buffer)
  char serialStr[10];

  if (params != NULL) {
    keyword = strtok(params, " \t");
    datastr = keyword + strlen(keyword) + 1;
    dlen = strlen(datastr);
    if (dlen) {
      if (strncmp(keyword, "ssid", 6)==0) {
        if (dlen>0 && dlen<32) {
          memset(controller.config.ssid, '\0', 32);
          strncpy(controller.config.ssid, datastr, dlen);
        } else {
          if (controller.config.isVerb)
			controller.cmdstream->println(F("Length of ssid string must not exceed 31 characters!"));
          errBadCmd(controller);
        }
        return;
      }
      else if (strncmp(keyword, "pass", 4)==0) {
        if (dlen>0 && dlen<64) {
          memset(controller.config.passkey, '\0', 64);
          strncpy(controller.config.passkey, datastr, dlen);
        } else {
          if (controller.config.isVerb)
			controller.cmdstream->println(F("Length of pass must not exceed 63 characters!"));
          errBadCmd(controller);
        }
        return;
      }
    } else {
      if (strncmp(keyword, "ssid", 6)==0) {
        controller.cmdstream->println(controller.config.ssid);
        return;
      }
      else if (strncmp(keyword, "pass", 4)==0) {
        controller.cmdstream->println(controller.config.passkey);
        return;
      }
      else if (strncmp(keyword, "connect", 7)==0) {
        controller.setupWifi();
        return;
      }
      else if (strncmp(keyword, "scan", 4)==0) {
        controller.scanWifi();
        return;
      }
    }
  }
  errBadCmd(controller);
}
#endif
