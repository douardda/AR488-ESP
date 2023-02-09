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
  { "addr",        3, &Controller::addr_h      },
  { "auto",        2, &Controller::amode_h     },
  { "clr",         2, &Controller::clr_h       },
  { "eoi",         3, &Controller::eoi_h       },
  { "eor",         3, &Controller::eor_h       },
  { "eos",         3, &Controller::eos_h       },
  { "eot_char",    3, &Controller::eot_char_h  },
  { "eot_enable",  3, &Controller::eot_en_h    },
#ifdef HAS_HELP_COMMAND
  { "help",        3, &Controller::help_h      },
#endif
  { "ifc",         2, &Controller::ifc_h       },
  { "llo",         2, &Controller::llo_h       },
  { "loc",         2, &Controller::loc_h       },
  { "lon",         1, &Controller::lon_h       },
  { "mode" ,       3, &Controller::cmode_h     },
  { "read",        2, &Controller::read_h      },
  { "read_tmo_ms", 2, &Controller::rtmo_h      },
  { "rst",         3, &Controller::rst_h       },
  { "savecfg",     3, &Controller::save_h      },
  { "spoll",       2, &Controller::spoll_h     },
  { "srq",         2, &Controller::srq_h       },
  { "status",      1, &Controller::stat_h      },
  { "trg",         2, &Controller::trg_h       },
  { "ver",         3, &Controller::ver_h       },
  // non-prologix commands
  { "allspoll",    2, &Controller::allspoll_h  },
  { "findrqs",     2, &Controller::findrqs_h   },
  { "findlstn",    2, &Controller::findlstn_h  },
  { "dcl",         2, &Controller::dcl_h       },
  { "default",     3, &Controller::default_h   },
  { "id",          3, &Controller::id_h        },
  { "idn",         3, &Controller::idn_h       },
#ifdef USE_MACROS
  { "macro",       3, &Controller::macro_h     },
#endif
  { "ppoll",       2, &Controller::ppoll_h     },
  { "prompt",      3, &Controller::prompt_h    },
  { "ren",         2, &Controller::ren_h       },
  { "repeat",      2, &Controller::repeat_h    },
  { "setvstr",     3, &Controller::setvstr_h   },
  { "srqauto",     2, &Controller::srqa_h      },
  { "tct",         2, &Controller::tct_h       },
  { "ton",         1, &Controller::ton_h       },
  { "tmbus",       3, &Controller::tmbus_h     },
  { "verbose",     3, &Controller::verb_h      },
#ifdef AR488_WIFI_ENABLE
  { "wifi",        3, &Controller::wifi_h      },
#endif
  { "xdiag",       3, &Controller::xdiag_h     }
};




/***** Extract command and pass to handler *****/
void Controller::getCmd(char *buffr) {

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
    if (cmdHidx[i].opmode & config.cmode) {
      // If its a command with parameters
      // Copy command parameters to params and call handler with parameters
      params = token + strlen(token) + 1;

      // If command parameters were specified
      if (strlen(params) > 0) {
#ifdef DEBUG1
        dbSerial->print(F("Calling handler with parameters: ")); dbSerial->println(params);
#endif
        // Call handler with parameters specified
        (this->*(cmdHidx[i].handler))(params);
      }else{
        // Call handler without parameters
        (this->*(cmdHidx[i].handler))(NULL);
      }
    } else {
      errBadCmd();
      if (config.isVerb) cmdstream->println(F("Command not available in this mode."));
    }

  } else {
    // No valid command found
    errBadCmd();
  }

}

/*************************************/
/***** STANDARD COMMAND HANDLERS *****/
/*************************************/

/***** Show or change device address *****/
void Controller::addr_h(char *params) {
  //  char *param, *stat;
  char *param;
  uint16_t val;
  if (params != NULL) {

    // Primary address
    param = strtok(params, " \t");
    if (notInRange(param, 1, 30, val)) return;
    if (val == config.caddr) {
      errBadCmd();
      if (config.isVerb) cmdstream->println(F("That is my address! Address of a remote device is required."));
      return;
    }
    config.paddr = val;
    if (config.isVerb) {
      cmdstream->print(F("Set device primary address to: "));
      cmdstream->println(val);
    }

    // Secondary address
    config.saddr = 0;
    val = 0;
    param = strtok(NULL, " \t");
    if (param != NULL) {
      if (notInRange(param, 96, 126, val)) return;
      config.saddr = val;
      if (config.isVerb) {
        cmdstream->print("Set device secondary address to: ");
        cmdstream->println(val);
      }
    }

  } else {
    cmdstream->print(config.paddr);
    if (config.saddr > 0) {
      cmdstream->print(F(" "));
      cmdstream->print(config.saddr);
    }
    cmdstream->println();
  }
}


/***** Show or set read timout *****/
void Controller::rtmo_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 1, 32000, val)) return;
    config.rtmo = val;
    if (config.isVerb) {
      cmdstream->print(F("Set [read_tmo_ms] to: "));
      cmdstream->print(val);
      cmdstream->println(F(" milliseconds"));
    }
  } else {
    cmdstream->println(config.rtmo);
  }
}


