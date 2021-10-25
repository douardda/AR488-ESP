#include <Arduino.h>
#include "AR488_Layouts.h"
#include "commands.h"
#include "gpib.h"
#include "controller.h"
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
  { "xdiag",       3, xdiag_h     }

};

/***** Execute a command *****/
void execCmd(char *buffr, uint8_t dsize, GPIB &gpib) {
  char line[PBSIZE];
  // Copy collected chars to line buffer
  memcpy(line, buffr, dsize);

  // Flush the parse buffer
  gpib.controller.flushPbuf();

#ifdef DEBUG1
  dbSerial->print(F("execCmd: Command received: ")); printHex(line, dsize);
#endif

  // Its a ++command so shift everything two bytes left (ignore ++) and parse
  for (int i = 0; i < dsize-2; i++) {
    line[i] = line[i + 2];
  }
  // Replace last two bytes with a null (\0) character
  line[dsize - 2] = '\0';
  line[dsize - 1] = '\0';
#ifdef DEBUG1
  dbSerial->print(F("execCmd: Sent to the command processor: ")); printHex(line, dsize-2);
#endif
  // Execute the command
  getCmd(line, gpib);

  // Show a prompt on completion?
  if (gpib.controller.config.isVerb) gpib.controller.showPrompt();
}


/***** Extract command and pass to handler *****/
void getCmd(char *buffr, GPIB& gpib) {

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
    if (cmdHidx[i].opmode & gpib.controller.config.cmode) {
      // If its a command with parameters
      // Copy command parameters to params and call handler with parameters
      params = token + strlen(token) + 1;

      // If command parameters were specified
      if (strlen(params) > 0) {
#ifdef DEBUG1
        dbSerial->print(F("Calling handler with parameters: ")); dbSerial->println(params);
#endif
        // Call handler with parameters specified
        cmdHidx[i].handler(params, gpib);
      }else{
        // Call handler without parameters
		cmdHidx[i].handler(NULL, gpib);
      }
    }else{
      errBadCmd(gpib);
      if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Command not available in this mode."));
    }

  } else {
    // No valid command found
    errBadCmd(gpib);
  }

}

/*************************************/
/***** STANDARD COMMAND HANDLERS *****/
/*************************************/

/***** Show or change device address *****/
void addr_h(char *params, GPIB& gpib) {
  //  char *param, *stat;
  char *param;
  uint16_t val;
  if (params != NULL) {

    // Primary address
    param = strtok(params, " \t");
    if (notInRange(param, 1, 30, val, gpib)) return;
    if (val == gpib.controller.config.caddr) {
      errBadCmd(gpib);
      if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("That is my address! Address of a remote device is required."));
      return;
    }
    gpib.controller.config.paddr = val;
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("Set device primary address to: "));
      gpib.controller.stream.println(val);
    }

    // Secondary address
    gpib.controller.config.saddr = 0;
    val = 0;
    param = strtok(NULL, " \t");
    if (param != NULL) {
      if (notInRange(param, 96, 126, val, gpib)) return;
      gpib.controller.config.saddr = val;
      if (gpib.controller.config.isVerb) {
        gpib.controller.stream.print("Set device secondary address to: ");
        gpib.controller.stream.println(val);
      }
    }

  } else {
    gpib.controller.stream.print(gpib.controller.config.paddr);
    if (gpib.controller.config.saddr > 0) {
      gpib.controller.stream.print(F(" "));
      gpib.controller.stream.print(gpib.controller.config.saddr);
    }
    gpib.controller.stream.println();
  }
}


/***** Show or set read timout *****/
void rtmo_h(char *params, GPIB& gpib) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 1, 32000, val, gpib)) return;
    gpib.controller.config.rtmo = val;
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("Set [read_tmo_ms] to: "));
      gpib.controller.stream.print(val);
      gpib.controller.stream.println(F(" milliseconds"));
    }
  } else {
    gpib.controller.stream.println(gpib.controller.config.rtmo);
  }
}


