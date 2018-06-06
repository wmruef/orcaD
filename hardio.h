/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * hardio.h : Header for BitsyX or Pi-Filling I/O Functions
 *
 * Created: July 2014
 *
 * Authors: Robert Hubley <rhubley@gmail.com>
 *          Wendi Ruef <wruef@ocean.washington.edu>
 * 
 * See LICENSE for conditions of use.
 *
 * $Id$
 *
 *********************************************************************
 * $Log$
 *
 *********************************************************************
 *
 */
#ifndef _HARDIO_H
#define _HARDIO_H

#ifdef BITSY
/*
 * BITSYX Hardware 
 */
#define SMARTIO_PORT_CONFIG _IOW('s', 4, char)
#define PORTA 0
#define PORTB 1
#define PORTC 2
#define PORTD 3

// Used by winch.c: This is very important to change
//                  if the winch control is moved 
//                  on the bitsyx hardware.  
#define WINCH_PORT "/dev/sio3"
#define SMARTIO_CONTROL_DEVICE = "/dev/sio5"
#define SMARTIO_PORTA_DEVICE = "/dev/sio8"


// Sets up PORT A as A/D and all the rest as I/O
#define SMARTIO_INIT_OPTION 2

// PORTA This port is special since it can have A/D and I/O
//       lines on the same port.  Remember that writing a "1"
//       to a A/D port will enable pull up resistors making
//       the recorded input voltage change.  For this port
//       we introduced a new masking concept ADMASK.  The
//       ADMASK indicates which lines ( with "0" ) should
//       never be written to with a logic 1 because this would
//       incorrectly turn on the pull up resistors.
// 
//        A0 = bit 0 = 0x00000001 
//        ADSSmartIO Port A ony has 4 ports available for use: A0-A3   
//
// Only A2 is a Input/Output port.  The rest are A/D.
#define PORTA_DIRMASK 0x04
// Ports A3, A1, and A0 are A/D and should never get written to.
#define PORTA_ADMASK 0xF4
// PORTB Nothing Used
#define PORTB_DIRMASK 0x00
// PORTC Currently PC0-PC4 are used
#define PORTC_DIRMASK 0x1F
// PORTD Nothing Used
#define PORTD_DIRMASK 0x00

#define EMERGENCY_WINCH_OFF emergencyPortClear( WINCH_PORT )
#define WINCH_PWR_STATUS getOutputLine( PORTC, 0 )
#define WINCH_ON setOutputLine( PORTC, 0, 1 )
#define WINCH_OFF setOutputLine( PORTC, 0, 0 )

#define WINCH_DIR_STATUS getOutputLine( PORTC, 1 )
// Strange problem on NPB Buoy.  Had to switch these! Fix! 
//#define WINCH_UP setOutputLine( PORTC, 1, 0 )
//#define WINCH_DOWN setOutputLine( PORTC, 1, 1 )
// Standard computers
#define WINCH_DOWN setOutputLine( PORTC, 1, 0 )
#define WINCH_UP setOutputLine( PORTC, 1, 1 )

#define HYDRO_STATUS getOutputLine( PORTC, 2 )
#define HYDRO_ON setOutputLine( PORTC, 2, 1 )
#define HYDRO_OFF setOutputLine( PORTC, 2, 0 )
#define WEATHER_STATUS getOutputLine( PORTC, 3 )
#define WEATHER_ON setOutputLine( PORTC, 3, 1 )
#define WEATHER_OFF setOutputLine( PORTC, 3, 0 )
#define METER_STATUS getOutputLine( PORTC, 4 )
#define METER_ON setOutputLine( PORTC, 4, 1 )
#define METER_OFF setOutputLine( PORTC, 4, 0 )
#define WIFI_STATUS getOutputLine( PORTA, 2 )
#define WIFIAMP_ON setOutputLine( PORTA, 2, 1 )
#define WIFIAMP_OFF setOutputLine( PORTA, 2, 0 )

#define GET_INTERNAL_BATTERY_VOLTAGE getADScaledValue( 1, 10.0 );
#define GET_EXTERNAL_BATTERY_VOLTAGE getADScaledValue( 0, 20.0 );
#define GET_SOLAR_RADIATION_VOLTAGE  getADScaledValue( 3, 1.0 );


int setOutputLine( char port, char line, char state );
int getOutputLine( char port, char line );
int initializeIO();
float getADScaledValue ( char line, float scale );
long getADValue ( char line );
int emergencyPortClear( char * port );


