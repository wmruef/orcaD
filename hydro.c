/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * hydro.c : Abstract interface to underwater package
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
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include "general.h"
#include "ctd.h"
#include "hydro.h"
#include "log.h"
#include "orcad.h"


//
// NAME
//   initHydro - Initialize the CTD for normal operations
//
// SYNOPSIS
//   #include "hydro.h"
//   
//   int initHydro( int hydroDeviceType, int hydroFD );
//
// DESCRIPTION
//   Initialize the hydro device for general operations.
//   This operation differs for each type of device attached.
//   See individual device init routines for specifics.
//
// RETURN VALUE
//   The function returns 1 upon success and a -1 upon failure.
//
int initHydro ( int hydroDeviceType, int hydroFD ) {

  // Say hello
  LOGPRINT( LVL_DEBG, "initHydro(): Called" );

  if ( hydroDeviceType == SEABIRD_CTD_19 )
  {
    // Nothing to do for this type of device for now.
  }
  else if ( hydroDeviceType == SEABIRD_CTD_19_PLUS )
  {
    if ( initCTD19Plus( hydroFD ) < 0 )
    {
      return( FAILURE );
    }
  }
  else
  { 
    LOGPRINT( LVL_DEBG, "initHydro(): Unknown hydro device "
              "type ( %d )", hydroDeviceType );
    return( FAILURE );
  }

  if ( syncHydroTime( hydroDeviceType, hydroFD ) < 0 )
  {
    LOGPRINT( LVL_ALRT, "initHydro(): Could not sync bitsyx and "
                        "ctd clocks!" );
    return( FAILURE );
  }

  return( SUCCESS );

}


//
// NAME
//   stopHydroLogging - Cease logging of hydrowire device
//
// SYNOPSIS
//   #include "hydro.h"
//
//   int stopHydroLogging( int hydroDeviceType, int hydroFD );
//
// DESCRIPTION
//   At a high level logging of a hydrowire device involves
//   at the minimum streaming of pressure data to the surface.
//   Normally it also involves the capture of numerous types
//   of subsurface data on a datalogger for later download.
//   This routine stops this streaming of data.
//
// RETURNS
//   Returns 1 for success and -1 for failure.  The
//   cause of failure is logged at level LVL_WARN.
//
int stopHydroLogging ( int hydroDeviceType, int hydroFD )
{
  // Say hello
  LOGPRINT( LVL_DEBG, "stopHydroLogging(): Called" );

  if ( hydroDeviceType == SEABIRD_CTD_19 )
    return( stopLoggingCTD19( hydroFD ) );
  else if ( hydroDeviceType == SEABIRD_CTD_19_PLUS )
    return( stopLoggingCTD19Plus( hydroFD ) );
  else
  { 
    LOGPRINT( LVL_DEBG, "startHydroLogging(): Unknown hydro device "
              "type ( %d )", hydroDeviceType );
    return( FAILURE );
  }

}


//
// NAME
//   startHydroLogging - Initiate logging on a hydro wire device
//
// SYNOPSIS
//   #include "hydro.h"
//
//   int startHydroLogging( int hydroDeviceType, int hydroFD );
//
// DESCRIPTION
//   At a high level logging of a hydrowire device involves
//   at the minimum streaming of pressure data to the surface.
//   Normally it also involves the capture of numerous types
//   of subsurface data on a datalogger for later download.
//   This routine starts this streaming of data.
//
// RETURNS
//   Returns 1 for success and -1 for failure.  The
//   cause of failure is logged at level LVL_WARN.
//
int startHydroLogging ( int hydroDeviceType, int hydroFD )
{

  // Say hello
  LOGPRINT( LVL_DEBG, "startHydroLogging(): Called" );

  if ( hydroDeviceType == SEABIRD_CTD_19 )
  { printf( "Hmmm I think its a 19\n" );
    return( startLoggingCTD19( hydroFD ) );
  }
  else if ( hydroDeviceType == SEABIRD_CTD_19_PLUS )
    return( startLoggingCTD19Plus( hydroFD ) );
  else
  {
    LOGPRINT( LVL_DEBG, "startHydroLogging(): Unknown hydro device "
              "type ( %d )", hydroDeviceType );
    return( FAILURE );
  }

}


//
// NAME
//   getHydroPressure - Read the pressure from the hydro-wire data stream
//
// SYNOPSIS
//   #include "hydro.h"
//
//   double getHydroPressure( int hydroDeviceType, int hydroFD );
//
// DESCRIPTION
//   This routine simply decodes the hydro devices data stream
//   and pulls out the pressure data.
//
// RETURNS
//   The depth in meters or -1 for in the event of failure.
//
// WARNING: Do not add LOGGING TO THIS FUNCTION.  We do not 
//          want this function to block! It is used by 
//          the movePackageUp/Down functions!
double getHydroPressure ( int hydroDeviceType, int hydroFD )
{

  if ( hydroDeviceType == SEABIRD_CTD_19 )
    return( getCTD19Pressure( hydroFD ) ); 
  else if ( hydroDeviceType == SEABIRD_CTD_19_PLUS )
    return( getCTD19PlusPressure( hydroFD ) ); 
  else
    return( FAILURE );

}