/***** Show or set end of send character *****/
void eos_h(char *params, GPIB& gpib) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 3, val, gpib)) return;
    gpib.controller.config.eos = (uint8_t)val;
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("Set EOS to: "));
      gpib.controller.stream.println(val);
    };
  } else {
    gpib.controller.stream.println(gpib.controller.config.eos);
  }
}


/***** Show or set EOI assertion on/off *****/
void eoi_h(char *params, GPIB& gpib) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val, gpib)) return;
    gpib.controller.config.eoi = val ? true : false;
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("Set EOI assertion: "));
      gpib.controller.stream.println(val ? "ON" : "OFF");
    };
  } else {
    gpib.controller.stream.println(gpib.controller.config.eoi);
  }
}


/***** Show or set interface to controller/device mode *****/
void cmode_h(char *params, GPIB& gpib) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val, gpib)) return;
    switch (val) {
      case 0:
        gpib.controller.config.cmode = 1;
        gpib.initDevice();
        break;
      case 1:
        gpib.controller.config.cmode = 2;
        gpib.initController();
        break;
    }
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("Interface mode set to: "));
      gpib.controller.stream.println(val ? "CONTROLLER" : "DEVICE");
    }
  } else {
    gpib.controller.stream.println(gpib.controller.config.cmode - 1);
  }
}


/***** Show or enable/disable sending of end of transmission character *****/
void eot_en_h(char *params, GPIB& gpib) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val, gpib)) return;
    gpib.controller.config.eot_en = val ? true : false;
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("Appending of EOT character: "));
      gpib.controller.stream.println(val ? "ON" : "OFF");
    }
  } else {
    gpib.controller.stream.println(gpib.controller.config.eot_en);
  }
}


/***** Show or set end of transmission character *****/
void eot_char_h(char *params, GPIB& gpib) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 255, val, gpib)) return;
    gpib.controller.config.eot_ch = (uint8_t)val;
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("EOT set to ASCII character: "));
      gpib.controller.stream.println(val);
    };
  } else {
    gpib.controller.stream.println(gpib.controller.config.eot_ch, DEC);
  }
}


/***** Show or enable/disable auto mode *****/
void amode_h(char *params, GPIB& gpib) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 3, val, gpib)) return;
    if (val > 0 && gpib.controller.config.isVerb) {
      gpib.controller.stream.println(F("WARNING: automode ON can cause some devices to generate"));
      gpib.controller.stream.println(F("         'addressed to talk but nothing to say' errors"));
    }
    gpib.controller.config.amode = (uint8_t)val;
    if (gpib.controller.config.amode < 3) gpib.controller.aRead = false;
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("Auto mode: "));
      gpib.controller.stream.println(gpib.controller.config.amode);
    }
  } else {
    gpib.controller.stream.println(gpib.controller.config.amode);
  }
}


/***** Display the controller version string *****/
void ver_h(char *params, GPIB& gpib) {
  // If "real" requested
  if (params != NULL && strncmp(params, "real", 3) == 0) {
    gpib.controller.stream.println(F(FWVER));
    // Otherwise depends on whether we have a custom string set
  } else {
    if (strlen(gpib.controller.config.vstr) > 0) {
      gpib.controller.stream.println(gpib.controller.config.vstr);
    } else {
      gpib.controller.stream.println(F(FWVER));
    }
  }
}


/***** Address device to talk and read the sent data *****/
void read_h(char *params, GPIB& gpib) {
  // Clear read flags
  gpib.rEoi = false;
  gpib.rEbt = false;
  // Read any parameters
  if (params != NULL) {
    if (strlen(params) > 3) {
      if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Invalid termination character - ignored!"));
    } else if (strncmp(params, "eoi", 3) == 0) { // Read with eoi detection
      gpib.rEoi = true;
    } else { // Assume ASCII character given and convert to an 8 bit byte
      gpib.rEbt = true;
      gpib.eByte = atoi(params);
    }
  }
  if (gpib.controller.config.amode == 3) {
    // In auto continuous mode we set this flag to indicate we are ready for continuous read
    gpib.controller.aRead = true;
  } else {
    // If auto mode is disabled we do a single read
    gpib.gpibReceiveData();
  }
}


