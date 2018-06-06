/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * bitsyxio.c : The bitsyX I/O routines
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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <fcntl.h>
#include "general.h"
#include "orcad.h"
#include "term.h"
#include "log.h"
#include "hydro.h"
#include "aquadopp.h"
#include "hardio.h"
#include "weather.h"


int initializeIO () {
  char option;
  int fd, r;

  // Say hi
  LOGPRINT( LVL_VERB, "intializeIO(): Entered" );

  //
  // Select SMARTIO option 
  //
  if (  ( fd = open("/dev/sio5", O_RDWR) ) < 0 ) {
    LOGPRINT( LVL_WARN, 
              "initializeIO(): Error opening SmartIO device /dev/sio5!" ); 
    return( FAILURE );
  }
  option = SMARTIO_INIT_OPTION;
  r = write(fd, &option, 1);
  close(fd);
  if ( r < 1 ) 
  {
    LOGPRINT( LVL_WARN, "initializeIO(): Error writing to /dev/sio5!" ); 
    return( FAILURE );
  }
  return( SUCCESS );
}

// 
// NAME
//   getOutputLine - get the state of a SMARTIO output line.
//
// SYNOPSIS
//   #include "hardio.h"
//
//   int getOutputLine( char port, char line );
//
// DESCRIPTION
//   Query the state ( 0/1 ) of the output line of a 
//   SMARTIO port.  
//
//   WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//   ADS has admitted that thier SIO drivers will always
//   return 2 bytes when reading from an SIO port.  This
//   means that the contract made by the UNIX read() system call
//   is broken by them.  If you try to specify size_t of 0 or 
//   1 bytes in read() you will get back 2 bytes!  Make room 
//   for this so that we don't overrun our buffer!  I found this
//   the hard way. 
//   WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//
// RETURNS
//   State of the given output line or -1 upon failure.
//
int getOutputLine ( char port, char line ) 
{
  int fd, ret;
  char currentState[2];  
  char portDevice[20];
  char portDirectionMask;
  char zeroMask = 0xFF;

  if ( port == PORTA ) {
    strcpy( portDevice,"/dev/sio1" );
    portDirectionMask = PORTA_DIRMASK;
    zeroMask = PORTA_ADMASK;
  }else if ( port == PORTB ) {
    strcpy(portDevice,"/dev/sio2");
    portDirectionMask = PORTB_DIRMASK;
  }else if ( port == PORTC ) {
    strcpy(portDevice,"/dev/sio3");
    portDirectionMask = PORTC_DIRMASK;
  }else if ( port == PORTD ) {
    strcpy(portDevice,"/dev/sio4");
    portDirectionMask = PORTD_DIRMASK;
  }else {
    // Wrong port used
    return( FAILURE );
  }

  //
  // Open the device for reading/writing
  //
  if (  ( fd = open(portDevice, O_RDWR) ) < 0 ) {
    // Could not open port!
    return( FAILURE );
  }

  //
  // Port Directions
  //
  ioctl(fd, SMARTIO_PORT_CONFIG, &portDirectionMask);
  
  ret = read(fd, currentState, 1);
  close(fd);
  return ( ( ( 0x01 << line ) & *currentState ) >> line  );

}


// 
// NAME
//   setOutputLine - Set the state of a SMARTIO output line.
//
// SYNOPSIS
//   #include "hardio.h"
//
//   int setOutputLine( char port, char line, char state );
//
// DESCRIPTION
//   Change the state ( 0/1 ) of the output line of a 
//   SMARTIO port.  
//
//   WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//   ADS has admitted that thier SIO drivers will always
//   return 2 bytes when reading from an SIO port.  This
//   means that the contract made by the UNIX read() system call
//   is broken by them.  If you try to specify size_t of 0 or 
//   1 bytes in read() you will get back 2 bytes!  Make room 
//   for this so that we don't overrun our buffer!  I found this
//   the hard way. 
//   WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//
//   NOTE: Function used in critical areas...do not log!
//
// RETURNS
//   -1 Upon failure
//    1 Upon success
//
int setOutputLine ( char port, char line, char state ) 
{
  int fd, ret;
  char currentState[2], newState, outMask;  
  char portDevice[20];
  char portDirectionMask;
  char zeroMask = 0xFF;

  if ( port == PORTA ) {
    strcpy( portDevice,"/dev/sio1" );
    portDirectionMask = PORTA_DIRMASK;
    zeroMask = PORTA_ADMASK;
  }else if ( port == PORTB ) {
    strcpy(portDevice,"/dev/sio2");
    portDirectionMask = PORTB_DIRMASK;
  }else if ( port == PORTC ) {
    strcpy(portDevice,"/dev/sio3");
    portDirectionMask = PORTC_DIRMASK;
  }else if ( port == PORTD ) {
    strcpy(portDevice,"/dev/sio4");
    portDirectionMask = PORTD_DIRMASK;
  }else {
    // Wrong port used
    return( FAILURE );
  }

  //
  // Open the device for reading/writing
  //
  if (  ( fd = open(portDevice, O_RDWR) ) < 0 ) {
    // Could not open port!
    return( FAILURE );
  }

  //
  // Port Directions
  //
  ioctl(fd, SMARTIO_PORT_CONFIG, &portDirectionMask);
  
  ret = read(fd, currentState, 1);
  outMask = (0x01 << line) & portDirectionMask;  
  if ( state ) {
    newState = *currentState | outMask;
  }else {
    newState = *currentState & ~outMask;
  }
  newState = newState & zeroMask;
  ret = write(fd, &newState, 1);
  close(fd);

  // TODO Consider returning something meaningful

  return( SUCCESS );
}


