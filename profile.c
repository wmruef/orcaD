/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * profile.c : The profiling routines
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
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "general.h"
#include "orcad.h"
#include "hardio.h"
#include "buoy.h"
#include "ctd.h"
#include "hydro.h"
#include "log.h"
#include "meterwheel.h"
#include "timer.h"
#include "winch.h"
#include "profile.h"
#include "aquadopp.h"

// TANK TESTING CHIMERAS
//#define movePackageUpDiscretely(a,b,c,d,e,f,g) sleep( 155 )
//#define movePackageDown(a,b,c,d) sleep( 310 )

// 
// NAME
//  profile - Run a profile mission.
//
// SYNOPSIS
//   #include "profile.h"
//
//   int profile( struct mission *missn );
//
// DESCRIPTION
//   Given the mission parameters ( missn ) run a profile
//   of the instrument package. 
//
//   BASIC PROFILE FLOW:
//
//   1. Activate CTD
//   2. Get pressure from CTD
//       a. Exit if pressure is not within range.
//   3. Wait 2 minutes for oxygen sensor to warm up.
//   4. Check battery voltage
//   5. Go up to surface discrete sampling along the way.
//       a. Subtract 0.25 from cycle.
//   6. Recheck pressure
//   7. Recheck battery voltage 
//   8. Wait for 20 seconds for sensors to equilibrate
//   9. While cycle > 0 
//        a. Perform one of the following operations depending
//           on the state of cycle counter:
//              - Go down to the bottom in one continuous motion.
//              - Go down to the parking depth.
//              - Go up to the surface discrete sampling the whole way.
//              - Go up to parking depth sampling the whole way.
//        b. Subtract the equivalant cycle amount from the cycle counter.
//  10. Stop CTD Logging
//
//  A profile cycle is best visualized as a sine wave where
//  the baseline is the parking depth of the package.  The
//  top of the cycle is the shallow depth and bottom of the wave
//  is the deep depth of a cast.  A complete cycle would be 
//  a profile which goes from parking depth to shallow, to deep
//  and then back to parking depth. I.e:
//
//     cycle = 0.5 = shallow cast
//     cycle = 1.0 = deep cast
//     cycle = 1.5 = deep + shallow etc.
//
// RETURNS
//   1 Upon success
//  -1 Upon failure
//
int profile ( struct mission *missn )
{
  unsigned int sleepSec;
  float  intbatt,extbatt;
  int hydroDeviceType;
  int hydroFD = -1;
  int hydroAuxFD = -1;
  float meterWheelDepth = 0;
  float tgtDepth = 1;
  double pressure;
  double pressureDepth;
  int ret = 0;
  float cycles = 0;
  struct sPort *mwPort = NULL;

  LOGPRINT( LVL_INFO, "profile(): Entered..." );


  //
  // INITIALIZE WINCH RELAYS 
  //   They should already be off...but it never hurts to 
  //   make sure by setting it to off.  The electrical
  //   outputs don't appear to bounce if you attempt to
  //   redundantly set the state.
  //
  if ( WINCH_DOWN < 0 )
  {
    LOGPRINT( LVL_ALRT, "profile(): Failed to verify winch direction down" );
    return( ESTUP );
  }
  if ( WINCH_OFF < 0 ) 
  {
    LOGPRINT( LVL_ALRT, "profile(): Failed to verify winch power off" );
    return( ESTUP );
  }

  // Initialize cycles
  cycles = missn->cycles;

  //
  // OPEN SERIAL PORTS TO HYDROWIRE AND METERWHEEL
  //
  if ( ( hydroDeviceType = getHydroWireDeviceType() )  < 0 )
  {
    LOGPRINT( LVL_ALRT, "profile(): Could not get hydro wire device type!" );
    return( ESTUP );
  }

  if ( hasSerialDevice( AGO_METER_WHEEL_COUNTER ) > 0 || 
       hasSerialDevice( ARDUINO_METER_WHEEL_COUNTER ) > 0 )
  {
    if ( ( mwPort = getMeterWheelPort() ) == NULL )
    {
      LOGPRINT( LVL_ALRT, 
                "profile(): Could not get meter wheel port!" );
      return( ESTUP );
    }
  }else 
  {
    LOGPRINT( LVL_ALRT, 
              "profile(): NOTE: Running profile without a meterwheel.  Winch control based on CTD pressure only." );
  }

  if ( ( hydroFD = getDeviceFileDescriptor( hydroDeviceType ) ) < 0 )
  {
    LOGPRINT( LVL_ALRT, 
              "profile(): Could not get hydroDevice (%d) file descriptor!",
              hydroDeviceType );
    return( ESTUP );
  }

  // 
  // Check to see if we have an auxilary sampling device
  //
  //   TODO: Currently only an AQUADOPP....but make this more 
  //         general as with the main serial port.
  //
  if ( hasSerialDevice( AQUADOPP ) > 0 )
  {
    // Open up the serial port
    if( ( hydroAuxFD = getDeviceFileDescriptor( AQUADOPP ) ) < 0 )
    {
      LOGPRINT( LVL_ALRT, "profile(): Could not open up the "
                "aquadopp serial port!" );
    }
  }

  // 
  // READ METERWHEEL BASELINE
  //
  if ( mwPort && ( meterWheelDepth = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor ) ) < 0 )
  {
    LOGPRINT( LVL_ALRT,
              "profile(): Could not read the meter wheel counter!" );
    return( ESTUP );
  }

  //
  // GET BATTERY STATUS
  //
  intbatt = GET_INTERNAL_BATTERY_VOLTAGE; 
  if ( intbatt < 0 ) 
    intbatt = GET_INTERNAL_BATTERY_VOLTAGE; 
  extbatt = GET_EXTERNAL_BATTERY_VOLTAGE;
  if ( extbatt < MINEXTVOLT )
  {
    extbatt = GET_EXTERNAL_BATTERY_VOLTAGE;
    if ( extbatt < MINEXTVOLT )
    {
      LOGPRINT( LVL_ALRT,
              "profile(): Battery voltage too low to continue with profile! ( intVolt = %4.1fv extVolt = %4.1fv", intbatt, extbatt );
      return( ESTUP );
    }
  }

  //
  // ACTIVATE CTD
  //
  LOGPRINT( LVL_INFO, "profile(): Activating CTD" );
  if ( startHydroLogging( hydroDeviceType, hydroFD ) < 0 ) 
  {
    LOGPRINT( LVL_ALRT, "profile(): Failed to activate the CTD!" );
    return( ESTUP );
  }

  //
  //  GET PRESSURE FROM CTD
  //
  if ( ( pressure = getHydroPressure( hydroDeviceType, hydroFD ) ) < 0 ) 
  {
    LOGPRINT( LVL_ALRT, "profile(): Could not get CTD pressure!" );
    LOGPRINT( LVL_INFO, "profile(): Stopping CTD logging..." );
    if ( ( ret = stopHydroLogging( hydroDeviceType, hydroFD ) ) < 0 )
    {
      // For now consider this a warning not a failure.
      LOGPRINT( LVL_ALRT, "profile(): Could not stop CTD from logging! "
                " Function returned %d.", ret );
    }
    return( ESTUP );
  }
  pressureDepth = convertDBToDepth( pressure );

  // Normal status print
  LOGPRINT( LVL_ALWY, 
            "profile(): pres=%6.2fdb prdp=%6.2fm mwdp=%6.1f "
            "ipwr=%4.1fv epwr=%4.1fv", pressure, pressureDepth,
            meterWheelDepth, intbatt, extbatt );

  // Check that CTD is within 5 db of parking pressure 
  if ( fabs( pressureDepth - opts.parkingDepth) > 5.0 ) 
  { 
     LOGPRINT( LVL_ALRT, "profile(): Warning package is *not* within 5 meters "
               "of parking depth ( %d )! Continuing!", opts.parkingDepth );
  }
  
  //
  // WAIT FOR 2 MINUTES FOR OXYGEN SENSOR TO WARM UP
  //
  sleepSec = 120;
  LOGPRINT( LVL_INFO, 
      "profile(): Sleeping for %d seconds while the oxygen sensor warms up.", 
      sleepSec );
  while ( ( sleepSec = sleep( sleepSec ) ) > 0 ) { /* nothing */ }


  // 
  // Start auxilary sampling if need be
  // 
  if (    hydroAuxFD >= 0
       && (    missn->auxSampleDirection == UP 
            || missn->auxSampleDirection == BOTH ) )
  {
    if ( startLoggingAquadopp( hydroAuxFD ) < 0 )
    {
      LOGPRINT( LVL_ALRT, "profile(): Failed to start aquadopp logging!" );
    }else {
      LOGPRINT( LVL_NOTC, "profile(): Started Aquadopp Logging..." );
    }
  }
 
  // If we have something to do ( always! )
  if ( cycles > 0 ) 
  {

    LOGPRINT( LVL_INFO, 
              "profile(): Moving package up discretely to %d meters",
              opts.minDepth );
    ret = movePackageUpDiscretely( hydroFD, hydroDeviceType,
                                   mwPort, opts.minDepth,
                                   missn->depths, missn->numDepths,
                                   missn->equilibrationTime );
    if ( ret < 0 ) 
    {
      LOGPRINT( LVL_ALRT, "profile(): Failed to move package up (status = %d)."
                "  The package may no longer be at parking depth!",
                ret );
      LOGPRINT( LVL_INFO, "profile(): Stopping CTD logging..." );
      if ( ( ret = stopHydroLogging( hydroDeviceType, hydroFD ) ) < 0 )
      {
        // For now consider this a warning not a failure.
        LOGPRINT( LVL_ALRT, "profile(): Could not stop CTD from logging! "
                  " Function returned %d.", ret );
      }
      return( EPROF );
    }

    // 
    // Basic print status
    //
    if ( mwPort )
      meterWheelDepth = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor );
    intbatt = GET_INTERNAL_BATTERY_VOLTAGE;
    if ( intbatt < 0 )
      intbatt = GET_INTERNAL_BATTERY_VOLTAGE;
    extbatt = GET_EXTERNAL_BATTERY_VOLTAGE;
    if ( extbatt < 0 ) 
      extbatt = GET_EXTERNAL_BATTERY_VOLTAGE;
    pressure = getHydroPressure( hydroDeviceType, hydroFD );
    pressureDepth = convertDBToDepth( pressure );
    LOGPRINT( LVL_ALWY, 
              "profile(): pres=%6.2fdb prdp=%6.2fm mwdp=%6.1f "
              "ipwr=%4.1fv epwr=%4.1fv", pressure, pressureDepth,
              meterWheelDepth, intbatt, extbatt );

    // 
    // Stop the auxilary sampling if need be
    //
    if (    hydroAuxFD >= 0
         && missn->auxSampleDirection == UP  )
    {
      if ( stopLoggingAquadopp( hydroAuxFD ) < 0 )
      {
        LOGPRINT( LVL_ALRT, "profile(): Failed to stop aquadopp logging!" );
      }else {
        LOGPRINT( LVL_NOTC, "profile(): Stopping Aquadopp Logging..." );
      }
    }

    //
    // WAIT FOR SENSORS TO EQUILIBRATE
    //
    sleepSec = missn->equilibrationTime;
    LOGPRINT( LVL_INFO,
              "profile(): Sleeping for %d seconds sensor equilibrate.", 
              sleepSec );
    while ( ( sleepSec = sleep( sleepSec ) ) > 0 ) { /*nothing*/ }

    //
    // Check winch voltage
    //
    if ( extbatt < MINEXTVOLT ) 
    {
      LOGPRINT( LVL_ALRT, "profile(): Exiting because battery voltage is too low! ( intVolt = %4.1fv extVolt = %4.1fv", intbatt, extbatt );
      LOGPRINT( LVL_INFO, "profile(): Stopping CTD logging..." );
      if ( ( ret = stopHydroLogging( hydroDeviceType, hydroFD ) ) < 0 )
      {
        // For now consider this a warning not a failure.
        LOGPRINT( LVL_ALRT, "profile(): Could not stop CTD from logging! "
                  " Function returned %d.", ret );
      }
      return( EPROF );
    } 

    // Done going to the surface
    cycles -= 0.25;

    //
    // MAIN CYLCE LOOP
    //
    while ( cycles > 0 ) 
    {

      // 
      // Start auxilary sampling if need be
      // 
      if (    hydroAuxFD >= 0
           && missn->auxSampleDirection == DOWN )
      {
        if ( startLoggingAquadopp( hydroAuxFD ) < 0 )
        {
          LOGPRINT( LVL_ALRT, "profile(): Failed to start aquadopp logging!" );
        }else {
          LOGPRINT( LVL_NOTC, "profile(): Started Aquadopp Logging..." );
        }
      }

      //
      // Prepare to move the package down 
      //

      // First determine which depth
      if ( cycles > 0.5 ) 
        tgtDepth = opts.maxDepth;
      else 
        tgtDepth = opts.parkingDepth;
      
      // Move the package down
      LOGPRINT( LVL_INFO, "profile(): Taking CTD down to %6.2f meters",
                tgtDepth );
      ret = movePackageDown( hydroFD, hydroDeviceType, mwPort, tgtDepth );
      if ( ret < 0 ) 
      {
        LOGPRINT( LVL_ALRT, "profile(): Failed to move package down "
                  "(status = %d).  The package may no longer be at "
                  "parking depth!", ret );
        LOGPRINT( LVL_INFO, "profile(): Stopping CTD logging..." );
        if ( ( ret = stopHydroLogging( hydroDeviceType, hydroFD ) ) < 0 )
        {
          // For now consider this a warning not a failure.
          LOGPRINT( LVL_ALRT, "profile(): Could not stop CTD from logging! "
                    " Function returned %d.", ret );
        }
        return( EPROF );
      }

      // 
      // Basic print status
      //
      if ( mwPort )
        meterWheelDepth = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor );
      intbatt = GET_INTERNAL_BATTERY_VOLTAGE;
      if ( intbatt < 0 )
        intbatt = GET_INTERNAL_BATTERY_VOLTAGE;
      extbatt = GET_EXTERNAL_BATTERY_VOLTAGE;
      if ( extbatt < 0 ) 
        extbatt = GET_EXTERNAL_BATTERY_VOLTAGE;
      pressure = getHydroPressure( hydroDeviceType, hydroFD );
      pressureDepth = convertDBToDepth( pressure );
      LOGPRINT( LVL_ALWY, 
                "profile(): pres=%6.2fdb prdp=%6.2fm mwdp=%6.1f "
                "ipwr=%4.1fv epwr=%4.1fv", pressure, pressureDepth,
                meterWheelDepth, intbatt, extbatt );

      // 
      // Stop the auxilary sampling if need be
      //
      if (    hydroAuxFD >= 0
           && missn->auxSampleDirection == DOWN  )
      {
        if ( stopLoggingAquadopp( hydroAuxFD ) < 0 )
        {
          LOGPRINT( LVL_ALRT, "profile(): Failed to stop aquadopp logging!" );
        }else {
          LOGPRINT( LVL_NOTC, "profile(): Stopping Aquadopp Logging..." );
        }
      }

      //
      // WAIT FOR SENSORS TO EQUILIBRATE
      //
      sleepSec = missn->equilibrationTime;
      LOGPRINT( LVL_INFO,
                "profile(): Sleeping for %d seconds sensor equilibrate.", 
                sleepSec );
      while ( ( sleepSec = sleep( sleepSec ) ) > 0 ) { /*nothing*/ }

      //
      // Check winch voltage
      //
      if ( extbatt < MINEXTVOLT ) 
      {
        LOGPRINT( LVL_ALRT, "profile(): Exiting because battery voltage is too low! ( intVolt = %4.1fv extVolt = %4.1fv", intbatt, extbatt );
        LOGPRINT( LVL_INFO, "profile(): Stopping CTD logging..." );
        if ( ( ret = stopHydroLogging( hydroDeviceType, hydroFD ) ) < 0 )
        {
          // For now consider this a warning not a failure.
          LOGPRINT( LVL_ALRT, "profile(): Could not stop CTD from logging! "
                    " Function returned %d.", ret );
        }
        return( EPROF );
      } 

      // 
      // Start auxilary sampling if need be
      // 
      if (    hydroAuxFD >= 0
           && missn->auxSampleDirection == UP )
      {
        if ( startLoggingAquadopp( hydroAuxFD ) < 0 )
        {
          LOGPRINT( LVL_ALRT, "profile(): Failed to start aquadopp logging!" );
        }else {
          LOGPRINT( LVL_NOTC, "profile(): Started Aquadopp Logging..." );
        }
      }

      // Decrement cycles if we were successful
      if ( cycles > 0.5 ) 
        cycles -= 0.5;
      else 
        cycles -= 0.25;

      //
      // Prepare to take the package up
      //
      if ( cycles > 0 ) {
        // First determine which depth
        if ( cycles > 0.25 )
          tgtDepth = opts.minDepth;
        else 
          tgtDepth = opts.parkingDepth;

        // Move the package up ( use discrete sampling )
        LOGPRINT( LVL_INFO,
                  "profile(): Moving package up discretely to %6.2f meters", 
                  tgtDepth );
  
        ret = movePackageUpDiscretely( hydroFD, hydroDeviceType, mwPort, tgtDepth,
                                       missn->depths, missn->numDepths, 
                                       missn->equilibrationTime );
        if ( ret < 0 )
        {
          LOGPRINT( LVL_ALRT, "profile(): Failed to move package up "
                    "(status = %d).  The package may no longer be at "
                    "parking depth!", ret );
          LOGPRINT( LVL_INFO, "profile(): Stopping CTD logging..." );
          if ( ( ret = stopHydroLogging( hydroDeviceType, hydroFD ) ) < 0 )
          {
            // For now consider this a warning not a failure.
            LOGPRINT( LVL_ALRT, "profile(): Could not stop CTD from logging! "
                      " Function returned %d.", ret );
          }
          return( EPROF );
        }

        // 
        // Basic print status
        //
        if ( mwPort )
          meterWheelDepth = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor );
        intbatt = GET_INTERNAL_BATTERY_VOLTAGE;
        if ( intbatt < 0 )
          intbatt = GET_INTERNAL_BATTERY_VOLTAGE;
        extbatt = GET_EXTERNAL_BATTERY_VOLTAGE;
        if ( extbatt < 0 )
          extbatt = GET_EXTERNAL_BATTERY_VOLTAGE;
        pressure = getHydroPressure( hydroDeviceType, hydroFD );
        pressureDepth = convertDBToDepth( pressure );
        LOGPRINT( LVL_ALWY, 
                  "profile(): pres=%6.2fdb prdp=%6.2fm mwdp=%6.1f "
                  "ipwr=%4.1fv epwr=%4.1fv", pressure, pressureDepth,
                  meterWheelDepth, intbatt, extbatt );

        // Decrement cycles if we were successful
        if ( cycles > 0.25 ) 
          cycles -= 0.5;
        else 
          cycles -= 0.25;

        // 
        // Stop the auxilary sampling if need be
        //
        if (    hydroAuxFD >= 0
             && missn->auxSampleDirection == UP )
        {
          if ( stopLoggingAquadopp( hydroAuxFD ) < 0 )
          {
            LOGPRINT( LVL_ALRT, "profile(): Failed to stop aquadopp logging!" );
          }else {
            LOGPRINT( LVL_NOTC, "profile(): Stopping Aquadopp Logging..." );
          }
        }

      } // if ( cycles > 0 )

    } // while ( cycles > 0 )  ...main cycle loop

  } // if ( cycles > 0 ) 

  // 
  //  STOP CTD SAMPLING
  // 
  LOGPRINT( LVL_INFO, "profile(): Stopping CTD logging..." );
  if ( ( ret = stopHydroLogging( hydroDeviceType, hydroFD ) ) < 0 )
  {
    // For now consider this a warning not a failure.
    LOGPRINT( LVL_ALRT, "profile(): Could not stop CTD from logging! "
              " Function returned %d.  Exiting...", ret );
  }

  // 
  // Stop the auxilary sampling if need be
  //
  if (    hydroAuxFD >= 0
       && missn->auxSampleDirection == BOTH  )
  {
    if ( stopLoggingAquadopp( hydroAuxFD ) < 0 )
    {
      LOGPRINT( LVL_ALRT, "profile(): Failed to stop aquadopp logging!" );
    }else {
      LOGPRINT( LVL_NOTC, "profile(): Stopping Aquadopp Logging..." );
    }
  }

  return ( SUCCESS );

} // profile()