/***** Send device clear (usually resets the device to power on state) *****/
void clr_h(char *params, GPIB& gpib) {
  if (gpib.addrDev(gpib.controller.config.paddr, 0)) {
    if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Failed to address device"));
    return;
  }
  if (gpib.gpibSendCmd(GC_SDC))  {
    if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Failed to send SDC"));
    return;
  }
  if (gpib.uaddrDev()) {
    if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Failed to untalk GPIB bus"));
    return;
  }
  // Set GPIB controls back to idle state
  gpib.setGpibControls(CIDS);
}


/***** Send local lockout command *****/
void llo_h(char *params, GPIB& gpib) {
  // NOTE: REN *MUST* be asserted (LOW)
  if (digitalRead(REN)==LOW) {
    // For 'all' send LLO to the bus without addressing any device - devices will show REM
    if (params != NULL) {
      if (0 == strncmp(params, "all", 3)) {
        if (gpib.gpibSendCmd(GC_LLO)) {
          if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Failed to send universal LLO."));
        }
      }
    } else {
      // Address current device
      if (gpib.addrDev(gpib.controller.config.paddr, 0)) {
        if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Failed to address the device."));
        return;
      }
      // Send LLO to currently addressed device
      if (gpib.gpibSendCmd(GC_LLO)) {
        if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Failed to send LLO to device"));
        return;
      }
      // Unlisten bus
      if (gpib.uaddrDev()) {
        if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Failed to unlisten the GPIB bus"));
        return;
      }
    }
  }
  // Set GPIB controls back to idle state
  gpib.setGpibControls(CIDS);
}