//
// NAME
//   emergencyPortClear - clear all port lines to low -- quickly.
//
// SYNOPSIS
//   #include "hardio.h"
//
//   int emergencyPortClear( char * port );
//
// DESCRIPTION
//   A simple routine to set all port lines to low without
//   too much overhead.  This is used when attempting to 
//   shutdown mechanical hardware ( such as the winch )
//   quickly after something goes wrong.
//
// RETURNS
//   Signal kind ( not quite safe ) 
//   routine to set port lines to low
//
int emergencyPortClear ( char * port )
{
  int fd;
  char newState;
  char portDirectionMask;

  //
  // Open the device for reading/writing
  //
  // What happens if this is called from a signal
  //   handler and another part of the program 
  //   has the port open??
  if (  ( fd = open( port, O_RDWR) ) > 0 )
  {
    portDirectionMask = 0xFF;
    newState = 0x00;
    //
    // Port Directions
    //
    // NOTE: This is probably not signal safe.  Let's
    //       hope we didn't interrupt an ioctl call. At
    //       least we don't expect to return to the
    //       interrupted program.
    ioctl(fd, SMARTIO_PORT_CONFIG, &portDirectionMask);
    //
    //  Do it now...now...now!
    //
    write(fd, &newState, 1);
  }else
  {
    // Uh-oh...now what do we do???
    // If we are here then we couldn't open the SMART IO
    //   port.  Freak out!
    return( -1 );
  }
  close( fd );
  return( 1 );
}

//
// NAME
//   getADValue - Get the analog signal magnitude at a A/D port.
//
// SYNOPSIS
//   #include "hardio.h"
//
//   long getADValue( char line );
//
// DESCRIPTION
//   Open the A/D port on the bitsyx and read the
//   A/D line.  
//
// RETURNS
//   The raw voltage on the line or -1 if something
//   went wrong.
//
long getADValue ( char line ) {
  int fd;
  long value, ret;

  // Say hi
  LOGPRINT( LVL_VERB, "getADValue(): Entered" );

  //
  // Open the device for reading
  //
  if (  ( fd = open( "/dev/sio8", O_RDONLY) ) < 0 ) {
    LOGPRINT( LVL_WARN, 
              "getADValue(): Error opening SmartIO device /dev/sio8!" );
    return -1;
  }

  // Read A/D channel
  value = (short)line; // Channel to read
  ret = read(fd, &value, 2); // ~ 125 usec per conversion
  //if (value > 0x3fff) {
  if ( 0 > value ) {
    // error...
    LOGPRINT( LVL_WARN, 
              "getADValue(): Negative value returned from A/D line!" );
  }
  close(fd);

  // Say goodbye
  LOGPRINT( LVL_VERB, "getADValue(): Returning: %d", value );
  return( value );
}

//
// NAME
//   getADVoltage - Get the analog signal voltage at a A/D port scaled.
//
// SYNOPSIS
//   #include "bitsyxio.h"
//
//   float getADVoltage( char line );
//
// DESCRIPTION
//   Dummy API placeholder. This is only available on
//   Raspberry PIs.
//
// RETURNS
//   0.0
//
float getADVoltage ( char line )
{
  return ( 0.0 );
}

//
// NAME
//   getADScaledValue - Get the analog signal magnitude at a A/D port scaled.
//
// SYNOPSIS
//   #include "hardio.h"
//
//   short getADScaledValue( char line );
//
// DESCRIPTION
//   Open the A/D port on the bitsyx and read the
//   A/D line raw value and scale it. 
//
// RETURNS
//   The raw voltage on the line scaled or -1 if something
//   went wrong.
//
float getADScaledValue ( char line, float scale ) 
{
  long r;

  // Say hi only
  LOGPRINT( LVL_VERB, "getADScaledValue(): Entered" );

  r = getADValue( line );
  return ( (float)(( r / (float)1023 ) * (float)2.5 * scale) );
}


