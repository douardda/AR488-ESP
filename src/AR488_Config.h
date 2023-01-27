#ifndef AR488_CONFIG_H
#define AR488_CONFIG_H

/*********************************************/
/***** AR488 GLOBAL CONFIGURATION HEADER *****/
/***** vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv *****/


/***** Firmware version *****/
#define FWVER "AR488 GPIB controller, ver. 0.49.14, 02/03/2021"

/***** BOARD CONFIGURATION *****/
/*
 * Platform will be selected automatically based on
 * Arduino definition.
 * Only ONE board/layout should be selected per platform
 * Only ONE Serial port can be used to receive output
 */


/*** Custom layout ***/
/*
 * Uncomment to use custom board layout
 */
//#define AR488_CUSTOM

/*
 * Configure the appropriate board/layout section
 * below as required
 */
#ifdef AR488_CUSTOM
  /* Board layout */
  /*
   * Define board layout in the AR488 CUSTOM LAYOUT
   * section below
   */
  /* Serial ports */
  #define AR_HW_SERIAL
#ifndef AR_SERIAL_PORT
  #define AR_SERIAL_PORT Serial
  //#define AR_SERIAL_PORT Serial1
  //#define AR_SERIAL_PORT Serial2
  //#define AR_SERIAL_PORT Serial3
  //#define AR_CDC_SERIAL
  //#define AR_SW_SERIAL
  #define AR_BT_SERIAL_PORT Serial
#endif

/*** UNO and NANO boards ***/
#elif __AVR_ATmega328P__
  /* Board/layout selection */
#if !defined(AR488_UNO) && !defined(AR488_NANO)
  #define AR488_UNO
  //#define AR488_NANO
#endif
  /*** Serial ports ***/
  //Select HardwareSerial or SoftwareSerial (default = HardwareSerial) ***/
  // The UNO/NANO default hardware port is 'Serial'
  // (Comment out #define AR_HW_SERIAL if using SoftwareSerial)
  #define AR_HW_SERIAL
  #ifdef AR_HW_SERIAL
    #define AR_SERIAL_PORT Serial
    #define AR_BT_SERIAL_PORT Serial
  #else
    // Select software serial port
    #define AR_SW_SERIAL
  #endif

/*** MEGA 32U4 based boards (Micro, Leonardo) ***/
#elif __AVR_ATmega32U4__
  /*** Board/layout selection ***/
  #define AR488_MEGA32U4_MICRO  // Artag's design for Micro board
  //#define AR488_MEGA32U4_LR3  // Leonardo R3 (same pin layout as Uno)
  /*** Serial ports ***/
  // By default the CDC serial port is used
  // Comment out #define AR_CDC_SERIAL if using RXI, TXO pins
  #define AR_CDC_SERIAL
  #ifdef AR_CDC_SERIAL
    // The Mega 32u4 default port is a virtual USB CDC port named 'Serial'
    #define AR_SERIAL_PORT Serial
    #define AR_BT_SERIAL_PORT Serial
  #else
    // Use hardware port Serial1
    #define AR_HW_SERIAL
    #define AR_SERIAL_PORT Serial1
    #define AR_BT_SERIAL_PORT Serial1
  #endif

/*** MEGA 2560 board ***/
#elif __AVR_ATmega2560__
  /*** Board/layout selection ***/
#if !defined(AR488_MEGA2560_D) && !defined(AR488_MEGA2560_E1) & !defined(AR488_MEGA2560_E2)
  #define AR488_MEGA2560_D
  //#define AR488_MEGA2560_E1
  //#define AR488_MEGA2560_E2
#endif
  /*** Serial ports ***/
  // Mega 2560 supports Serial, Serial1, Serial2, Serial3. Since the pins
  // associated with Serial2 are used in the default pin layout, Serial2
  // is unavailable. The default port is 'Serial'. Choose ONE port only.
/*
  // and associated SERIALEVENT definition
*/
  #define AR_HW_SERIAL
