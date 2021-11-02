#include <Arduino.h>
#include <EEPROM.h>
#include "AR488_Eeprom.h"

/***** AR488_Eeprom.cpp, ver. 0.00.05, 27/06/2020 *****/
/*
 * EEPROM functions implementation
 */



/***** Forward declarations of internal functions *****/
uint16_t getCRC16(uint8_t bytes[], uint16_t bsize);
unsigned long int getCRC32(uint8_t bytes[], uint16_t bsize);


/**********************************/
/***** COMMON FUNCTIONS *****/
/**********************************/


/***** Generate a CRC *****/

unsigned long int getCRC32(uint8_t bytes[], uint16_t bsize) {

  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };
  unsigned long crc = ~0L;

  for (uint16_t idx=0; idx<bsize; ++idx) {
    crc = crc_table[(crc ^ bytes[idx]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (bytes[idx] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}

uint16_t getCRC16(uint8_t bytes[], uint16_t bsize){
  uint8_t x;
  uint16_t crc = 0xFFFF;

  for (uint16_t idx=0; idx<bsize; ++idx) {
    x = crc >> 8 ^ bytes[idx];
    x ^= x>>4;
    crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
  }
  return crc;
}