/***** Show or set end of send character *****/
void Controller::eos_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 3, val)) return;
    config.eos = (uint8_t)val;
    if (config.isVerb) {
      cmdstream->print(F("Set EOS to: "));
      cmdstream->println(val);
    };
  } else {
    cmdstream->println(config.eos);
  }
}


/***** Show or set EOI assertion on/off *****/
void Controller::eoi_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    config.eoi = val ? true : false;
    if (config.isVerb) {
      cmdstream->print(F("Set EOI assertion: "));
      cmdstream->println(val ? "ON" : "OFF");
    };
  } else {
    cmdstream->println(config.eoi);
  }
}

/***** Show or set EOI assertion on/off *****/
void Controller::tct_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 30, val)) return;
    if (val == config.caddr) {
      errBadCmd();
      if (config.isVerb) cmdstream->println(F("That is my address! Address of a remote device is required."));
      return;
    }
    if (gpib->takeControl(val)) {
      if (config.isVerb)
        cmdstream->println("Failed to send the TCT message");
    }
  } else {
    if (config.isVerb) {
      cmdstream->println("The address of the device is expected.");
    }
  }
}


/***** Show or set interface to controller/device mode *****/
void Controller::cmode_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    switch (val) {
      case 0:
        config.cmode = 1;
        gpib->initDevice();
        break;
      case 1:
        config.cmode = 2;
        gpib->initController();
        break;
    }
    if (config.isVerb) {
      cmdstream->print(F("Interface mode set to: "));
      cmdstream->println(val ? "CONTROLLER" : "DEVICE");
    }
  } else {
    cmdstream->println(config.cmode - 1);
  }
}


/***** Show or enable/disable sending of end of transmission character *****/
void Controller::eot_en_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    config.eot_en = val ? true : false;
    if (config.isVerb) {
      cmdstream->print(F("Appending of EOT character: "));
      cmdstream->println(val ? "ON" : "OFF");
    }
  } else {
    cmdstream->println(config.eot_en);
  }
}


/***** Show or set end of transmission character *****/
void Controller::eot_char_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 255, val)) return;
    config.eot_ch = (uint8_t)val;
    if (config.isVerb) {
      cmdstream->print(F("EOT set to ASCII character: "));
      cmdstream->println(val);
    };
  } else {
    cmdstream->println(config.eot_ch, DEC);
  }
}


/***** Show or enable/disable auto mode *****/
void Controller::amode_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 3, val)) return;
    config.amode = (uint8_t)val;
    if (config.amode < 3) aRead = false;
    if (config.isVerb) {
    amode_h(NULL);
    if (val > 0) {
    cmdstream->println(F("WARNING: automode ON can cause some devices to generate"));
    cmdstream->println(F("         'addressed to talk but nothing to say' errors"));
    }
  }
  } else {
    if (config.isVerb) {
    cmdstream->print(String("Auto mode: ") + config.amode + " (");
    switch (config.amode) {
    case 0:
      cmdstream->print(F("off"));
      break;
    case 1:
      cmdstream->print(F("prologix mode"));
      break;
    case 2:
      cmdstream->print(F("on query"));
      break;
    case 3:
      cmdstream->print(F("continuous"));
      break;
    }
    cmdstream->println(F(")"));
  }
  else
    cmdstream->println(config.amode);
  }
}


/***** Display the controller version string *****/
void Controller::ver_h(char *params) {
  // If "real" requested
  if (params != NULL && strncmp(params, "real", 3) == 0) {
    cmdstream->println(F(FWVER));
    // Otherwise depends on whether we have a custom string set
  } else {
    if (strlen(config.vstr) > 0) {
      cmdstream->println(config.vstr);
    } else {
      cmdstream->println(F(FWVER));
    }
  }
}


/***** Address device to talk and read the sent data *****/
void Controller::read_h(char *params) {
  // Clear read flags
  gpib->rEoi = false;
  gpib->rEbt = false;
  // Read any parameters
  if (params != NULL) {
    if (strlen(params) > 3) {
      if (config.isVerb) cmdstream->println(F("Invalid termination character - ignored!"));
    } else if (strncmp(params, "eoi", 3) == 0) { // Read with eoi detection
      gpib->rEoi = true;
    } else { // Assume ASCII character given and convert to an 8 bit byte
      gpib->rEbt = true;
      gpib->eByte = atoi(params);
    }
  }
  if (config.amode == 3) {
    // In auto continuous mode we set this flag to indicate we are ready for continuous read
    aRead = true;
  } else {
    // If auto mode is disabled we do a single read
    gpib->gpibReceiveData();
  }
}


/***** Send device clear (usually resets the device to power on state) *****/
void Controller::clr_h(char *params) {
  if (gpib->addrDev(config.paddr, 0)) {
    if (config.isVerb) cmdstream->println(F("Failed to address device"));
    return;
  }
  if (gpib->gpibSendCmd(GC_SDC))  {
    if (config.isVerb) cmdstream->println(F("Failed to send SDC"));
    return;
  }
  if (gpib->uaddrDev()) {
    if (config.isVerb) cmdstream->println(F("Failed to untalk GPIB bus"));
    return;
  }
  // Set GPIB controls back to idle state
  gpib->setGpibControls(CIDS);
}


