
#include "gpib.h"
#include "AR488_Layouts.h"
#include "commands.h"


GPIB::GPIB(Stream& stream, AR488Conf& conf, Controller& controller):
		outstream(stream),
		AR488(conf),
		controller(controller),
		verbose(false)
{
}

/***** Initialise device mode *****/
void GPIB::initDevice() {
  // Set GPIB control bus to device idle mode
  setGpibControls(DINI);

  // Initialise GPIB data lines (sets to INPUT_PULLUP)
  readGpibDbus();
}


/***** Initialise controller mode *****/
void GPIB::initController() {
  // Set GPIB control bus to controller idle mode
  setGpibControls(CINI);  // Controller initialise state
  // Initialise GPIB data lines (sets to INPUT_PULLUP)
  readGpibDbus();
  // Assert IFC to signal controller in charge (CIC)
  assertIfc();
}

/***************************************/
/***** GPIB DATA HANDLING ROUTINES *****/
/***************************************/

/*****  Send a single byte GPIB command *****/
bool GPIB::gpibSendCmd(uint8_t cmdByte) {

  bool stat = false;

  // Set lines for command and assert ATN
  setGpibControls(CCMS);

  // Send the command
  stat = gpibWriteByte(cmdByte);
  if (stat && AR488.isVerb) {
    outstream.print(F("gpibSendCmd: failed to send command "));
    outstream.print(cmdByte, HEX);
    outstream.println(F(" to device"));
  }

  // Return to controller idle state
  //  setGpibControls(CIDS);
  // NOTE: this breaks serial poll

  return stat ? ERR : OK;
}


/***** Send the status byte *****/
void GPIB::gpibSendStatus() {
  // Have been addressed and polled so send the status byte
  if (AR488.isVerb) {
    outstream.print(F("Sending status byte: "));
    outstream.println(AR488.stat);
  };
  setGpibControls(DTAS);
  gpibWriteByte(AR488.stat);
  setGpibControls(DIDS);
}


/***** Send a series of characters as data to the GPIB bus *****/
void GPIB::gpibSendData(char *data, uint8_t dsize) {

  bool err = false;

  // If lon is turned on we cannot send data so exit
  if (controller.isRO) return;

  // Controler can unlisten bus and address devices
  if (AR488.cmode == 2) {

    if (deviceAddressing) {
      // Address device to listen
      if (addrDev(AR488.paddr, 0)) {
        if (AR488.isVerb) {
          outstream.print(F("gpibSendData: failed to address device "));
          outstream.print(AR488.paddr);
          outstream.println(F(" to listen"));
        }
        return;
      }
    }

    deviceAddressing = controller.dataBufferFull ? false : true;

#ifdef DEBUG3
    dbSerial->println(F("Device addressed."));
#endif

    // Set control lines to write data (ATN unasserted)
    setGpibControls(CTAS);

  } else {
    setGpibControls(DTAS);
  }
#ifdef DEBUG3
  dbSerial->println(F("Set write data mode."));
  dbSerial->print(F("Send->"));
#endif

  // Write the data string
  for (int i = 0; i < dsize; i++) {
    // If EOI asserting is on
    if (AR488.eoi) {
      // Send all characters
      err = gpibWriteByte(data[i]);
    } else {
      // Otherwise ignore non-escaped CR, LF and ESC
      if ((data[i] != CR) || (data[i] != LF) || (data[i] != ESC)) err = gpibWriteByte(data[i]);
    }
#ifdef DEBUG3
    dbSerial->print(data[i]);
#endif
    if (err) break;
  }

#ifdef DEBUG3
  dbSerial->println("<-End.");
#endif

  if (!err) {
    // Write terminators according to EOS setting
    // Do we need to write a CR?
    if ((AR488.eos & 0x2) == 0) {
      gpibWriteByte(CR);
#ifdef DEBUG3
      dbSerial->println(F("Appended CR"));
#endif
    }
    // Do we need to write an LF?
    if ((AR488.eos & 0x1) == 0) {
      gpibWriteByte(LF);
#ifdef DEBUG3
      dbSerial->println(F("Appended LF"));
#endif
    }
  }

  // If EOI enabled and no more data to follow then assert EOI
  if (AR488.eoi && !controller.dataBufferFull) {
    setGpibState(0b00000000, 0b00010000, 0);
    //    setGpibState(0b00010000, 0b00000000, 0b00010000);
    delayMicroseconds(40);
    setGpibState(0b00010000, 0b00010000, 0);
    //    setGpibState(0b00010000, 0b00010000, 0b00010000);
#ifdef DEBUG3
    dbSerial->println(F("Asserted EOI"));
#endif
  }

  if (AR488.cmode == 2) {   // Controller mode
    if (!err) {
      if (deviceAddressing) {
        // Untalk controller and unlisten bus
        if (uaddrDev()) {
          if (AR488.isVerb) outstream.println(F("gpibSendData: Failed to unlisten bus"));
        }

#ifdef DEBUG3
        dbSerial->println(F("Unlisten done"));
#endif
      }
    }

    // Controller - set lines to idle?
    setGpibControls(CIDS);

  }else{    // Device mode
    // Set control lines to idle
    setGpibControls(DIDS);
  }

#ifdef DEBUG3
    dbSerial->println(F("<- End of send."));
#endif

}