/***** Send Go To Local (GTL) command *****/
void loc_h(char *params, GPIB& gpib) {
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
      if (gpib.addrDev(gpib.controller.config.paddr, 0)) {
        if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Failed to address device."));
        return;
      }
      // Send GTL
      if (gpib.gpibSendCmd(GC_GTL)) {
        if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Failed sending LOC."));
        return;
      }
      // Unlisten bus
      if (gpib.uaddrDev()) {
        if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Failed to unlisten GPIB bus."));
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
void ifc_h(char *params, GPIB& gpib) {
  gpib.assertIfc();
}


/***** Send a trigger command *****/
void trg_h(char *params, GPIB& gpib) {
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
    addrs[0] = gpib.controller.config.paddr;
    cnt++;
  } else {
    // Read address parameters into array
    while (cnt < 15) {
      if (cnt == 0) {
        param = strtok(params, " \t");
      } else {
        param = strtok(NULL, " \t");
      }
      if (notInRange(param, 1, 30, val, gpib)) return;
      addrs[cnt] = (uint8_t)val;
      cnt++;
    }
  }

  // If we have some addresses to trigger....
  if (cnt > 0) {
    for (int i = 0; i < cnt; i++) {
      // Address the device
      if (gpib.addrDev(addrs[i], 0)) {
        if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Failed to address device"));
        return;
      }
      // Send GTL
      if (gpib.gpibSendCmd(GC_GET))  {
        if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Failed to trigger device"));
        return;
      }
      // Unaddress device
      if (gpib.uaddrDev()) {
        if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Failed to unlisten GPIB bus"));
        return;
      }
    }

    // Set GPIB controls back to idle state
    gpib.setGpibControls(CIDS);

    if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Group trigger completed."));
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
void rst_h(char *params, GPIB& gpib) {
  gpib.controller.reset();
}


/***** Serial Poll Handler *****/
void spoll_h(char *params, GPIB& gpib) {
  char *param;
  uint8_t addrs[15];
  uint8_t sb = 0;
  uint8_t r;
  //  uint8_t i = 0;
  uint8_t j = 0;
  uint16_t val = 0;
  bool all = false;
  bool eoiDetected = false;

  // Initialise address array
  for (int i = 0; i < 15; i++) {
    addrs[i] = 0;
  }

  // Read parameters
  if (params == NULL) {
    // No parameters - trigger addressed device only
    addrs[0] = gpib.controller.config.paddr;
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
        if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Serial poll of all devices requested..."));
        break;
        // Read all address parameters
      } else if (strlen(params) < 3) { // No more than 2 characters
        if (notInRange(param, 1, 30, val, gpib)) return;
        addrs[j] = (uint8_t)val;
        j++;
      } else {
        errBadCmd(gpib);
        if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Invalid parameter"));
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
  if ( gpib.gpibSendCmd(GC_LAD + gpib.controller.config.caddr) )  {
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
    if (val != gpib.controller.config.caddr) {

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
            gpib.controller.stream.print(F("SRQ:")); gpib.controller.stream.print(i); gpib.controller.stream.print(F(",")); gpib.controller.stream.println(sb, DEC);
            i = j;
          }
        } else {
          // Return decimal number representing status byte
          gpib.controller.stream.println(sb, DEC);
          if (gpib.controller.config.isVerb) {
            gpib.controller.stream.print(F("Received status byte ["));
            gpib.controller.stream.print(sb);
            gpib.controller.stream.print(F("] from device at address: "));
            gpib.controller.stream.println(val);
          }
          i = j;
        }
      } else {
        if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Failed to retrieve status byte"));
      }
    }
  }
  if (all) gpib.controller.stream.println();

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
  if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Serial poll completed."));

}


/***** Return status of SRQ line *****/
void srq_h(char *params, GPIB& gpib) {
  //NOTE: LOW=asserted, HIGH=unasserted
  gpib.controller.stream.println(!digitalRead(SRQ));
}


/***** Set the status byte (device mode) *****/
void stat_h(char *params, GPIB& gpib) {
  uint16_t val = 0;
  // A parameter given?
  if (params != NULL) {
    // Byte value given?
    if (notInRange(params, 0, 255, val, gpib)) return;
    gpib.controller.config.stat = (uint8_t)val;
    if (val & 0x40) {
      setSrqSig();
      if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("SRQ asserted."));
    } else {
      clrSrqSig();
      if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("SRQ un-asserted."));
    }
  } else {
    // Return the currently set status byte
    gpib.controller.stream.println(gpib.controller.config.stat);
  }
}


/***** Save controller configuration *****/
void save_h(char *params, GPIB& gpib) {
#ifdef E2END
  uint8_t *conf = (uint8_t*) &(gpib.controller.config);
  epWriteData(conf, AR_CFG_SIZE);
  if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Settings saved."));
#else
  gpib.controller.stream.println(F("EEPROM not supported."));
#endif
}


/***** Show state or enable/disable listen only mode *****/
void lon_h(char *params, GPIB& gpib) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val, gpib)) return;
    gpib.controller.isRO = val ? true : false;
    if (gpib.controller.isTO) gpib.controller.isTO = false; // Talk-only mode must be disabled!
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("LON: "));
      gpib.controller.stream.println(val ? "ON" : "OFF") ;
    }
  } else {
    gpib.controller.stream.println(gpib.controller.isRO);
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
void aspoll_h(char *params, GPIB& gpib) {
  spoll_h((char*)"all", gpib);
}


/***** Send Universal Device Clear *****/
/*
 * The universal Device Clear (DCL) is unaddressed and affects all devices on the Gpib bus.
 */
void dcl_h(char *params, GPIB& gpib) {
  if ( gpib.gpibSendCmd(GC_DCL) )  {
    if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Sending DCL failed"));
    return;
  }
  // Set GPIB controls back to idle state
  gpib.setGpibControls(CIDS);
}


/***** Re-load default configuration *****/
void default_h(char *params, GPIB& gpib) {
  gpib.controller.initConfig();
}


/***** Show or set end of receive character(s) *****/
void eor_h(char *params, GPIB& gpib) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 15, val, gpib)) return;
    gpib.controller.config.eor = (uint8_t)val;
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("Set EOR to: "));
      gpib.controller.stream.println(val);
    };
  } else {
    if (gpib.controller.config.eor>7) gpib.controller.config.eor = 0;  // Needed to reset FF read from EEPROM after FW upgrade
    gpib.controller.stream.println(gpib.controller.config.eor);
  }
}