/***** Send local lockout command *****/
void Controller::llo_h(char *params) {
  // NOTE: REN *MUST* be asserted (LOW)
  if (digitalRead(REN)==LOW) {
    // For 'all' send LLO to the bus without addressing any device - devices will show REM
    if (params != NULL) {
      if (0 == strncmp(params, "all", 3)) {
        if (gpib->gpibSendCmd(GC_LLO)) {
          if (config.isVerb) cmdstream->println(F("Failed to send universal LLO."));
        }
      }
    } else {
      // Address current device
      if (gpib->addrDev(config.paddr, 0)) {
        if (config.isVerb) cmdstream->println(F("Failed to address the device."));
        return;
      }
      // Send LLO to currently addressed device
      if (gpib->gpibSendCmd(GC_LLO)) {
        if (config.isVerb) cmdstream->println(F("Failed to send LLO to device"));
        return;
      }
      // Unlisten bus
      if (gpib->uaddrDev()) {
        if (config.isVerb) cmdstream->println(F("Failed to unlisten the GPIB bus"));
        return;
      }
    }
  }
  // Set GPIB controls back to idle state
  gpib->setGpibControls(CIDS);
}


/***** Send Go To Local (GTL) command *****/
void Controller::loc_h(char *params) {
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
      if (gpib->addrDev(config.paddr, 0)) {
        if (config.isVerb) cmdstream->println(F("Failed to address device."));
        return;
      }
      // Send GTL
      if (gpib->gpibSendCmd(GC_GTL)) {
        if (config.isVerb) cmdstream->println(F("Failed sending LOC."));
        return;
      }
      // Unlisten bus
      if (gpib->uaddrDev()) {
        if (config.isVerb) cmdstream->println(F("Failed to unlisten GPIB bus."));
        return;
      }
      // Set GPIB controls back to idle state
      gpib->setGpibControls(CIDS);
    }
  }
}


/***** Assert IFC for 150 microseconds *****/
/* This indicates that the AR488 the Controller-in-Charge on
 * the bus and causes all interfaces to return to their idle
 * state
 */
void Controller::ifc_h(char *params) {
  gpib->assertIfc();
}


/***** Send a trigger command *****/
void Controller::trg_h(char *params) {
  char *param;
  uint8_t addrs[15];
  uint16_t val = 0;
  uint8_t cnt = 0;

  // Initialise address array
  for (int i = 0; i < 15; i++) {
    addrs[i] = 0;
  }

  // Read parameters
  if (params == NULL) {
    // No parameters - trigger addressed device only
    addrs[0] = config.paddr;
    cnt++;
  } else {
    // Read address parameters into array
  for(param=strtok(params, " \t"), cnt=0;
    (param != NULL) && (cnt < 15);
    param = strtok(NULL, " \t"), ++cnt) {
      if (notInRange(param, 1, 30, val)) return;
      addrs[cnt] = (uint8_t)val;
    }
  }
  if (config.isVerb) cmdstream->println(String("Got ") + cnt + " add to trigger");

  // If we have some addresses to trigger....
  if (cnt > 0) {
    for (int i = 0; i < cnt; i++) {
      // Address the device
      if (gpib->addrDev(addrs[i], 0)) {
        if (config.isVerb)
      cmdstream->println(String("Failed to address device ") + addrs[i]);
        continue;
      }
      // Send GTL
      if (gpib->gpibSendCmd(GC_GET))  {
        if (config.isVerb)
      cmdstream->println(String("Failed to send command to device ") + addrs[i]);
        continue;
      }
      // Unaddress device
      if (gpib->uaddrDev()) {
        if (config.isVerb)
      cmdstream->println(String("Failed to unaddress device ") + addrs[i]);
        continue;
      }
    }

    // Set GPIB controls back to idle state
    gpib->setGpibControls(CIDS);

    if (config.isVerb)
    cmdstream->println(F("Group trigger completed."));
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
void Controller::rst_h(char *params) {
  reset();
}




/***** Return status of SRQ line *****/
void Controller::srq_h(char *params) {
  //NOTE: LOW=asserted, HIGH=unasserted
  cmdstream->println(!digitalRead(SRQ));
}


/***** Set the status byte (device mode) *****/
void Controller::stat_h(char *params) {
  uint16_t val = 0;
  // A parameter given?
  if (params != NULL) {
    // Byte value given?
    if (notInRange(params, 0, 255, val)) return;
    config.stat = (uint8_t)val;
    if (val & 0x40) {
      gpib->setSrqSig();
      if (config.isVerb) cmdstream->println(F("SRQ asserted."));
    } else {
      gpib->clrSrqSig();
      if (config.isVerb) cmdstream->println(F("SRQ un-asserted."));
    }
  } else {
    // Return the currently set status byte
    cmdstream->println(config.stat);
  }
}


/***** Save controller configuration *****/
void Controller::save_h(char *params) {
  saveConfig();
}


/***** Show state or enable/disable listen only mode *****/
void Controller::lon_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    isRO = val ? true : false;
    if (isTO) isTO = false; // Talk-only mode must be disabled!
    if (config.isVerb) {
      cmdstream->print(F("LON: "));
      cmdstream->println(val ? "ON" : "OFF") ;
    }
  } else {
    cmdstream->println(isRO);
  }
}