// 
// NAME
//   downloadHydroData - Download data archives from hydro device
// 
// SYNOPSIS
//   #include "hydro.h"
//
//   int downloadHydroData( int hydroDeviceType, int hydroFD, FILE *outFile );
//
// DESCRIPTION
//   Download data stored in the data logger attached to the
//   hydro wire. The communications with the hydro wire are 
//   provided through an open/initialized file descriptor hydroFD.
//   The output file is passed in as outFile and should be 
//   already opened and writable.
//
// RETURNS
//   Data is written to the file and the file remains open.
//   Upon success a 1 is returned otherwise -1 is returned.
//   Specific problems are logged at a LVL_WARN level.
//     
int downloadHydroData ( int hydroDeviceType, int hydroFD, FILE * outFile ) 
{

  // Say hello
  LOGPRINT( LVL_DEBG, "downloadHydroData(): Called" );

  if ( hydroDeviceType == SEABIRD_CTD_19 )
    return( downloadCTD19Data( hydroFD, outFile ) );
  else if ( hydroDeviceType == SEABIRD_CTD_19_PLUS )
    return( downloadCTD19PlusData( hydroFD, outFile ) );
  else
  {
    LOGPRINT( LVL_DEBG, "startHydroLogging(): Unknown hydro device "
              "type ( %d )", hydroDeviceType );
    return( FAILURE );
  }

}


//
// NAME
//   syncHydroTime - Sync date and time on the CTD with our time.
//
// SYNOPSIS
//   #include "ctd.h"
//
// DESCRIPTION
//   Sync a Seabird CTD to the system date and time.
//
// RETURNS
//   1 Upon success
//  -1 Upon failure
//
int syncHydroTime ( int hydroDeviceType, int hydroFD ) {
  time_t now_t;
  struct tm *now_tm;
  time_t was_t;
  struct tm *was_tm;
  int timeOff = 0;

  // Say hello
  LOGPRINT( LVL_DEBG, "syncHydroTime(): Called" );


  // Get the system time
  now_t = time( NULL );
  now_tm = localtime( &now_t );

  if ( hydroDeviceType == SEABIRD_CTD_19 )
  {
    // Get the ctd time
    if ( ( was_tm = getCTD19Time( hydroFD ) ) == NULL )
    {
      LOGPRINT( LVL_WARN, "syncHydroTime(): Could not get "
                "ctd time!" );
      return( FAILURE );
    }
  }
  else if ( hydroDeviceType == SEABIRD_CTD_19_PLUS )
  {
    // Get the ctd time
    if ( ( was_tm = getCTD19PlusTime( hydroFD ) ) == NULL )
    {
      LOGPRINT( LVL_WARN, "syncHydroTime(): Could not get "
                "ctd time!" );
      return( FAILURE );
    }
  }
  else
  {
    LOGPRINT( LVL_WARN, "syncHydroTime(): Unknown hydro device "
              "type ( %d )", hydroDeviceType );
    return( FAILURE );
  }

  if ( ( was_t = mktime( was_tm ) ) < 0 )
  {
    LOGPRINT( LVL_ALRT, "syncHydroTime(): The ctd clock data was "
              "garbled!  Giving up." );
    return( FAILURE );
  }else {
    // See if it's outrageously off
    if ( was_t < now_t && ( now_t - was_t ) > 60 )
    {
      LOGPRINT( LVL_ALRT, "syncHydroTime(): The ctd clock was behind "
                "by %d seconds! %d %d", ( now_t - was_t ), now_t, was_t );
      timeOff = 1;
    }else if ( was_t > now_t && ( was_t - now_t ) > 60 )
    {
      LOGPRINT( LVL_ALRT, "syncHydroTime(): The ctd clock was fast "
                "by %d seconds!", ( was_t - now_t ) );
      timeOff = 1;
    }
  }

  if ( timeOff ) 
  { 
    if ( hydroDeviceType == SEABIRD_CTD_19 )
    {
      sleep( 1 );
      if ( setCTD19Time( hydroFD, now_tm ) < 0 )
      {
        LOGPRINT( LVL_ALRT, "syncHydroTime(): Could not set the CTD time!" );
        return( FAILURE );
      }

      // Get the system time
      now_t = time( NULL );
      now_tm = localtime( &now_t );

      // Get it again
      if ( ( was_tm = getCTD19Time( hydroFD ) ) == NULL )
      {
        LOGPRINT( LVL_WARN, "syncHydroTime(): Could not get ctd time 2nd " 
                            "time!" );
        return( FAILURE );
      }
    }
    else if ( hydroDeviceType == SEABIRD_CTD_19_PLUS )
    {
      // Set the time
      if ( setCTD19PlusTime( hydroFD, now_tm ) < 0 )
      {
        LOGPRINT( LVL_ALRT, "syncHydroTime(): Could not set the CTD time!" );
        return( FAILURE );
      }

      // Get the system time
      now_t = time( NULL );
      now_tm = localtime( &now_t );

      // Get it again
      if ( ( was_tm = getCTD19PlusTime( hydroFD ) ) == NULL )
      {
        LOGPRINT( LVL_WARN, "syncHydroTime(): Could not get ctd time 2nd "
                            "time!" );      
        return( FAILURE );
      }
    }
  
    // and make sure we set it correctly
    was_t = mktime( was_tm );
    if ( abs( was_t - now_t ) > 20 )
    {
      LOGPRINT( LVL_ALRT, "syncHydroTime(): After attempting to set the ctd "
                "time we are still off by %d seconds!  Failed to set "
                "the time correctly!", ( was_t - now_t ) );
      return( FAILURE );
    }
  }

  return( SUCCESS );
}