/***** Parallel Poll Handler *****/
/*
 * Device must be set to respond on DIO line 1 - 8
 */
void ppoll_h(char *params, GPIB& gpib) {
  uint8_t sb = 0;

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
  gpib.controller.stream.println(sb, DEC);

  if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Parallel poll completed."));
}


/***** Assert or de-assert REN 0=de-assert; 1=assert *****/
void ren_h(char *params, GPIB& gpib) {
#if defined (SN7516X) && not defined (SN7516X_DC)
  params = params;
  gpib.controller.stream.println(F("Unavailable")) ;
#else
  // char *stat;
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val, gpib)) return;
    digitalWrite(REN, (val ? LOW : HIGH));
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("REN: "));
      gpib.controller.stream.println(val ? "REN asserted" : "REN un-asserted") ;
    };
  } else {
    gpib.controller.stream.println(digitalRead(REN) ? 0 : 1);
  }
#endif
}


/***** Enable verbose mode 0=OFF; 1=ON *****/
void verb_h(char *params, GPIB& gpib) {
  gpib.controller.config.isVerb = !gpib.controller.config.isVerb;
  gpib.controller.stream.print("Verbose: ");
  gpib.controller.stream.println(gpib.controller.config.isVerb ? "ON" : "OFF");
}


/***** Set version string *****/
/* Replace the standard AR488 version string with something else
 *  NOTE: some instrument software requires a sepcific version string to ID the interface
 */
void setvstr_h(char *params, GPIB& gpib) {
  uint8_t plen;
  char idparams[64];
  plen = strlen(params);
  memset(idparams, '\0', 64);
  strncpy(idparams, "verstr ", 7);
  strncat(idparams, params, plen);

/*
gpib.controller.stream.print(F("Plen: "));
gpib.controller.stream.println(plen);
gpib.controller.stream.print(F("Params: "));
gpib.controller.stream.println(params);
gpib.controller.stream.print(F("IdParams: "));
gpib.controller.stream.println(idparams);
*/

  id_h(idparams, gpib);

/*
  if (params != NULL) {
    len = strlen(params);
    if (len>47) len=47; // Ignore anything over 47 characters
    memset(gpib.controller.config.vstr, '\0', 48);
    strncpy(gpib.controller.config.vstr, params, len);
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("Changed version string to: "));
      gpib.controller.stream.println(params);
    };
  }
*/
}


/***** Talk only mode *****/
void ton_h(char *params, GPIB& gpib) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val, gpib)) return;
    gpib.controller.isTO = val ? true : false;
    if (gpib.controller.isTO) gpib.controller.isRO = false; // Read-only mode must be disabled in TO mode!
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("TON: "));
      gpib.controller.stream.println(val ? "ON" : "OFF") ;
    }
  } else {
    gpib.controller.stream.println(gpib.controller.isTO);
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
void srqa_h(char *params, GPIB& gpib) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val, gpib)) return;
    switch (val) {
      case 0:
        gpib.controller.isSrqa = false;
        break;
      case 1:
        gpib.controller.isSrqa = true;
        break;
    }
    if (gpib.controller.config.isVerb) gpib.controller.stream.println(gpib.controller.isSrqa ? "SRQ auto ON" : "SRQ auto OFF") ;
  } else {
    gpib.controller.stream.println(gpib.controller.isSrqa);
  }
}