/***********************************/
/***** CUSTOM COMMAND HANDLERS *****/
/***********************************/


/*
FINDRQS
BEGIN Find Device Requesting Service
  Initialize pointer to top of address list
  Assert ATN TRUE
  Send IEEE 488.1 UNL remote message
  Send controller's LAG
  Send IEEE 488.1 SPE remote message
  REPEAT
    Send TAG of address pointed to
    Set ATN FALSE
    Handshake a DAB (STB & RQS)
    Advance pointer
    Set ATN TRUE
  UNTIL RQS is true or pointer is past the end of the address list
  Send IEEE 488.1 SPD remote message
  Send IEEE 488.1 UNT remote message
  IF RQS is TRUE
    THEN
      return last address sent, last status byte received
    ELSE
      return error
END Find Device Requesting Service
 */

/*
ALLSPOLL
BEGIN Serial Poll All Devices
  Initialize pointer to top of address list
  Initialize status byte list
  Set ATN TRUE
  Send IEEE 488.1 UNL remote message
  Send controller's LAG
  Send IEEE 488.1 SPE remote message
  WHILE pointer is not past the end of the address list
    Send TAG of address pointed to
    Set ATN FALSE
    Handshake a DAB (STB & RQS)
    Store the STB & RQS in the status byte list
    Advance pointer
    Set ATN TRUE
  END WHILE
  Send IEEE 488.1 SPD remote message
  Send IEEE 488.1 UNT remote message
END Serial Poll All Devices
*/

/***** spoll  *****/
/*
 * Spoll the current addressed device, or the device at given address if any.
 *
 * Display the returned value, if any.
 */
void Controller::spoll_h(char *params) {
  spoll(params, 0);
}

/***** ALLSPOLL 488.2 common protocol *****/
/*
 * Spoll all the devices given as argument
 *
 * Display the list of status bytes.
 */
void Controller::allspoll_h(char *params) {
  spoll(params, 1);
}

/***** FINDRQS 488.2 common protocol *****/
/*
 * Look for the device requesting service (typically as a result of a device
 * addressing the SRQ line).
 *
 * Display 'SRQ:<addr>,<statusbyte>' for the first device having
 * the SRQ bit set.
 */
void Controller::findrqs_h(char *params) {
  spoll(params, 2);
}

/***** Low level Serial Poll implementation *****/
void Controller::spoll(char *params, int mode) {
  // mode: 0=spoll one, 1=allspoll, 2=findsrq
  char *param;
  uint8_t addrs[31];
  uint8_t sb = 0;
  uint8_t r;
  //  uint8_t i = 0;
  uint8_t n_addr = 0;
  uint16_t val = 0;
  bool eoiDetected = false;

  // Initialise address array
  for (int i = 0; i < 31; i++) {
    addrs[i] = 0;
  }

  // Read parameters
  if (params == NULL) {
    // No parameters - trigger addressed device only
    addrs[0] = config.paddr;
    n_addr = 1;
  } else {
    // Read address parameters into array
    for (param=strtok(params, " \t"), n_addr=0;
        (param != NULL) && (n_addr < 31);
        param = strtok(NULL, " \t"), ++n_addr) {
      if ((strlen(params) < 2) && !(notInRange(param, 0, 30, val))) {
        addrs[n_addr] = (uint8_t)val;
      } else {
        errBadCmd();
        if (config.isVerb) cmdstream->println(F("Invalid parameter"));
        return;
      }
    }
  }
  if (mode == 0 && n_addr > 1) {
    // only addr is supported in default spoll mode; secondary addresses are not yet supported
    errBadCmd();
    if (config.isVerb) cmdstream->println(F("Invalid parameter"));
    return;
  }

  if (config.isVerb)
    cmdstream->println(String("Got ") + n_addr + " devices to spoll");

  // Send Unlisten [UNL] to all devices
  if ( gpib->gpibSendCmd(GC_UNL) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll: failed to send UNL"));
#endif
    return;
  }

  // Controller addresses itself as listner
  if ( gpib->gpibSendCmd(GC_LAD + config.caddr) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll: failed to send LAD"));
#endif
    return;
  }

  // Send Serial Poll Enable [SPE] to all devices
  if ( gpib->gpibSendCmd(GC_SPE) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll: failed to send SPE"));
#endif
    return;
  }

  // Poll GPIB address or addresses as set by i and j
  for (int i=0; i < n_addr; i++) {

    // Set GPIB address in val
    val = addrs[i];

    // Don't need to poll own address
    if (val != config.caddr) {
      if (config.isVerb) {
        cmdstream->println(String("Polling ") + val);
      }
      // Address a device to talk
      if ( gpib->gpibSendCmd(GC_TAD + val) )  {

#ifdef DEBUG4
        dbSerial->println(F("spoll: failed to send TAD"));
#endif
        return;
      }

      // Set GPIB control to controller active listner state (ATN unasserted)
      gpib->setGpibControls(CLAS);

      // Read the response byte (usually device status) using handshake
      r = gpib->gpibReadByte(&sb, &eoiDetected);

      // If we successfully read a byte
      if (!r) {
        if (mode == 2) {
          // If in FINDRQS mode, return specially formatted response: SRQ:addr,status
          // but only when RQS bit set
          if (sb & 0x40) {
            cmdstream->print(String(F("SRQ:")) + val + F(","));
            cmdstream->print(sb, DEC);
            break;
          }
        } else if (mode == 1) {
          // ALLSPOLL mode, return a specially formatted response: 'addr:status '
          cmdstream->print(String(val) + F(":") + sb + " ");
        } else {
          // SPOLL mode
          // Return decimal number representing status byte
          cmdstream->print(sb, DEC);
        }
      }
    }
  }
  cmdstream->println();

  // Send Serial Poll Disable [SPD] to all devices
  if ( gpib->gpibSendCmd(GC_SPD) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll: failed to send SPD"));
#endif
    return;
  }

  // Send Untalk [UNT] to all devices
  if ( gpib->gpibSendCmd(GC_UNT) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll: failed to send UNT"));
#endif
    return;
  }

  // Unadress listners [UNL] to all devices
  if ( gpib->gpibSendCmd(GC_UNL) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll: failed to send UNL"));
#endif
    return;
  }

  // Set GPIB control to controller idle state
  gpib->setGpibControls(CIDS);

  // Set SRQ to status of SRQ line. Should now be unasserted but, if it is
  // still asserted, then another device may be requesting service so another
  // serial poll will be called from the main loop
  gpib->setSRQ(digitalRead(SRQ) == LOW);
  if (config.isVerb) cmdstream->println(F("Serial poll completed."));

}