/***** Receive data from the GPIB bus ****/
/*
 * Readbreak:
 * 5 - EOI detected
 * 7 - command received via serial
 */
bool GPIB::gpibReceiveData() {

  uint8_t r = 0; //, db;
  uint8_t bytes[3] = {0};
  uint8_t eor = AR488.eor&7;
  int x = 0;
  bool eoiStatus;
  bool eoiDetected = false;

  // Reset transmission break flag
  tranBrk = 0;

  // Set status of EOI detection
  eoiStatus = rEoi; // Save status of rEoi flag
  if (AR488.eor==7) rEoi = true;    // Using EOI as terminator

  // Set up for reading in Controller mode
  if (AR488.cmode == 2) {   // Controler mode
    // Address device to talk
    if (addrDev(AR488.paddr, 1)) {
      if (AR488.isVerb) {
        outstream.print(F("Failed to address the device"));
        outstream.print(AR488.paddr);
        outstream.println(F(" to talk"));
      }
    }
    // Wait for instrument ready
    Wait_on_pin_state(HIGH, NRFD, AR488.rtmo);
    // Set GPIB control lines to controller read mode
    setGpibControls(CLAS);

  // Set up for reading in Device mode
  } else {  // Device mode
    // Set GPIB controls to device read mode
    setGpibControls(DLAS);
    rEoi = true;  // In device mode we read with EOI by default
  }

#ifdef DEBUG7
    dbSerial->println(F("gpibReceiveData: Start listen ->"));
    dbSerial->println(F("Before loop flags:"));
    dbSerial->print(F("TRNb: "));
    dbSerial->println(tranBrk);
    dbSerial->print(F("rEOI: "));
    dbSerial->println(rEoi);
    dbSerial->print(F("ATN:  "));
    dbSerial->println(isAtnAsserted() ? 1 : 0);
#endif

  // Ready the data bus
  readyGpibDbus();

  // Perform read of data (r=0: data read OK; r>0: GPIB read error);
  while (r == 0) {

    // Tranbreak > 0 indicates break condition
    if (tranBrk > 0) break;

    // ATN asserted
    if (isAtnAsserted()) break;

    // Read the next character on the GPIB bus
    r = gpibReadByte(&bytes[0], &eoiDetected);

    // When reading with amode=3 or EOI check serial input and break loop if neccessary
    if ((AR488.amode==3) || rEoi) controller.serialIn_h();

    // Line terminator detected (loop breaks on command being detected or data buffer full)
    if (controller.lnRdy > 0) {
      controller.aRead = false;  // Stop auto read
      break;
    }

#ifdef DEBUG7
    if (eoiDetected) dbSerial->println(F("\r\nEOI detected."));
#endif

    // If break condition ocurred or ATN asserted then break here
    if (isAtnAsserted()) break;

#ifdef DEBUG7
    dbSerial->print(bytes[0], HEX), dbSerial->print(' ');
#else
    // Output the character to the serial port
    outstream.print((char)bytes[0]);
#endif

    // Byte counter
    x++;

    // EOI detection enabled and EOI detected?
    if (rEoi) {
      if (eoiDetected) break;
    }else{
      // Has a termination sequence been found ?
      if (isTerminatorDetected(bytes, eor)) break;
    }

    // Stop on timeout
    if (r > 0) break;

    // Shift last three bytes in memory
    bytes[2] = bytes[1];
    bytes[1] = bytes[0];
  }

#ifdef DEBUG7
  dbSerial->println();
  dbSerial->println(F("After loop flags:"));
  dbSerial->print(F("ATN: "));
  dbSerial->println(isAtnAsserted());
  dbSerial->print(F("TMO:  "));
  dbSerial->println(r);
#endif

  // End of data - if verbose, report how many bytes read
  if (AR488.isVerb) {
    outstream.print(F("Bytes read: "));
    outstream.println(x);
  }

  // Detected that EOI has been asserted
  if (eoiDetected) {
    if (AR488.isVerb) outstream.println(F("EOI detected!"));
    // If eot_enabled then add EOT character
    if (AR488.eot_en) outstream.print(AR488.eot_ch);
  }

  // Return rEoi to previous state
  rEoi = eoiStatus;

  // Verbose timeout error
  if (r > 0) {
    if (AR488.isVerb && r == 1) outstream.println(F("Timeout waiting for sender!"));
    if (AR488.isVerb && r == 2) outstream.println(F("Timeout waiting for transfer to complete!"));
  }

  // Return controller to idle state
  if (AR488.cmode == 2) {

    // Untalk bus and unlisten controller
    if (uaddrDev()) {
      if (AR488.isVerb) outstream.print(F("gpibSendData: Failed to untalk bus"));
    }

    // Set controller back to idle state
    setGpibControls(CIDS);

  } else {
    // Set device back to idle state
    setGpibControls(DIDS);
  }

#ifdef DEBUG7
    dbSerial->println(F("<- End listen."));
#endif

  // Reset flags
//  isReading = false;
  if (tranBrk > 0) tranBrk = 0;

  if (r > 0) return ERR;

  return OK;
}