/***** Repeat a given command and return result *****/
void repeat_h(char *params, GPIB& gpib) {

  uint16_t count;
  uint16_t tmdly;
  char *param;

  if (params != NULL) {
    // Count (number of repetitions)
    param = strtok(params, " \t");
    if (strlen(param) > 0) {
      if (notInRange(param, 2, 255, count, gpib)) return;
    }
    // Time delay (milliseconds)
    param = strtok(NULL, " \t");
    if (strlen(param) > 0) {
      if (notInRange(param, 0, 30000, tmdly, gpib)) return;
    }

    // Pointer to remainder of parameters string
    param = strtok(NULL, "\n\r");
    if (strlen(param) > 0) {
      for (uint16_t i = 0; i < count; i++) {
        // Send string to instrument
        gpib.gpibSendData(param, strlen(param));
        delay(tmdly);
        gpib.gpibReceiveData();
      }
    } else {
      errBadCmd(gpib);
      if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Missing parameter"));
      return;
    }
  } else {
    errBadCmd(gpib);
    if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Missing parameters"));
  }

}


/***** Run a macro *****/
void macro_h(char *params, GPIB& gpib) {
#ifdef USE_MACROS
  uint16_t val;
  const char * macro;

  if (params != NULL) {
    if (notInRange(params, 0, 9, val, gpib)) return;
    //    execMacro((uint8_t)val);
    gpib.controller.runMacro = (uint8_t)val;
  } else {
    for (int i = 0; i < 10; i++) {
	  macro = (char*)pgm_read_word(macros + i);
      //      gpib.controller.stream.print(i);gpib.controller.stream.print(F(": "));
      if (strlen_P(macro) > 0) {
        gpib.controller.stream.print(i);
        gpib.controller.stream.print(" ");
      }
    }
    gpib.controller.stream.println();
  }
#else
  memset(params, '\0', 5);
  gpib.controller.stream.println(F("Disabled"));
#endif
}


/***** Bus diagnostics *****/
/*
 * Usage: xdiag mode byte
 * mode: 0=data bus; 1=control bus
 * byte: byte to write on the bus
 * Note: values to switch individual bits = 1,2,4,8,10,20,40,80
 */
void xdiag_h(char *params, GPIB& gpib){
  char *param;
  uint8_t mode = 0;
  uint8_t val = 0;

  // Get first parameter (mode = 0 or 1)
  param = strtok(params, " \t");
  if (param != NULL) {
    if (strlen(param)<4){
      mode = atoi(param);
      if (mode>2) {
        gpib.controller.stream.println(F("Invalid: 0=data bus; 1=control bus"));
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
      if (gpib.controller.config.cmode==2) {
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

void tmbus_h(char *params, GPIB& gpib) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 30000, val, gpib)) return;
    gpib.controller.config.tmbus = val;
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("TmBus set to: "));
      gpib.controller.stream.println(val);
    };
  } else {
    gpib.controller.stream.println(gpib.controller.config.tmbus, DEC);
  }
}


/***** Set device ID *****/
/*
 * Sets the device ID parameters including:
 * ++id verstr - version string (same as ++setvstr)
 * ++id name   - short name of device (e.g. HP3478A) up to 15 characters
 * ++id serial - serial number up to 9 digits long
 */