/***** FINDLSTN 488.2 common protocol *****/
/*
 * Looks for listeners on the bus
 * Does only look for primary addresses for now
 */
void Controller::findlstn_h(char *params) {
  uint8_t addrs[31];
  if ( gpib->findListeners(addrs) )  {
    if (config.isVerb) cmdstream->println(F("FINDLSTN failed"));
    return;
  }
  for(int i=0; i<31; i++) {
    if (addrs[i] < 31) {
      cmdstream->print(addrs[i]);
      cmdstream->print(" ");
    }
  }
  cmdstream->println();
}


/***** Send Universal Device Clear *****/
/*
 * The universal Device Clear (DCL) is unaddressed and affects all devices on the Gpib bus.
 */
void Controller::dcl_h(char *params) {
  if ( gpib->gpibSendCmd(GC_DCL) )  {
    if (config.isVerb) cmdstream->println(F("Sending DCL failed"));
    return;
  }
  // Set GPIB controls back to idle state
  gpib->setGpibControls(CIDS);
}


/***** Re-load default configuration *****/
void Controller::default_h(char *params) {
  initConfig();
}


/***** Show or set end of receive character(s) *****/
void Controller::eor_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 15, val)) return;
    config.eor = (uint8_t)val;
    if (config.isVerb) {
      cmdstream->print(F("Set EOR to: "));
      cmdstream->println(val);
    };
  } else {
    if (config.eor>7) config.eor = 0;  // Needed to reset FF read from EEPROM after FW upgrade
    cmdstream->println(config.eor);
  }
}


/***** Parallel Poll Handler *****/
/*
 * Device must be set to respond on DIO line 1 - 8
 */
void Controller::ppoll_h(char *params) {
  uint8_t sb = 0;

  // Poll devices
  // Start in controller idle state
  gpib->setGpibControls(CIDS);
  delayMicroseconds(20);
  // Assert ATN and EOI
  setGpibState(0b00000000, 0b10010000, 0);
  //  setGpibState(0b10010000, 0b00000000, 0b10010000);
  delayMicroseconds(20);
  // Read data byte from GPIB bus without handshake
  sb = readGpibDbus();
  // Return to controller idle state (ATN and EOI unasserted)
  gpib->setGpibControls(CIDS);

  // Output the response byte
  cmdstream->println(sb, DEC);

  if (config.isVerb) cmdstream->println(F("Parallel poll completed."));
}


/***** Assert or de-assert REN 0=de-assert; 1=assert *****/
void Controller::ren_h(char *params) {
#if defined (SN7516X) && not defined (SN7516X_DC)
  params = params;
  cmdstream->println(F("Unavailable")) ;
#else
  // char *stat;
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    digitalWrite(REN, (val ? LOW : HIGH));
    if (config.isVerb) {
      cmdstream->print(F("REN: "));
      cmdstream->println(val ? "REN asserted" : "REN un-asserted") ;
    };
  } else {
    cmdstream->println(digitalRead(REN) ? 0 : 1);
  }
#endif
}


/***** Enable verbose mode 0=OFF; 1=ON *****/
void Controller::verb_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    config.isVerb = val ? true : false;
    }
  cmdstream->print("Verbose: ");
  cmdstream->println(config.isVerb ? "ON" : "OFF");
}

/***** Enable prompt mode 0=OFF; 1=ON *****/
void Controller::prompt_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    config.showPrompt = val ? true : false;
    }
  cmdstream->print("Show prompt: ");
  cmdstream->println(config.showPrompt ? "ON" : "OFF");
}