#elif RASPPI



#define PI_FILLING_MCP23017_ADDR 0x24
#define MCP23017_IODIRA  (__u8)0x00
#define MCP23017_IODIRB  (__u8)0x01
#define MCP23017_GPPUA   (__u8)0x0c
#define MCP23017_GPPUB   (__u8)0x0d
#define MCP23017_GPIOA   (__u8)0x12
#define MCP23017_GPIOB   (__u8)0x13

// Sets up the first six lines of PORT A
// to be output lines.
//#define MCP23017_IODIRA_DIRMASK 0xA0
// Not sure why I had to to do this.  It appears
//  that port 5 doesn't work as output unless I also
//  specify port 6 as output.  ie. 0x80 in IODIRA.
#define MCP23017_IODIRA_DIRMASK 0x00


// TODO This will go away
#define WINCH_PORT "/dev/sio3"
#define EMERGENCY_WINCH_OFF emergencyPortClear( WINCH_PORT )
#define WINCH_PWR_STATUS getOutputLine( 4 )
#define WINCH_ON setOutputLine( 4, 1 )
#define WINCH_OFF setOutputLine( 4, 0 )

#define WINCH_DIR_STATUS getOutputLine( 3 )
#define WINCH_DOWN setOutputLine( 3, 0 )
#define WINCH_UP setOutputLine( 3, 1 )

#define HYDRO_STATUS getOutputLine( 2 )
#define HYDRO_ON setOutputLine( 2, 1 )
#define HYDRO_OFF setOutputLine( 2, 0 )

#define WEATHER_STATUS getOutputLine( 1 )
#define WEATHER_ON setOutputLine( 1, 1 )
#define WEATHER_OFF setOutputLine( 1, 0 )

#define METER_STATUS getOutputLine( 0 )
#define METER_ON setOutputLine( 0, 1 )
#define METER_OFF setOutputLine( 0, 0 )

#define WIFI_STATUS getOutputLine( 5 )
#define WIFIAMP_ON setOutputLine( 5, 1 )
#define WIFIAMP_OFF setOutputLine( 5, 0 )

#define ODROID_STATUS getOutputLine( 6 )
#define ODROID_ON setOutputLine( 6, 1 )
#define ODROID_OFF setOutputLine( 6, 0 )

#define ZOOCAM_STATUS getOutputLine( 7 )
#define ZOOCAM_ON setOutputLine( 7, 1 )
#define ZOOCAM_OFF setOutputLine( 7, 0 )

// Setup a macro here for more I/O lines
// like:
// #define MY_COOL_DEVICE_ON setOutputLine(6,1)
// #define MY_COOL_DEVICE_OFF setOutputLine(6,0)

// 14V Sense:
//   Voltage Divider R1 = 88.7K, R2 = 10K
//   1 / ( R2 / ( R1 + R2 ) ) = 9.87
#define GET_INTERNAL_BATTERY_VOLTAGE ( getADVoltage(0) * 9.87 );
// 24V Sense:
//   Voltage Divider R1 = 191K, R2 = 10K
//   1 / ( R2 / ( R1 + R2 ) ) = 20.1
#define GET_EXTERNAL_BATTERY_VOLTAGE ( getADVoltage(1) * 20.1 );
// Just the raw voltage
#define GET_SOLAR_RADIATION_VOLTAGE  ( getADVoltage(2) );

int setOutputLine( char line, char state );
int getOutputLine( char line );
int initializeIO();
float getADScaledValue ( char line, float scale );
float getADVoltage ( char line );
long getADValue ( char line );
int emergencyPortClear( char * port );


#define PI_FILLING_MCP3424_1_ADDR 0x6A
#define PI_FILLING_MCP3424_2_ADDR 0x6C

#define MCP3424_PGA_X1 0x00
#define MCP3424_PGA_X2 0x01
#define MCP3424_PGA_X4 0x02
#define MCP3424_PGA_X8 0x03

#define MCP3424_12BIT 0x00
#define MCP3424_14BIT 0x04 
#define MCP3424_16BIT 0x08
#define MCP3424_18BIT 0x0C

#define MCP3424_SAMPLE_CONTINUOUS 0x10
#define MCP3424_START_CONV 0x80

#define MCP3424_CHAN_1 0x00
#define MCP3424_CHAN_2 0x20
#define MCP3424_CHAN_3 0x40
#define MCP3424_CHAN_4 0x60



#endif

#endif