#ifndef AR_SERIAL_PORT
  #define AR_SERIAL_PORT Serial
  //#define AR_SERIAL_PORT Serial1
  //#define AR_SERIAL_PORT Serial3
  #define AR_BT_SERIAL_PORT Serial
#endif

#endif  // Board/layout selection


/***** Software Serial Support *****/
/*
 * Configure the SoftwareSerial TX/RX pins and baud rate here
 * Note: SoftwareSerial support can conflicts with PCINT support
 * if so, when using SoftwareSerial, disable USE_INTERRUPTS.
 */
#ifdef AR_SW_SERIAL
  #define AR_SW_SERIAL_RX 53
  #define AR_SW_SERIAL_TX 51
  #define AR_SW_SERIAL_BAUD 57600
#else
  #define AR_SERIAL_BAUD 115200
#endif
/*
 * Note: SoftwareSerial reliable only up to a MAX of 57600 baud only
 */


/***** Pin State Detection *****/
/*
 * With UNO. NANO and MEGA boards with pre-defined layouts,
 * USE_INTERRUPTS can and should be used.
 * With the AR488_CUSTOM layout and unknown boards, USE_INTERRUPTS must
 * be commented out. Interrupts are used on pre-defined AVR board layouts
 * and will respond faster, however in-loop checking for state of pin states
 * can be supported with any board layout.
 */
#ifdef __AVR__
  // For supported boards use interrupt handlers
  #if defined (AR488_UNO) || defined (AR488_NANO) || defined (AR488_MEGA2560) || defined (AR488_MEGA32U4)
    #ifndef AR488_CUSTOM
      #define USE_INTERRUPTS
    #endif
  #endif
#endif


/***** Enable Macros *****/
/*
 * Uncomment to enable macro support. The Startup macro allows the
 * interface to be configured at startup. Macros 1 - 9 can be
 * used to execute a sequence of commands with a single command
 * i.e, ++macro n, where n is the number of the macro
 *
 * USE_MACROS must be enabled to enable the macro feature including
 * macro_0 (the startup macro).
 */
//#define USE_MACROS    // Enable the macro feature


/***** Enable SN7516x chips *****/
/*
 * Uncomment to enable the use of SN7516x GPIB tranceiver ICs.
 * This will require the use of an additional GPIO pin to control
 * the read and write modes of the ICs.
 */
//#define SN7516X
//#ifdef SN7516X
//  #define SN7516X_TE 7
//  #define SN7516X_DC 13
//  #define SN7516X_SC 12
//#endif


/***** Bluetooth (HC05) support *****/
/*
 * Uses built-in LED on GPIO pin 13 to signal status
 */
//#define AR_HC05_EN 12      // HC05 Bluetooth module enable and control pin
#ifdef AR488_BT_ENABLE
  #define AR_BT_BAUD 115200     // Bluetooth module preferred baud rate
  #define AR_BT_NAME "AR488-BT" // Bluetooth device name
  #define AR_BT_CODE "488488"   // Bluetooth pairing code
#endif


/***** Acknowledge interface is ready *****/
//#define SAY_HELLO

/***** Debug options *****/
// Uncomment to send debug messages to another port
//#define DB_SERIAL_PORT Serial1
// Configure alternative port for debug messages
#define DB_SERIAL_BAUD 115200
#define DB_HW_SERIAL
#ifdef DB_SW_SERIAL
  #define DB_SW_SERIAL_RX 53
  #define DB_SW_SERIAL_TX 51
#endif

// Configure debug level
//#define DEBUG1  // getCmd
//#define DEBUG2  // setGpibControls
//#define DEBUG3  // gpibSendData
//#define DEBUG4  // spoll_h
//#define DEBUG5  // attnRequired
//#define DEBUG6  // EEPROM
//#define DEBUG7  // gpibReceiveData
//#define DEBUG8  // ppoll_h
//#define DEBUG9  // bluetooth
//#define DEBUG10 // ID command


/***** ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ *****/
/***** AR488 GLOBAL CONFIGURATION HEADER *****/
/*********************************************/

#endif // AR488_CONFIG_H