/***** Set version string *****/
/* Replace the standard AR488 version string with something else
 *  NOTE: some instrument software requires a sepcific version string to ID the interface
 */
void Controller::setvstr_h(char *params) {
  char idparams[64];
  memset(idparams, '\0', 64);
  strncpy(idparams, "verstr ", 7);
  strncat(idparams, params, 64-7);

/*
cmdstream->print(F("Plen: "));
cmdstream->println(plen);
cmdstream->print(F("Params: "));
cmdstream->println(params);
cmdstream->print(F("IdParams: "));
cmdstream->println(idparams);
*/

  id_h(idparams);

/*
  if (params != NULL) {
    len = strlen(params);
    if (len>47) len=47; // Ignore anything over 47 characters
    memset(config.vstr, '\0', 48);
    strncpy(config.vstr, params, len);
    if (config.isVerb) {
      cmdstream->print(F("Changed version string to: "));
      cmdstream->println(params);
    };
  }
*/
}


/***** Talk only mode *****/
void Controller::ton_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    isTO = val ? true : false;
    if (isTO) isRO = false; // Read-only mode must be disabled in TO mode!
    if (config.isVerb) {
      cmdstream->print(F("TON: "));
      cmdstream->println(val ? "ON" : "OFF") ;
    }
  } else {
    cmdstream->println(isTO);
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
void Controller::srqa_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    switch (val) {
      case 0:
        isSrqa = false;
        break;
      case 1:
        isSrqa = true;
        break;
    }
    if (config.isVerb) cmdstream->println(isSrqa ? "SRQ auto ON" : "SRQ auto OFF") ;
  } else {
    cmdstream->println(isSrqa);
  }
}


/***** Repeat a given command and return result *****/
void Controller::repeat_h(char *params) {

  uint16_t count;
  uint16_t tmdly;
  char *param;

  if (params != NULL) {
    // Count (number of repetitions)
    param = strtok(params, " \t");
    if (notInRange(param, 2, 255, count)) return;

    // Time delay (milliseconds)
    param = strtok(NULL, " \t");
	if (notInRange(param, 0, 30000, tmdly)) return;

    // Pointer to remainder of parameters string
    param = strtok(NULL, "\n\r");
    if ((param == NULL) || (strlen(param) == 0)) {
      errBadCmd();
      if (config.isVerb)
		cmdstream->println(F("Missing parameter"));
      return;
	} else {
      for (uint16_t i = 0; i < count; i++) {
        // Send string to instrument
		gpib->gpibSendData(param, strlen(param), false);
        delay(tmdly);
        gpib->gpibReceiveData();
      }
	}
  } else {
    errBadCmd();
    if (config.isVerb)
	  cmdstream->println(F("Missing parameters"));
  }
}


/***** Run a macro *****/
void Controller::macro_h(char *params) {
#ifdef USE_MACROS
  uint16_t val;
  uint8_t dlen = 0;
  char * macro; // Pointer to keyword following ++id
  char * keyword; // Pointer to supplied data (remaining characters in buffer)

  if (params != NULL) {
    macro = strtok(params, " \t");
    if (notInRange(macro, 0, 9, val)) return;
    keyword = macro + strlen(macro) + 1;
    dlen = strlen(keyword);
	if (dlen) {
      if (strncmp(keyword, "set", 3)==0) {
		editMacro = (uint8_t)val;
	  }
	  else if (strncmp(keyword, "del", 3)==0) {
		deleteMacro((uint8_t)val);
	  }
	  else
		// invalid sub command
		return;
	} else
	  runMacro = (uint8_t)val;
  } else {
	displayMacros();
  }
#else
  memset(params, '\0', 5);
  cmdstream->println(F("Disabled"));
#endif
}


/***** Bus diagnostics *****/
/*
 * Usage: xdiag mode byte
 * mode: 0=data bus; 1=control bus
 * byte: byte to write on the bus
 * Note: values to switch individual bits = 1,2,4,8,10,20,40,80
 */
