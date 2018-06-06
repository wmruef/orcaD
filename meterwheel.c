/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * meterwheel.c : Routines to interface with the meterwheel  
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <ftdi.h>
#include "term.h"
#include "log.h"
#include "general.h"
#include "meterwheel.h"
#include "buoy.h"
#include "orcad.h"
#include "serial.h"

//
// NAME
//   getMeterWheelPort - Initialize and return a sPort struct for
//                       whatever meterwheel is attached.
//
// SYNOPSIS
//   #include "meterwheel.h"
//
//   struct sPort *getMeterWheelPort();
//
// DESCRIPTION
//   We have several meter wheel counters now.  This routine
//   simply checks to see which one is attached, opens up
//   a communications to it and returns the serial port
//   datastructure ( sPort ) containing all the details.
//  
// RETURNS
//   struct *sPort or NULL upon failure
//
struct sPort *getMeterWheelPort ()
{
  if ( hasSerialDevice( AGO_METER_WHEEL_COUNTER ) > 0 )
  {
    return( getSerialDevice( AGO_METER_WHEEL_COUNTER) );
  }else if ( hasSerialDevice( ARDUINO_METER_WHEEL_COUNTER ) > 0 )
  {
    return( getSerialDevice( ARDUINO_METER_WHEEL_COUNTER) );
  }else 
  {
    return( NULL );
  }
}

//
// NAME
//   readMeterWheelAdjusted - Read meterwheel count and convert to meters
//
// SYNOPSIS
//   #include "meterwheel.h"
//
//   float readMeterWheelAdjusted( struct sPort *mwPort );
//
// DESCRIPTION
//   Read the meterwheel count and adjust distance for wheel
//   size.  See orcad.cfg for conversion factor information.
//   Note that the meterwheel counter's memory is volatile so 
//   the depth is relative to the position of the cable at
//   the time the counter circuit was powered up ( or manually
//   reset by the switch in the pressure case ). Also be aware
//   that the counter will recycle at 999.9 meters.
//  
// RETURNS
//   The meterwheel count in meters.   
//
float readMeterWheelAdjusted ( struct sPort *mwPort, double factor )
{
  float mwc;
  mwc = readMeterWheelCount( mwPort );
  if ( mwc >= 0 )
    return( (float)( mwc * factor ) );
  else
    return -1.0;
}

// 
// NAME
//   readMeterWheelCount - Read meterwheel count
//
// SYNOPSIS
//   #include "meterwheel.h"
//
//   float readMeterWheelCount( struct sPort *mwPort );
//
// DESCRIPTION
//   Read the meterwheel counter.  Note that the meterwheel 
//   counter's memory is volatile so the depth is relative 
//   to the position of the cable at the time the counter 
//   circuit was powered up ( or manually reset by the switch
//   in the pressure case ). Also be aware that the counter
//   will recycle at 999.9 meters.
//
// RETURNS
//   The wheel count in rotations.
//
// WARNING: Do not log this function!  We do not
//          want this to block since its used by
//          the movePackageUp/Down routines!
//
float readMeterWheelCount ( struct sPort *mwPort ) 
{
  char buffer[80];
  float retValue = -1.0;
  char *comaPtr = NULL;
  char *valuePtr = NULL;
  int mwFD = -1;
  int counts = -1;
  
  if ( mwPort->fileDescriptor > -1 )
  {
    mwFD = mwPort->fileDescriptor;
    term_flush( mwFD );
    if ( serialGetLine( mwFD, buffer, 80, 700L, "\n" ) < 21 ) 
      if ( serialGetLine( mwFD, buffer, 80, 700L, "\n" ) < 21 ) 
        if ( serialGetLine( mwFD, buffer, 80, 700L, "\n" ) < 21 ) 
          if ( serialGetLine( mwFD, buffer, 80, 700L, "\n" ) < 21 ) 
          {
            // Error communicating with meter wheel!
            return( -1.0 );
          }
        
    if (    ( ( comaPtr = index( buffer, ',' ) ) != NULL )
         && ( ( comaPtr = index( ++comaPtr, ',' ) ) != NULL ) ) 
    {
      valuePtr = ++comaPtr; 
      if ( ( comaPtr = index( valuePtr, ',' ) ) != NULL ) 
      {
        *comaPtr = '\0';
        if ( sscanf( valuePtr, "%g", &retValue ) == 1 ) {
          return( retValue );
        }
      }else {
        // Could not find third coma!
        return( -2.0 );
      }
    }
    // Could not find first two comas!
    return( -3.0 );
  }else if ( mwPort->ftdiContext != NULL )
  {
    if ( usbGetLine( mwPort, buffer, 80, 700L, "\n" ) < 9 ) 
      if ( usbGetLine( mwPort, buffer, 80, 700L, "\n" ) < 9 ) 
        if ( usbGetLine( mwPort, buffer, 80, 700L, "\n" ) < 9 ) 
          if ( usbGetLine( mwPort, buffer, 80, 700L, "\n" ) < 9 ) 
         {
            // Error communicating with meter wheel!
            return( -1.0 );
         }
    if ( ( comaPtr = index( buffer, ',' ) ) != NULL )
    {
      valuePtr = ++comaPtr; 
      if ( sscanf( valuePtr, "%d", &counts ) == 1 ) {
        return( (float)(counts * 1.0 ));
      }else {
        // Could not parse count!
        return( -2.0 );
      }
    }
    // Could not find coma!
    return( -3.0 );
  }else
  {
    // Could not figure out what device type this is!
    return( -4.0 );
  }
}