/***** Check for terminator *****/
bool GPIB::isTerminatorDetected(uint8_t bytes[3], uint8_t eor_sequence){
  if (rEbt) {
    // Stop on specified <char> if appended to ++read command
    if (bytes[0] == eByte) return true;
  }else{
    // Look for specified terminator (CR+LF by default)
    switch (eor_sequence) {
      case 0:
          // CR+LF terminator
          if (bytes[0]==LF && bytes[1]==CR) return true;
          break;
      case 1:
          // CR only as terminator
          if (bytes[0]==CR) return true;
          break;
      case 2:
          // LF only as terminator
          if (bytes[0]==LF) return true;
          break;
      case 3:
          // No terminator (will rely on timeout)
          break;
      case 4:
          // Keithley can use LF+CR instead of CR+LF
          if (bytes[0]==CR && bytes[1]==LF) return true;
          break;
      case 5:
          // Solarton (possibly others) can also use ETX (0x03)
          if (bytes[0]==0x03) return true;
          break;
      case 6:
          // Solarton (possibly others) can also use CR+LF+ETX (0x03)
          if (bytes[0]==0x03 && bytes[1]==LF && bytes[2]==CR) return true;
          break;
      default:
          // Use CR+LF terminator by default
          if (bytes[0]==LF && bytes[1]==CR) return true;
          break;
      }
  }
  return false;
}


/***** Read a SINGLE BYTE of data from the GPIB bus using 3-way handshake *****/
/*
 * (- this function is called in a loop to read data    )
 * (- the GPIB bus must already be configured to listen )
 */