void id_h(char *params, GPIB& gpib) {
  uint8_t dlen = 0;
  char * keyword; // Pointer to keyword following ++id
  char * datastr; // Pointer to supplied data (remaining characters in buffer)
  char serialStr[10];

#ifdef DEBUG10
  gpib.controller.stream.print(F("Params: "));
  gpib.controller.stream.println(params);
#endif

  if (params != NULL) {
    keyword = strtok(params, " \t");
    datastr = keyword + strlen(keyword) + 1;
    dlen = strlen(datastr);
    if (dlen) {
      if (strncmp(keyword, "verstr", 6)==0) {
#ifdef DEBUG10
        gpib.controller.stream.print(F("Keyword: "));
        gpib.controller.stream.println(keyword);
        gpib.controller.stream.print(F("DataStr: "));
        gpib.controller.stream.println(datastr);
#endif
        if (dlen>0 && dlen<48) {
#ifdef DEBUG10
        gpib.controller.stream.println(F("Length OK"));
#endif
          memset(gpib.controller.config.vstr, '\0', 48);
          strncpy(gpib.controller.config.vstr, datastr, dlen);
          if (gpib.controller.config.isVerb) gpib.controller.stream.print(F("VerStr: "));
		  gpib.controller.stream.println(gpib.controller.config.vstr);
        }else{
          if (gpib.controller.config.isVerb)
			  gpib.controller.stream.println(F("Length of version string must not exceed 48 characters!"));
          errBadCmd(gpib);
        }
        return;
      }
      if (strncmp(keyword, "name", 4)==0) {
        if (dlen>0 && dlen<16) {
          memset(gpib.controller.config.sname, '\0', 16);
          strncpy(gpib.controller.config.sname, datastr, dlen);
        }else{
          if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Length of name must not exceed 15 characters!"));
          errBadCmd(gpib);
        }
        return;
      }
      if (strncmp(keyword, "serial", 6)==0) {
        if (dlen < 10) {
          gpib.controller.config.serial = atol(datastr);
        }else{
          if (gpib.controller.config.isVerb) gpib.controller.stream.println(F("Serial number must not exceed 9 characters!"));
          errBadCmd(gpib);
        }
        return;
      }
//      errBadCmd(gpib);
    }else{
      if (strncmp(keyword, "verstr", 6)==0) {
        gpib.controller.stream.println(gpib.controller.config.vstr);
        return;
      }
      if (strncmp(keyword, "name", 4)==0) {
        gpib.controller.stream.println(gpib.controller.config.sname);
        return;
      }
      if (strncmp(keyword, "serial", 6)==0) {
        memset(serialStr, '\0', 10);
        snprintf(serialStr, 10, "%09lu", gpib.controller.config.serial);  // Max str length = 10-1 i.e 9 digits + null terminator
        gpib.controller.stream.println(serialStr);
        return;
      }
    }
  }
  errBadCmd(gpib);
}


void idn_h(char * params, GPIB& gpib){
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 2, val, gpib)) return;
    gpib.controller.config.idn = (uint8_t)val;
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("Sending IDN: "));
      gpib.controller.stream.print(val ? "Enabled" : "Disabled");
      if (val==2) gpib.controller.stream.print(F(" with serial number"));
      gpib.controller.stream.println();
    };
  } else {
    gpib.controller.stream.println(gpib.controller.config.idn, DEC);
  }
}


/***** Check whether a parameter is in range *****/
/* Convert string to integer and check whether value is within
 * lowl to higl inclusive. Also returns converted text in param
 * to a uint16_t integer in rval. Returns true if successful,
 * false if not
*/
bool notInRange(char *param, uint16_t lowl, uint16_t higl, uint16_t &rval, GPIB& gpib) {

  // Null string passed?
  if (strlen(param) == 0) return true;

  // Convert to integer
  rval = 0;
  rval = atoi(param);

  // Check range
  if (rval < lowl || rval > higl) {
    errBadCmd(gpib);
    if (gpib.controller.config.isVerb) {
      gpib.controller.stream.print(F("Valid range is between "));
      gpib.controller.stream.print(lowl);
      gpib.controller.stream.print(F(" and "));
      gpib.controller.stream.println(higl);
    }
    return true;
  }
  return false;
}

/***** Unrecognized command *****/
void errBadCmd(GPIB& gpib) {
  gpib.controller.stream.println(F("Unrecognized command"));
}
