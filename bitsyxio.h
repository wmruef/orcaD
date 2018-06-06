/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * bitsyxio.h : Header for BitsyX I/O Functions
 *
 * Created: August 2005
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
#ifndef _BITSYXIO_H
#define _BITSYXIO_H

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
short getADValue ( char line );
int emergencyPortClear( char * port );
float getADVoltage ( char line );


#endif