uint8_t GPIB::gpibReadByte(uint8_t *db, bool *eoi) {
  bool atnStat = (digitalRead(ATN) ? false : true); // Set to reverse, i.e. asserted=true; unasserted=false;
  *eoi = false;

  // Unassert NRFD (we are ready for more data)
  setGpibState(0b00000100, 0b00000100, 0);

  // ATN asserted and just got unasserted - abort - we are not ready yet
  if (atnStat && (digitalRead(ATN)==HIGH)) {
    setGpibState(0b00000000, 0b00000100, 0);
    return 3;
  }

  // Wait for DAV to go LOW indicating talker has finished setting data lines..
  if (Wait_on_pin_state(LOW, DAV, AR488.rtmo))  {
    if (AR488.isVerb) outstream.println(F("gpibReadByte: timeout waiting for DAV to go LOW"));
    setGpibState(0b00000000, 0b00000100, 0);
    // No more data for you?
    return 1;
  }

  // Assert NRFD (NOT ready - busy reading data)
  setGpibState(0b00000000, 0b00000100, 0);

  // Check for EOI signal
  if (rEoi && digitalRead(EOI) == LOW) *eoi = true;

  // read from DIO
  *db = readGpibDbus();

  // Unassert NDAC signalling data accepted
  setGpibState(0b00000010, 0b00000010, 0);

  // Wait for DAV to go HIGH indicating data no longer valid (i.e. transfer complete)
  if (Wait_on_pin_state(HIGH, DAV, AR488.rtmo))  {
    if (AR488.isVerb) outstream.println(F("gpibReadByte: timeout waiting DAV to go HIGH"));
    return 2;
  }

  // Re-assert NDAC - handshake complete, ready to accept data again
  setGpibState(0b00000000, 0b00000010, 0);

  // GPIB bus DELAY
  delayMicroseconds(AR488.tmbus);

  return 0;

}


/***** Write a SINGLE BYTE onto the GPIB bus using 3-way handshake *****/
/*
 * (- this function is called in a loop to send data )
 */
bool GPIB::gpibWriteByte(uint8_t db) {

  bool err;

  err = gpibWriteByteHandshake(db);

  // Unassert DAV
  setGpibState(0b00001000, 0b00001000, 0);

  // Reset the data bus
  setGpibDbus(0);

  // GPIB bus DELAY
  delayMicroseconds(AR488.tmbus);

  // Exit successfully
  return err;
}


/***** GPIB send byte handshake *****/
bool GPIB::gpibWriteByteHandshake(uint8_t db) {

    // Wait for NDAC to go LOW (indicating that devices are at attention)
  if (Wait_on_pin_state(LOW, NDAC, AR488.rtmo)) {
    if (AR488.isVerb) outstream.println(F("gpibWriteByte: timeout waiting for receiver attention [NDAC asserted]"));
    return true;
  }
  // Wait for NRFD to go HIGH (indicating that receiver is ready)
  if (Wait_on_pin_state(HIGH, NRFD, AR488.rtmo))  {
    if (AR488.isVerb) outstream.println(F("gpibWriteByte: timeout waiting for receiver ready - [NRFD unasserted]"));
    return true;
  }

  // Place data on the bus
  setGpibDbus(db);

  // Assert DAV (data is valid - ready to collect)
  setGpibState(0b00000000, 0b00001000, 0);

  // Wait for NRFD to go LOW (receiver accepting data)
  if (Wait_on_pin_state(LOW, NRFD, AR488.rtmo))  {
    if (AR488.isVerb) outstream.println(F("gpibWriteByte: timeout waiting for data to be accepted - [NRFD asserted]"));
    return true;
  }

  // Wait for NDAC to go HIGH (data accepted)
  if (Wait_on_pin_state(HIGH, NDAC, AR488.rtmo))  {
    if (AR488.isVerb) outstream.println(F("gpibWriteByte: timeout waiting for data accepted signal - [NDAC unasserted]"));
    return true;
  }

  return false;
}


/***** Untalk bus then address a device *****/
/*
 * dir: 0=listen; 1=talk;
 */
