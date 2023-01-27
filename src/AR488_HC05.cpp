#include <Arduino.h>

#ifdef AR488_BT_HC05

#include "AR488_HC05.h"

#if defined(AR_SW_SERIAL)
#include <SoftwareSerial.h>
#endif

/***** AR488_BT.cpp, ver. 0.48.24, 25/04/2020 *****/
/*
* AR488: HC05 BlueTooth module for Arduino
*/

// Buffer
#define RBSIZE 64
char rBuf[RBSIZE];

void hc05Init(Stream *);
bool hc05DetectBaud(Stream *serial);
bool hc05ChkCfg(Stream *serial);
bool hc05Cfg(Stream *serial);
bool hc05AtReply(Stream* serial, const char* reply);
void blinkLed(uint8_t count);


/******************************/
/*****  BLUETOOTH SUPPORT *****/
/******************************/

/***** Initialise *****/
void hc05Init(Stream *serial) {
#if defined(AR_CDC_SERIAL)
  Serial_ *btSerial = (Serial_*) serial;
#elif defined(AR_HW_SERIAL)
  HardwareSerial *btSerial = (HardwareSerial*) serial;
#elif defined(AR_SW_SERIAL)
  SoftwareSerial *btSerial = (SoftwareSerial*) serial;
#endif

  // Enable bluetooth HC05 board config mode
  pinMode(AR_HC05_EN, OUTPUT);
  digitalWrite(AR_HC05_EN, HIGH);
  // Detect baud rate
  if (hc05DetectBaud(serial)) {
    // Baud rate detected
    blinkLed(2);
    delay(400);
    // Are we already configured?
    if (hc05ChkCfg(serial)) {
      // Yes - blink LED once
      delay(400);
      blinkLed(1);
    }else{
      // No - then configure
      delay(400);
      // Configure BT
      if (hc05Cfg(serial)) blinkLed(3);
    }
  }

  // Reset bluetooth HC05 board and enter user mode
  delay(2000);
  digitalWrite(AR_HC05_EN, LOW);
  delay(500);
  btSerial->print(F("AT+RESET\r\n"));
  delay(500);
  btSerial->end();
  //arSerial_->begin(AR_BT_BAUD);  // XXX TODO
}


/***** Detect the HC05 board config mode baud rate *****/
bool hc05DetectBaud(Stream *serial){
#if defined(AR_CDC_SERIAL)
  Serial_ *btSerial = (Serial_*) serial;
#elif defined(AR_HW_SERIAL)
  HardwareSerial *btSerial = (HardwareSerial*) serial;
#elif defined(AR_SW_SERIAL)
  SoftwareSerial *btSerial = (SoftwareSerial*) serial;
#endif
  long int brate[5] = {9600, 19200, 38400, 57600, 115200};
  uint8_t i = 0;
  while (i<5){
    btSerial->begin(brate[i]);
    delay(400);
    btSerial->println("AT");
    delay(200);
    if (hc05AtReply(serial, "OK")) return true;
    i++;
  }
  return false;
}


/***** Check configuration for change *****/
bool hc05ChkCfg(Stream *serial){
  char baudstr[20];
  char baudrate[8];

  memset(baudstr, '\0', 20);
  memset(baudrate, '\0', 8);

  sprintf(baudrate, "%ld", AR_BT_BAUD);
  strncpy(baudstr, "+UART:", 6);
  strcat(baudstr, baudrate);
  strcat(baudstr, ",1,0");

  serial->println(F("AT+NAME?"));
  delay(200);
  if (!hc05AtReply(serial, "+NAME:" AR_BT_NAME)) return false;
  delay(200);

  serial->println(F("AT+UART?"));
  delay(200);
  if(!hc05AtReply(serial, baudstr)) return false;
  delay(200);

  serial->println(F("AT+PSWD?"));
  delay(200);
  if (!hc05AtReply(serial, "+PIN:\"" AR_BT_CODE "\"")) return false;

  return true;
}


/***** Configure the HC05 *****/
bool hc05Cfg(Stream *serial){
  serial->println(F("AT+ROLE=0"));
  delay(200);
  if (!hc05AtReply(serial, "OK")) return false;

  serial->println(F("AT+NAME=\"" AR_BT_NAME "\""));
  delay(200);
  if (!hc05AtReply(serial, "OK")) return false;

  serial->println(F("AT+PSWD=\"" AR_BT_CODE "\""));
  delay(200);
  if (!hc05AtReply(serial, "OK")) return false;

  serial->print(F("AT+UART="));
  serial->print(AR_BT_BAUD);
  serial->println(F(",1,0"));
  delay(200);
  if (!hc05AtReply(serial, "OK")) return false;

  return true;
}


/***** Is the reply what we expected? *****/
bool hc05AtReply(Stream* serial, const char* reply) {
  int sz = strlen(reply);

  memset(rBuf, '\0', RBSIZE);
  // Read the first line
  serial->readBytesUntil(0x0A, rBuf, RBSIZE-1);
  // Discared the rest
  while (serial->available()){
    serial->read();
  }
  if (strncmp(reply, rBuf, sz) == 0) return true;
  return false;
}


/***** Blink the internal LED *****/
void blinkLed(uint8_t count){
#ifdef LED_BUILTIN
  for (uint8_t i=0; i<count; i++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }
#endif
}

/*************************************/
/*****  ENF OF HC05 BLUETOOTH SUPPORT *****/
/*************************************/

#endif  // AR_BT_HC05