void Controller::xdiag_h(char *params){
  char *param;
  uint8_t mode = 0;
  uint8_t val = 0;

  // Get first parameter (mode = 0 or 1)
  param = strtok(params, " \t");
  if (param != NULL) {
    if (strlen(param)<4){
      mode = atoi(param);
      if (mode>2) {
        cmdstream->println(F("Invalid: 0=data bus; 1=control bus"));
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
      if (config.cmode==2) {
        gpib->setGpibControls(CINI);
      }else{
        gpib->setGpibControls(DINI);
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

void Controller::tmbus_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 30000, val)) return;
    config.tmbus = val;
    if (config.isVerb) {
      cmdstream->print(F("TmBus set to: "));
      cmdstream->println(val);
    };
  } else {
    cmdstream->println(config.tmbus, DEC);
  }
}


/***** Set device ID *****/
/*
 * Sets the device ID parameters including:
 * ++id verstr - version string (same as ++setvstr)
 * ++id name   - short name of device (e.g. HP3478A) up to 15 characters
 * ++id serial - serial number up to 9 digits long
 */
void Controller::id_h(char *params) {
  uint8_t dlen = 0;
  char * keyword; // Pointer to keyword following ++id
  char * datastr; // Pointer to supplied data (remaining characters in buffer)
  char serialStr[10];

#ifdef DEBUG10
  cmdstream->print(F("Params: "));
  cmdstream->println(params);
#endif

  if (params != NULL) {
    keyword = strtok(params, " \t");
	if ((keyword == NULL) || strlen(keyword) == 0) {
	  if (config.isVerb)
		cmdstream->println(F("Missing keyword (verstr, name or serial)"));
	  return;
	}

    datastr = keyword + strlen(keyword) + 1;
    dlen = strlen(datastr);
    if (dlen) {
      if (strncmp(keyword, "verstr", 6)==0) {
#ifdef DEBUG10
        cmdstream->print(F("Keyword: "));
        cmdstream->println(keyword);
        cmdstream->print(F("DataStr: "));
        cmdstream->println(datastr);
#endif
        if (dlen>0 && dlen<48) {
#ifdef DEBUG10
        cmdstream->println(F("Length OK"));
#endif
          memset(config.vstr, '\0', 48);
          strncpy(config.vstr, datastr, dlen);
          if (config.isVerb) cmdstream->print(F("VerStr: "));
		  cmdstream->println(config.vstr);
        }else{
          if (config.isVerb)
			  cmdstream->println(F("Length of version string must not exceed 48 characters!"));
          errBadCmd();
        }
        return;
      }
      else if (strncmp(keyword, "name", 4)==0) {
        if (dlen>0 && dlen<16) {
          memset(config.sname, '\0', 16);
          strncpy(config.sname, datastr, dlen);
        }else{
          if (config.isVerb) cmdstream->println(F("Length of name must not exceed 15 characters!"));
          errBadCmd();
        }
        return;
      }
      else if (strncmp(keyword, "serial", 6)==0) {
        if (dlen < 10) {
          config.serial = atol(datastr);
        }else{
          if (config.isVerb) cmdstream->println(F("Serial number must not exceed 9 characters!"));
          errBadCmd();
        }
        return;
      }
	  else {
		if (config.isVerb)
		  cmdstream->println(F("Invalid keyword ([verstr, name, serial])"));
		return;
	  }
    } else {  // display kw value
      if (strncmp(keyword, "verstr", 6)==0) {
        cmdstream->println(config.vstr);
      }
      else if (strncmp(keyword, "name", 4)==0) {
        cmdstream->println(config.sname);
      }
      else if (strncmp(keyword, "serial", 6)==0) {
        memset(serialStr, '\0', 10);
        snprintf(serialStr, 10, "%09lu", config.serial);  // Max str length = 10-1 i.e 9 digits + null terminator
        cmdstream->println(serialStr);
      }
	  else {
		if (config.isVerb)
		  cmdstream->println(F("Invalid keyword ([verstr, name, serial])"));
	  }
    }
  }
  //errBadCmd();
}


void Controller::idn_h(char * params){
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 2, val)) return;
    config.idn = (uint8_t)val;
    if (config.isVerb) {
      cmdstream->print(F("Sending IDN: "));
      cmdstream->print(val ? "Enabled" : "Disabled");
      if (val==2) cmdstream->print(F(" with serial number"));
      cmdstream->println();
    };
  } else {
    cmdstream->println(config.idn, DEC);
  }
}


/***** Check whether a parameter is in range *****/
/* Convert string to integer and check whether value is within
 * lowl to higl inclusive. Also returns converted text in param
 * to a uint16_t integer in rval. Returns true if successful,
 * false if not
*/
bool Controller::notInRange(char *param, uint16_t lowl, uint16_t higl, uint16_t &rval) {

  // Null string passed?
  if (param == NULL) return true;
  // Empty string passed?
  if (strlen(param) == 0) return true;

  // Convert to integer
  rval = 0;
  rval = atoi(param);

  // Check range
  if (rval < lowl || rval > higl) {
    errBadCmd();
    if (config.isVerb) {
      cmdstream->print(F("Valid range is between "));
      cmdstream->print(lowl);
      cmdstream->print(F(" and "));
      cmdstream->println(higl);
    }
    return true;
  }
  return false;
}

/***** Unrecognized command *****/
void Controller::errBadCmd() {
  cmdstream->println(F("Unrecognized command"));
}


#ifdef AR488_WIFI_ENABLE
/***** Configure Wifi connection *****/
/*
 * Sets the wifi parameterw:
 * ++wifi ssid - wifi endpoint
 * ++wifi pass - passphrase
 * ++wifi connect - to (re)start the wifi connection
 */