bool GPIB::addrDev(uint8_t addr, bool dir) {
  if (gpibSendCmd(GC_UNL)) return ERR;
  if (dir) {
    // Device to talk, controller to listen
    if (gpibSendCmd(GC_TAD + addr)) return ERR;
    if (gpibSendCmd(GC_LAD + AR488.caddr)) return ERR;
  } else {
    // Device to listen, controller to talk
    if (gpibSendCmd(GC_LAD + addr)) return ERR;
    if (gpibSendCmd(GC_TAD + AR488.caddr)) return ERR;
  }
  return OK;
}


/***** Unaddress a device (untalk bus) *****/
bool GPIB::uaddrDev() {
  // De-bounce
  delayMicroseconds(30);
  // Utalk/unlisten
  if (gpibSendCmd(GC_UNL)) return ERR;
  if (gpibSendCmd(GC_UNT)) return ERR;
  return OK;
}


/**********************************/
/*****  GPIB CONTROL ROUTINES *****/
/**********************************/


/***** Wait for "pin" to reach a specific state *****/
/*
 * Returns false on success, true on timeout.
 * Pin MUST be set as INPUT_PULLUP otherwise it will not change and simply time out!
 */
bool GPIB::Wait_on_pin_state(uint8_t state, uint8_t pin, int interval) {

  unsigned long timeout = millis() + interval;
  bool atnStat = (digitalRead(ATN) ? false : true); // Set to reverse - asserted=true; unasserted=false;

  while (digitalRead(pin) != state) {
    // Check timer
    if (millis() >= timeout) return true;
    // ATN status was asserted but now unasserted so abort
    if (atnStat && (digitalRead(ATN)==HIGH)) return true;
    //    if (digitalRead(EOI)==LOW) tranBrk = 2;
  }
  return false;        // = no timeout therefore succeeded!
}

/***** Control the GPIB bus - set various GPIB states *****/
/*
 * state is a predefined state (CINI, CIDS, CCMS, CLAS, CTAS, DINI, DIDS, DLAS, DTAS);
 * Bits control lines as follows: 8-ATN, 7-SRQ, 6-REN, 5-EOI, 4-DAV, 3-NRFD, 2-NDAC, 1-IFC
 * setGpibState byte1 (databits) : State - 0=LOW, 1=HIGH/INPUT_PULLUP; Direction - 0=input, 1=output;
 * setGpibState byte2 (mask)     : 0=unaffected, 1=enabled
 * setGpibState byte3 (mode)     : 0=set pin state, 1=set pin direction
 */
void GPIB::setGpibControls(uint8_t state) {

  // Switch state
  switch (state) {
    // Controller states

    case CINI:  // Initialisation
      // Set pin direction
      setGpibState(0b10111000, 0b11111111, 1);
      // Set pin state
      setGpibState(0b11011111, 0b11111111, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
  #ifdef SN7516X_DC
        digitalWrite(SN7516X_DC,LOW);
  #endif
  #ifdef SN7516X_SC
        digitalWrite(SN7516X_SC,HIGH);
  #endif
#endif
#ifdef DEBUG2
      dbSerial->println(F("Initialised GPIB control mode"));
#endif
      break;

    case CIDS:  // Controller idle state
      setGpibState(0b10111000, 0b10011110, 1);
      setGpibState(0b11011111, 0b10011110, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
#endif
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines to idle state"));
#endif
      break;

    case CCMS:  // Controller active - send commands
      setGpibState(0b10111001, 0b10011111, 1);
      setGpibState(0b01011111, 0b10011111, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines for sending a command"));
#endif
      break;

    case CLAS:  // Controller - read data bus
      // Set state for receiving data
      setGpibState(0b10100110, 0b10011110, 1);
      setGpibState(0b11011000, 0b10011110, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
#endif
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines for reading data"));
#endif
      break;

    case CTAS:  // Controller - write data bus
      setGpibState(0b10111001, 0b10011110, 1);
      setGpibState(0b11011111, 0b10011110, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines for writing data"));
#endif
      break;

    /* Bits control lines as follows: 8-ATN, 7-SRQ, 6-REN, 5-EOI, 4-DAV, 3-NRFD, 2-NDAC, 1-IFC */

    // Listener states
    case DINI:  // Listner initialisation
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
  #ifdef SN7516X_DC
        digitalWrite(SN7516X_DC,HIGH);
  #endif
  #ifdef SN7516X_SC
        digitalWrite(SN7516X_SC,LOW);
  #endif
#endif
      setGpibState(0b00000000, 0b11111111, 1);
      setGpibState(0b11111111, 0b11111111, 0);
#ifdef DEBUG2
      dbSerial->println(F("Initialised GPIB listener mode"));
#endif
      break;

    case DIDS:  // Device idle state
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif
      setGpibState(0b00000000, 0b00001110, 1);
      setGpibState(0b11111111, 0b00001110, 0);
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines to idle state"));
#endif
      break;

    case DLAS:  // Device listner active (actively listening - can handshake)
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
#endif
      setGpibState(0b00000110, 0b00001110, 1);
      setGpibState(0b11111001, 0b00001110, 0);
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines to idle state"));
#endif
      break;

    case DTAS:  // Device talker active (sending data)
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif
      setGpibState(0b00001000, 0b00001110, 1);
      setGpibState(0b11111001, 0b00001110, 0);
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines for listening as addresed device"));
#endif
      break;
#ifdef DEBUG2

    default:
      // Should never get here!
      //      setGpibState(0b00000110, 0b10111001, 0b11111111);
      dbSerial->println(F("Unknown GPIB state requested!"));
#endif
  }

  // Save state
  cstate = state;

  // GPIB bus delay (to allow state to settle)
  delayMicroseconds(AR488.tmbus);

}

/******************************************************/
/***** Device mode GPIB command handling routines *****/
/******************************************************/

/***** Attention handling routine *****/
/*
 * In device mode is invoked whenever ATN is asserted
 */
void GPIB::attnRequired() {

  uint8_t db = 0;
  uint8_t stat = 0;
  bool mla = false;
  bool mta = false;
  bool spe = false;
  bool spd = false;
  bool eoiDetected = false;

  // Set device listner active state (assert NDAC+NRFD (low), DAV=INPUT_PULLUP)
  setGpibControls(DLAS);

#ifdef DEBUG5
  dbSerial->println(F("Answering attention!"));
#endif

  // Read bytes
//  while (isATN) {
  while (digitalRead(ATN)==LOW) {
    stat = gpibReadByte(&db, &eoiDetected);
    if (!stat) {

#ifdef DEBUG5
      dbSerial->println(db, HEX);
#endif

      // Device is addressed to listen
      if (AR488.paddr == (db ^ 0x20)) { // MLA = db^0x20
#ifdef DEBUG5
        dbSerial->println(F("attnRequired: Controller wants me to data accept data <<<"));
#endif
        mla = true;
      }

      // Device is addressed to talk
      if (AR488.paddr == (db ^ 0x40)) { // MLA = db^0x40
          // Call talk handler to send data
          mta = true;
#ifdef DEBUG5
          if (!spe) dbSerial->println(F("attnRequired: Controller wants me to send data >>>"));
#endif
      }

      // Serial poll enable request
      if (db==GC_SPE) spe = true;

      // Serial poll disable request
      if (db==GC_SPD) spd = true;

      // Unlisten
      if (db==GC_UNL) unl_h();

      // Untalk
      if (db==GC_UNT) unt_h();

    }

  }

#ifdef DEBUG5
  dbSerial->println(F("End ATN loop."));
#endif

  if (mla) {
#ifdef DEBUG5
    dbSerial->println(F("Listening..."));
#endif
    // Call listen handler (receive data)
    mla_h();
    mla = false;
  }

  // Addressed to listen?
  if (mta) {
    // Serial poll enabled
    if (spe) {
#ifdef DEBUG5
      dbSerial->println(F("attnRequired: Received serial poll enable."));
#endif
      spe_h();
      spe = false;
    // Otherwise just send data
    }else{
      mta_h();
      mta = false;
    }
  }

  // Serial poll disable received
  if (spd) {
#ifdef DEBUG5
    dbSerial->println(F("attnRequired: Received serial poll disable."));
#endif
    spd_h();
    mta = false;
    spe = false;
    spd = false;
  }

  // Finished attention - set controls to idle
  setGpibControls(DIDS);

#ifdef DEBUG5
  dbSerial->println(F("attnRequired: END attnReceived."));
#endif

}


/***** Device is addressed to listen - so listen *****/
void GPIB::mla_h(){
  gpibReceiveData();
}


/***** Device is addressed to talk - so send data *****/
void GPIB::mta_h(){
  if (controller.lnRdy == 2) sendToInstrument(controller.pBuf, controller.pbPtr);
}


/***** Selected Device Clear *****/
void GPIB::sdc_h() {
  // If being addressed then reset
  if (AR488.isVerb) outstream.println(F("Resetting..."));
#ifdef DEBUG5
  dbSerial->print(F("Reset adressed to me: ")); dbSerial->println(aTl);
#endif
  if (aTl) controller.reset();
  if (AR488.isVerb) outstream.println(F("Reset failed."));
}


/***** Serial Poll Disable *****/
void GPIB::spd_h() {
  if (AR488.isVerb) outstream.println(F("<- serial poll request ended."));
}


/***** Serial Poll Enable *****/
void GPIB::spe_h() {
  if (AR488.isVerb) outstream.println(F("Serial poll request received from controller ->"));
  gpibSendStatus();
  if (AR488.isVerb) outstream.println(F("Status sent."));
  // Clear the SRQ bit
  AR488.stat = AR488.stat & ~0x40;
  // Clear the SRQ signal
  clrSrqSig();
  if (AR488.isVerb) outstream.println(F("SRQ bit cleared (if set)."));
}


/***** Unlisten *****/
void GPIB::unl_h() {
  // Stop receiving and go to idle
#ifdef DEBUG5
  dbSerial->println(F("Unlisten received."));
#endif
  rEoi = false;
  tranBrk = 3;  // Stop receving transmission
}


/***** Untalk *****/
void GPIB::unt_h() {
  // Stop sending data and go to idle
#ifdef DEBUG5
  dbSerial->println(F("Untalk received."));
#endif
}


void GPIB::lonMode(){

  gpibReceiveData();

  // Clear the buffer to prevent it getting blocked
  if (controller.lnRdy==2) controller.flushPbuf();

}

/***** Detect ATN state *****/
/*
 * When interrupts are being used the state is automatically flagged when
 * the ATN interrupt is triggered. Where the interrupt cannot be used the
 * state of the ATN line needs to be checked.
 */
bool GPIB::isAtnAsserted() {
#ifndef USE_INTERRUPTS
  // no interrupt, so check the current value
  setATN(digitalRead(ATN) == LOW);
#endif
  return isATN();
}


/****** Send data to instrument *****/
/* Processes the parse buffer whenever a full CR or LF
 * and sends data to the instrument
 */
void GPIB::sendToInstrument(char *buffr, uint8_t dsize) {

#ifdef DEBUG1
  dbSerial->print(F("sendToInstrument: Received for sending: ")); printHex(buffr, dsize);
#endif

  // Is this an instrument query command (string ending with ?)
  if (buffr[dsize-1] == '?') isQuery = true;

  // Send string to instrument
  gpibSendData(buffr, dsize);
  // Clear data buffer full flag
  if (controller.dataBufferFull) controller.dataBufferFull = false;

  // Show a prompt on completion?
  if (AR488.isVerb) controller.showPrompt();

  // Flush the parse buffer
  controller.flushPbuf();

#ifdef DEBUG1
  dbSerial->println(F("sendToInstrument: Sent."));
#endif

}


void GPIB::assertIfc() {
  if (AR488.cmode==2) {
    // Assert IFC
    setGpibState(0b00000000, 0b00000001, 0);
    delayMicroseconds(150);
    // De-assert IFC
    setGpibState(0b00000001, 0b00000001, 0);
    if (AR488.isVerb)
	  outstream.println(F("IFC signal asserted for 150 microseconds"));
  }
}