void Controller::wifi_h(char *params) {
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
          memset(config.ssid, '\0', 32);
          strncpy(config.ssid, datastr, dlen);
        } else {
          if (config.isVerb)
			cmdstream->println(F("Length of ssid string must not exceed 31 characters!"));
          errBadCmd();
        }
      }
      else if (strncmp(keyword, "pass", 4)==0) {
        if (dlen>0 && dlen<64) {
          memset(config.passkey, '\0', 64);
          strncpy(config.passkey, datastr, dlen);
        } else {
          if (config.isVerb)
			cmdstream->println(F("Length of pass must not exceed 63 characters!"));
          errBadCmd();
        }
      }
	  else {
		if (config.isVerb)
		  cmdstream->println(F("Invalid keyword ([ssid, pass, connect, scan])"));
	  }
    } else {
      if (strncmp(keyword, "ssid", 6)==0) {
        cmdstream->println(config.ssid);
      }
      else if (strncmp(keyword, "pass", 4)==0) {
        cmdstream->println(config.passkey);
      }
      else if (strncmp(keyword, "connect", 7)==0) {
        setupWifi();
      }
      else if (strncmp(keyword, "scan", 4)==0) {
        scanWifi();
      }
	  else {
		if (config.isVerb)
		  cmdstream->println(F("Invalid keyword ([ssid, pass, connect, scan])"));
	  }
    }
  }
}
#endif

#ifdef HAS_HELP_COMMAND
static const char cmdHelp[] PROGMEM = {
  "== Prologix compatible command set ==\n"
  "addr: Display/set device address\n"
  "auto: Automatically request talk and read response\n"
  "clr: Send Selected Device Clear to current GPIB address\n"
  "eoi: Enable/disable assertion of EOI signal\n"
  "eor: Show or set end of receive character(s)\n"
  "eos: Specify GPIB termination character\n"
  "eot_char: Set character to append to USB output when EOT enabled\n"
  "eot_enable: Enable/Disable appending user specified character to USB output on EOI detection\n"
  "help: This message\n"
  "ifc: Assert IFC signal for 150 miscoseconds - make AR488 controller in charge\n"
  "llo: Local lockout - disable front panel operation on instrument\n"
  "loc: Enable front panel operation on instrument\n"
  "lon: Put controller in listen-only mode (listen to all traffic)\n"
  "mode: Set the interface mode (0=controller/1=device)\n"
  "read: Read data from instrument\n"
  "read_tmo_ms: Read timeout specified between 1 - 3000 milliseconds\n"
  "rst: Reset the controller\n"
  "savecfg: Save configration\n"
  "spoll: Serial poll the addressed host or all instruments\n"
  "srq: Return status of srq signal (1-srq asserted/0-srq not asserted)\n"
  "status: Set the status byte to be returned on being polled (bit 6 = RQS, i.e SRQ asserted)\n"
  "trg: Send trigger to selected devices (up to 15 addresses)\n"
  "ver: Display firmware version\n"
  // additional commands
  "== Extension command set ==\n"
  "allspoll: Serial poll all instruments (alias: ++spoll all)\n"
  "findrqs: Find device requesting service\n"
  "findlstn: Find all devices listening on the GPIB bus\n"
  "dcl: Send unaddressed (all) device clear  [power on reset] (is the rst?)\n"
  "default: Set configuration to controller default settings\n"
  "id name: Show/Set the name of the interface\n"
  "id serial: Show/Set the serial number of the interface\n"
  "id verstr: Show/Set the version string (replaces setvstr)\n"
  "idn: Enable/Disable reply to *idn? (disabled by default)\n"
#ifdef USE_MACROS
  "macro: List defined macros\n"
  "macro <n>: Execute macro number <n>\n"
  "macro <n> set: Edit macro number <n>\n"
  "macro <n> del: Delete macro number <n>\n"
#endif
  "ppoll: Conduct a parallel poll\n"
  "ren: Assert or Unassert the REN signal\n"
  "repeat: Repeat a given command and return result\n"
  "setvstr: Set custom version string (to identify controller, e.g. \"GPIB-USB\"). Max 47 chars, excess truncated.\n"
  "srqauto: Automatically conduct serial poll when SRQ is asserted\n"
  "ton: Put controller in talk-only mode (send data only)\n"
  "tmbus: Timing parameters (see the doc)\n"
  "verbose: Verbose (human readable) mode\n"
#ifdef AR488_WIFI_ENABLE
  "wifi ssid: Set or get the wifi SSID (31 chars max)\n"
  "wifi passkey: Set or get the wifi passphrase (63 chars max)\n"
  "wifi connect: Connect to the configure wifi AP\n"
  "wifi scan: Scan for accessible AP\n"
#endif
  "xdiag: Bus diagnostics (see the doc)\n"
};
#endif

/***** Show help message *****/
void Controller::help_h(char *params) {
#ifdef HAS_HELP_COMMAND
  char c;
  char token[20];
  int i;

  i = 0;
  for (unsigned int k = 0; k < strlen_P(cmdHelp); k++) {
    c = pgm_read_byte_near(cmdHelp + k);
	if ((params == NULL) && (i==0) && (c == '='))
	  // a "title" line, print it (if no params)
	  i = 255;

    if (i < 20) {
      if(c == ':') {
        token[i] = 0;
        if((params == NULL) || (strcmp(token, params) == 0)) {
          cmdstream->print(F("++"));
          cmdstream->print(token);
          cmdstream->print(c);
          cmdstream->print('\t');
          i = 255; // means we need to print until \n
        }
      } else {
        token[i] = c;
        i++;
      }
    }
    else if (i == 255) {
      cmdstream->print(c);
    }
    if (c == '\n') {
      i = 0;
    }
  }
#else
  cmdstream->println(F("help command not supported on this device"));
#endif
}
