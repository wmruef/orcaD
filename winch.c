/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * winch.c : Winch control routines  
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
 * Perhaps we can protect the winch by placing a max timeout
 * in these routines based on a given winch speed and distance
 * to travel.
 *
 * TODO: Make equlibration time a parameter!
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "general.h" 
#include "orcad.h"
#include "hardio.h"
#include "ctd.h"
#include "hydro.h"
#include "log.h"
#include "meterwheel.h"
#include "buoy.h"
#include "winch.h"


//
// NAME
//   movePackageUpDiscretely - Move package up stoping at discrete levels.
//
// SYNOPSIS
//   #include "winch.h"
//
//   int movePackageUpDiscretely( int hydroFD, int hydroDeviceType,
//                                struct sPort *mwPort, int tgtDepth,
//                                int *discreteDepths, int numDiscreteDepths );
//
// DESCRIPTION
//   Move the package up stoping at discrete levels defined by the
//   discreteDepths array.
//
// RETURNS
//
//     ESUCC   Success!
//     EUNKN   A general unclassified error
//     EPROP   According to the pressure the winch is moving oppositely
//     ECTOP   According to the counter the winch is moving oppositely
//     ECTST   According to the counter the winch is not moving
//     EPRES   Error obtaining pressure data
//     ECOUN   Error obtaining counter data
//
int movePackageUpDiscretely ( int hydroFD, int hydroDeviceType,
                              struct sPort *mwPort, int tgtDepth,
                              int *discreteDepths, int numDiscreteDepths,
                              int equilibrationTime )
{
  int j;
  int retValue; 
  float meterWheelDepth = 0;
  float intbatt, extbatt;
  double pressure, pressureDepth;


  // This assumes that the depths are in high
  //   to low order in the array.
  // TODO:....have a check for this!
  for( j=0; j<numDiscreteDepths; j++ )
  {
    // Don't go too far
    if ( discreteDepths[ j ] < tgtDepth ) 
    {
      break;
    }

    // Move the package up
    if ( ( retValue = 
              movePackageUp( hydroFD, hydroDeviceType,
                             mwPort, discreteDepths[ j ] ) ) < 0 ) 
    {
      // Some sort of error occured!
      return( retValue );
    }

    //
    // Get Status
    //
    pressure = getHydroPressure( hydroDeviceType, hydroFD );
    pressureDepth = convertDBToDepth( pressure );
    if ( mwPort )
      meterWheelDepth = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor );
    intbatt = GET_INTERNAL_BATTERY_VOLTAGE;
    extbatt = GET_EXTERNAL_BATTERY_VOLTAGE;

    // Normal status print
    LOGPRINT( LVL_ALWY,
            "movePackageUpDiscretely(): pres=%6.2fdb prdp=%6.2fm mwdp=%6.1f "
            "ipwr=%4.1fv epwr=%4.1fv", pressure, pressureDepth,
            meterWheelDepth, intbatt, extbatt );

   
    //
    // WAIT FOR # SECONDS FOR SENSORS TO EQUILIBRATE
    //
    LOGPRINT( LVL_INFO,
              "movePackageUpDiscretely(): Sleeping for %d seconds for sensor "
              "equilibration.", equilibrationTime );
    sleep( equilibrationTime );
    LOGPRINT( LVL_CRIT,
              "movePackageUpDiscretely(): Meter Wheel Adjusted Count = %f "
              "meters", meterWheelDepth );
    LOGPRINT( LVL_CRIT,
              "movePackageUpDiscretely(): Water Pressure = %6.2f db = "
              "%6.2f meters", pressure, pressureDepth );

    //
    //  GET BATTERY STATUS
    //
    if ( extbatt < 20 ) 
    { 
        // Signal that the battery was too low to continue
        return( EBATT );
    }

  }

  // If we are below tgtDepth move the rest of the way up
  if ( ( retValue = 
            movePackageUp( hydroFD, hydroDeviceType, mwPort, tgtDepth ) ) < 0 ) 
  {
    // Some sort of error occured
    return( retValue );
  }

  return( 1 );
}


//
// NAME
//   movePackageUp - Run the winch to move the instrument package up
//
// SYNOPSIS
//   #include "winch.h"
//
//   int movePackageUp( int hydroFD, int hydroDeviceType, 
//                      struct sPort *mwPort, int tgtDepth );
//
// DESCRIPTION
//  Move the package up to tgtDepth using a pressure and
//  meter wheel counter as a guide.  This routine should
//  be protected from signal interrupts!  Should this
//  be interrupted while the winch is turned on you may
//  crash the package into the buoy!!!!
//
//    hydroFD: An open file descriptor to a hydrowire
//             device which is **currently** producing 
//             pressure data. 
//
//    mwfd:    An open file descriptor to a meter wheel
//             counter.
//
//    tgtDepth: The depth which you would like to move
//              the package to.
//
// RETURNS:
//    
//     ESUCC   Success!
//     EUNKN   A general unclassified error
//     EPROP   According to the pressure the winch is moving oppositely
//     ECTOP   According to the counter the winch is moving oppositely
//     ECTST   According to the counter the winch is not moving
//     EPRES   Error obtaining pressure data
//     ECOUN   Error obtaining counter data
//
int movePackageUp ( int hydroFD, int hydroDeviceType, struct sPort *mwPort, int tgtDepth )
{
  int i = 0, status = 1;
  float mcnt4 = 0, mcnt3 = 0, mcnt2 = 0, mcnt1 = 0;
  float meterWheelDepth = 0;
  float tmpVolts = 0;
  double pressure = 0, pressureDepth = 0;
  double Pm4 = 0, Pm3 = 0, Pm2 = 0, Pm1 = 0;
  int mCntStatic = 0;
  int mPresStatic = 0;


  LOGPRINT( LVL_VERB, 
            "movePackageUp(): Called. Attempting to move package to %d meters", 
            tgtDepth );

  //
  // Take an initial pressure reading
  //
  if ( ( pressure = getHydroPressure( hydroDeviceType, hydroFD ) ) < 0 )
  { 
    LOGPRINT( LVL_ALRT, "movePackageUp(): Could not obtain pressure data!" );
    return( EPRES );
  }
  pressureDepth = convertDBToDepth( pressure );
  if ( mwPort && ( meterWheelDepth = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor ) ) < 0 )
  {
    LOGPRINT( LVL_ALRT, "movePackageUp(): Could not obtain counter data!" );
    return( ECOUN );
  }
  LOGPRINT( LVL_DEBG, 
            "movePackageUp(): Starting: Meter Wheel Depth = %6.2f meters",
            meterWheelDepth );
  LOGPRINT( LVL_DEBG, 
        "movePackageUp(): Starting: Water Pressure = %6.2f db = %6.2f meters", 
        pressure, pressureDepth );


  // Make sure package isn't already too shallow 
  // and that we aren't trying to move by some very small amount
  if( tgtDepth < pressureDepth && ((pressureDepth - tgtDepth) > 0.2 ) )
  {

    // make sure we don't go too shallow. 
    if( tgtDepth < opts.minDepth ) tgtDepth = opts.minDepth;

    LOGPRINT( LVL_DEBG, "movePackageUp(): Entering critical loop!" );

    //*******************************************************************
    //          C R I T I C A L   S E C T I O N   S T A R T
    //
    // Do not put anything in this section which may block!!!
    //   If we block we could crash the package into the buoy!!!
    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    opts.inCritical = 1;
    WINCH_UP;
    WINCH_ON;

    if ( ( pressure = getHydroPressure( hydroDeviceType, hydroFD ) ) < 0 )
    {
      // Something probably went wrong.  Leave unhappy
      status = EPRES; goto endroutine;
    }
    pressureDepth = convertDBToDepth( pressure );

    // resolution of 1m
    i=0;
    while( ( pressureDepth - tgtDepth ) > 1 ) 
    {
      Pm4 = Pm3; Pm3 = Pm2; Pm2 = Pm1; Pm1 = pressureDepth;
      mcnt4 = mcnt3; mcnt3 = mcnt2; mcnt2 = mcnt1; mcnt1 = meterWheelDepth;
      if ( ( pressure = getHydroPressure( hydroDeviceType, hydroFD ) ) < 0 )
      {
        // Something probably went wrong.  Leave unhappy
        status = EPRES; goto endroutine;
      }
      pressureDepth = convertDBToDepth( pressure );
      if ( mwPort && ( meterWheelDepth = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor ) ) < 0 ) 
      {
        // Something probably went wrong.  Leave unhappy
        status = ECOUN; goto endroutine;
      }

      // Scan the voltage
      tmpVolts = GET_EXTERNAL_BATTERY_VOLTAGE;

      // NOTE: This is a bit dangerous.  There is a potential that
      //       should this I/O write be delayed....we will not
      //       shut off the winch before the package crashes.
      LOGPRINT( LVL_VERB, "movePackageUp(): critical loop pressureDepth = "
                          "%f, meterWheelDepth = %f, extVolts = %f;", 
                          pressureDepth, meterWheelDepth, tmpVolts );
 
      // Check winch direction 
      if( i > 4 )
      {
        if( Pm4 < Pm3 && Pm3 < Pm2 && 
            Pm2 < Pm1 && Pm1 < pressureDepth )
        {
          // ERROR: According to the presure the winch is going down
          status = EPROP; goto endroutine;
        }
        if( mwPort && 
            mcnt4 < mcnt3 && mcnt3 < mcnt2 && 
            mcnt2 < mcnt1 && mcnt1 < meterWheelDepth ) 
        {
          // ERROR: According to the counter the winch is going down
          status = ECTOP; goto endroutine;
        }
      }

      // Check winch movement
      if ( mwPort && i > 0 && mcnt1 == meterWheelDepth ) 
      {
        if ( ++mCntStatic == 4 )
        {
          // WARNING: According to the counter the winch is stopped
          status = WCTST;
        } 
        if ( mCntStatic > 4 && ( Pm4 - pressureDepth ) < EPDELTA_STATICMW )
        {
          // ERROR: According to the counter/pressure the winch is stopped
          status = ECTST;
          goto endroutine;
        }
      }
      else 
      {
        mCntStatic = 0;
      }

      if ( i > 4 && ( Pm4 - pressureDepth ) < EPDELTA_STATICMW ) 
      {
        if ( ++mPresStatic > 4 ) 
        {
          // ERROR: According to the pressure the winch is stopped
          status = EPRST;
          goto endroutine;
        }
      }
      else 
      {
        mPresStatic = 0;
      }

      i++;
    } // while( ( pressureDepth - tgtDepth ) > 1 )

    endroutine:
    if ( WINCH_OFF < 0 )
      if ( WINCH_OFF < 0 )
        if ( WINCH_OFF < 0 )
        {
           LOGPRINT( LVL_EMRG, 
                     "movePackageUp(): Possibly couldn't shut off the winch!" );
        }
    WINCH_DOWN;  // DOWN = IO power low
    opts.inCritical = 0;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    //          C R I T I C A L   S E C T I O N   E N D
    //*******************************************************************
    LOGPRINT( LVL_DEBG, 
              "movePackageUp(): Exited critical loop, status = %d, "
              "pressureDepth = %6.2f, tgtDepth = %d", status, pressureDepth,
              tgtDepth );
 
  } // if( tgtDepth < pressureDepth &&...

  // Report more info at high debug or on strange behaviour
  if ( status < 0 || abs( Pm1 - pressureDepth ) > 2  ) 
  {
     if ( abs( Pm1 - pressureDepth ) > 2 ) 
     {
       LOGPRINT( LVL_CRIT, "movePackageUp(): WARNING: The pressure jumped  " 
                 "by an improbable amount while moving the winch: %f " 
                 "meter change", ( Pm1 - pressureDepth ) );
     }

     // Decypher error status
     if ( status == EUNKN ) 
       LOGPRINT( LVL_CRIT, "movePackageUp(): Status = EUNKN; Unclassified " 
                           "error occured!" ); 
     else if ( status == EPROP )
       LOGPRINT( LVL_CRIT, "movePackageUp(): Status = EPROP; According " 
                           "to the pressure the winch is moving oppositely!" ); 
     else if ( status == ECTOP )
       LOGPRINT( LVL_CRIT, "movePackageUp(): Status = ECTOP; According " 
                           "to the counter the winch is moving oppositely!" ); 
     else if ( status == ECTST )
       LOGPRINT( LVL_CRIT, "movePackageUp(): Status = ECTST; According " 
                           "to the counter the winch is not moving!" ); 
     else if ( status == EPRES )
       LOGPRINT( LVL_CRIT, "movePackageUp(): Status = EPRES; Error " 
                           "obtaining pressure data!" ); 
     else if ( status == ECOUN )
       LOGPRINT( LVL_CRIT, "movePackageUp(): Status = ECOUN; Error " 
                           "obtaining counter data!" ); 
     else if ( status == EPRST )
       LOGPRINT( LVL_CRIT, "movePackageUp(): Status = EPRST; According " 
                           "to the CTD the winch isn't moving! "
                           " ( CTD Pressure Threshold = %f, " 
                           "actual change = %f )",
                           EPDELTA_STATICMW, ( Pm4 - pressureDepth ) ); 
     else if ( status == WCTST )
     {
       LOGPRINT( LVL_CRIT, "movePackageUp(): Status = WCTST; Warning " 
                           "the counter wheel is sticking!" ); 
       // No need to upset other's about this type of problem.
       status = SUCCESS;
     }
     LOGPRINT( LVL_CRIT, "movePackageUp(): pressureDepth = %f; "
                         "Pm1(recent)-4(older) = " 
               "%f, %f, %f, %f", pressureDepth, Pm1, Pm2, Pm3, Pm4 );
     LOGPRINT( LVL_CRIT, "movePackageUp(): meterWheelDepth = %f; mcnt1-4 = "
               "%f, %f, %f, %f", meterWheelDepth, mcnt1, mcnt2, mcnt3, mcnt4 );
     LOGPRINT( LVL_CRIT, "movePackageUp(): loop counts ( i ) = %d ", i );
  }else {
     LOGPRINT( LVL_DEBG, "movePackageUp(): Status = %d" );
     LOGPRINT( LVL_CRIT, "movePackageUp(): pressureDepth = %f; "
                         "Pm1(recent)-4(older) = " 
               "%f, %f, %f, %f", pressureDepth, Pm1, Pm2, Pm3, Pm4 );
     LOGPRINT( LVL_DEBG, "movePackageUp(): meterWheelDepth = %f; mcnt1-4 = "
               "%f, %f, %f, %f", meterWheelDepth, mcnt1, mcnt2, mcnt3, mcnt4 );
  }

  //
  // Take final pressure/meter wheel reading
  //
  pressure = getHydroPressure( hydroDeviceType, hydroFD );
  pressureDepth = convertDBToDepth( pressure );
  if ( mwPort )
    meterWheelDepth = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor );
  
  LOGPRINT( LVL_VERB, 
            "movePackageUp(): Ending: Meter Wheel Depth = %6.2f meters",
            meterWheelDepth );
  LOGPRINT( LVL_VERB, 
          "movePackageUp(): Ending: Water Pressure = %6.2f db = %6.2f meters", 
          pressure, pressureDepth );

  return( status );
}


//
// NAME
//   movePackageDown - Run the winch to move the instrument package down
//
// SYNOPSIS
//   #include "winch.h"
//
//   int movePackageDown( int hydroFD, int hydroDeviceType, 
//                        struct sPort *mwPort, int tgtDepth );
//
// DESCRIPTION
//  Move the package down to tgtDepth using a pressure and
//  meter wheel counter as a guide.  This routine should
//  be protected from signal interrupts!  Should this
//  be interrupted while the winch is turned on you may
//  crash the package into the buoy!!!!
//
//    hydroFD: An open file descriptor to a hydrowire
//             device which is **currently** producing 
//             pressure data. 
//
//    mwfd:    An open file descriptor to a meter wheel
//             counter.
//
//    tgtDepth: The depth which you would like to move
//              the package to.
//
// RETURNS
//
//     ESUCC   Success!
//     EUNKN   A general unclassified error
//     EPROP   According to the pressure the winch is moving oppositely
//     ECTOP   According to the counter the winch is moving oppositely
//     ECTST   According to the counter the winch is not moving
//     EPRES   Error obtaining pressure data
//     ECOUN   Error obtaining counter data
//
int movePackageDown ( int hydroFD, int hydroDeviceType, struct sPort *mwPort, int tgtDepth )
{
  int i = 0, status = 1;
  float mcnt4 = 0, mcnt3 = 0, mcnt2 = 0, mcnt1 = 0;
  float meterWheelDepth = 0;
  float tmpVolts = 0;
  double pressure = 0, pressureDepth = 0;
  double Pm4 = 0, Pm3 = 0, Pm2 = 0, Pm1 = 0;
  int mCntStatic = 0;
  int mPresStatic = 0;

  LOGPRINT( LVL_VERB, 
          "movePackageDown(): Called attempting to move package to %d meters", 
          tgtDepth );

  //
  // Take an initial pressure reading
  //
  if ( ( pressure = getHydroPressure( hydroDeviceType, hydroFD ) ) < 0 )
  {
    LOGPRINT( LVL_ALRT, "movePackageDown(): Could not obtain pressure data!" );
    return( EPRES );
  }
  pressureDepth = convertDBToDepth( pressure );
  if ( mwPort && ( meterWheelDepth = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor ) ) < 0 )
  {
    LOGPRINT( LVL_ALRT, "movePackageDown(): Could not obtain counter data!" );
    return( ECOUN );
  }
  LOGPRINT( LVL_DEBG, 
            "movePackageDown(): Starting: Meter Wheel Depth = %6.2f meters",
            meterWheelDepth );
  LOGPRINT( LVL_DEBG, 
      "movePackageDown(): Starting: Water Pressure = %6.2f db = %6.2f meters", 
       pressure, pressureDepth );

  // Make sure package isn't already too deep
  // and that we aren't trying to move by some very small amount
  if( pressureDepth < tgtDepth && ((tgtDepth - pressureDepth) > 0.2 ) )
  {

    // make sure we don't go too deep. 
    if( tgtDepth > opts.maxDepth ) tgtDepth = opts.maxDepth;

    LOGPRINT( LVL_DEBG, "movePackageDown(): Entering critical loop!" );

    //*******************************************************************
    //          C R I T I C A L   S E C T I O N   S T A R T
    //
    // Do not put anything in this section which may block!!!
    //   If we block we could crash the package into the buoy!!!
    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    opts.inCritical = 1;
    WINCH_DOWN;
    WINCH_ON;

    if ( ( pressure = getHydroPressure( hydroDeviceType, hydroFD ) ) < 0 )
    {
      // Something probably went wrong.  Leave unhappy
      status = EPRES; goto endroutine;
    }
    pressureDepth = convertDBToDepth( pressure );

    // resolution of 1m
    i=0;
    while( ( tgtDepth - pressureDepth ) > 1 ) 
    {
      Pm4 = Pm3; Pm3 = Pm2; Pm2 = Pm1; Pm1 = pressureDepth;
      mcnt4 = mcnt3; mcnt3 = mcnt2; mcnt2 = mcnt1; mcnt1 = meterWheelDepth;
      if ( ( pressure = getHydroPressure( hydroDeviceType, hydroFD ) ) < 0 )
      {
        // Something probably went wrong.  Leave unhappy
        status = EPRES; goto endroutine;
      }
      pressureDepth = convertDBToDepth( pressure );
      if ( mwPort && ( meterWheelDepth = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor ) ) < 0 )
      {
        // Something probably went wrong.  Leave unhappy
        status = ECOUN; goto endroutine;
      }

      // Scan the voltage
      tmpVolts = GET_EXTERNAL_BATTERY_VOLTAGE;

      // Ok...this is a bit dangerous...but since we are only moving
      // the winch down....lets give it a shot.
      LOGPRINT( LVL_VERB, "movePackageDown(): critical loop pressureDepth = "
                          "%f, meterWheelDepth = %f, extVolts = %f;", 
                          pressureDepth, meterWheelDepth, tmpVolts );
 
      // Check winch direction 
      if( i > 4 )
      {
        if( Pm4 > Pm3 && Pm3 > Pm2 && 
            Pm2 > Pm1 && Pm1 > pressureDepth ) 
        {
          // ERROR: According to the pressure, winch appears to be going up 
          status = EPROP; goto endroutine;
        }
        if( mwPort &&
            mcnt4 > mcnt3 && mcnt3 > mcnt2 && 
            mcnt2 > mcnt1 && mcnt1 > meterWheelDepth )
        {
          // ERROR: According to the meterwheel, winch appears to be going up 
          status = ECTOP; goto endroutine;
        }
      }


      // Check winch movement
      if ( mwPort && i > 0 && mcnt1 == meterWheelDepth ) 
      {
        if ( ++mCntStatic == 4 )
        {
          // WARNING: According to the counter the winch is stopped
          status = WCTST;
        } 
        if ( mCntStatic > 4 && ( pressureDepth - Pm4 ) < EPDELTA_STATICMW )
        {
          // ERROR: According to the counter/pressure the winch is stopped
          status = ECTST;
          goto endroutine;
        }
      }
      else 
      {
        mCntStatic = 0;
      }

      if ( i > 4 && ( pressureDepth - Pm4 ) < EPDELTA_STATICMW ) 
      {
        if ( ++mPresStatic > 4 ) 
        {
          // ERROR: According to the pressure the winch is stopped
          status = EPRST;
          goto endroutine;
        }
      }
      else 
      {
        mPresStatic = 0;
      }

      i++;
    } // while( ( tgtDepth - pressureDepth ) > 1 )

    endroutine:
    if ( WINCH_OFF < 0 )
      if ( WINCH_OFF < 0 )
        if ( WINCH_OFF < 0 )
        {
          LOGPRINT( LVL_EMRG, 
                    "movePackageDown(): Possibly couldn't shut "
                    "off the winch!" );
        }
    WINCH_DOWN; // Winch down = io power low
    opts.inCritical = 0;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    //          C R I T I C A L   S E C T I O N   E N D
    //*******************************************************************
    LOGPRINT( LVL_DEBG, 
              "movePackageDown(): Exited critical loop, status = %d, "
              "pressureDepth = %6.2f, tgtDepth = %d", status, pressureDepth,
              tgtDepth );


  } // if( tgtDepth < pressureDepth &&...

  // Report more info at high debug or on strange behaviour
  if ( status < 0 || abs( pressureDepth - Pm1 ) > 2  ) 
  {
     if ( abs( pressureDepth - Pm1 ) > 2 ) 
     {
       LOGPRINT( LVL_CRIT, "movePackageDown(): WARNING: The pressure jumped  " 
                 "by an improbable amount while moving the winch: %f " 
                 "meter change", (pressureDepth - Pm1) );
     }

     // Decypher error status
     if ( status == EUNKN ) 
       LOGPRINT( LVL_CRIT, "movePackageDown(): Status = EUNKN; Unclassified " 
                           "error occured!" ); 
     else if ( status == EPROP )
       LOGPRINT( LVL_CRIT, "movePackageDown(): Status = EPROP; According " 
                           "to the pressure the winch is moving oppositely!" ); 
     else if ( status == ECTOP )
       LOGPRINT( LVL_CRIT, "movePackageDown(): Status = ECTOP; According " 
                           "to the counter the winch is moving oppositely!" ); 
     else if ( status == ECTST )
       LOGPRINT( LVL_CRIT, "movePackageDown(): Status = ECTST; According " 
                           "to the counter the winch is not moving!" ); 
     else if ( status == EPRES )
       LOGPRINT( LVL_CRIT, "movePackageDown(): Status = EPRES; Error " 
                           "obtaining pressure data!" ); 
     else if ( status == ECOUN )
       LOGPRINT( LVL_CRIT, "movePackageDown(): Status = ECOUN; Error " 
                           "obtaining counter data!" ); 
     else if ( status == EPRST )
       LOGPRINT( LVL_CRIT, "movePackageDown(): Status = EPRST; According "
                           "to the CTD the winch isn't moving! "
                           " ( CTD Pressure Threshold = %f, " 
                           "actual change = %f )",
                           EPDELTA_STATICMW, ( pressureDepth - Pm4 ) );
     else if ( status == WCTST )
     {
       LOGPRINT( LVL_CRIT, "movePackageDown(): Status = WCTST; Warning " 
                           "the counter wheel is sticking!" ); 
       // No need to upset other's about this type of problem.
       status = SUCCESS;
     }
     LOGPRINT( LVL_CRIT, "movePackageDown(): pressureDepth = %f; "
                         "Pm1(recent)-4(older) = " 
               "%f, %f, %f, %f", pressureDepth, Pm1, Pm2, Pm3, Pm4 );
     LOGPRINT( LVL_CRIT, "movePackageDown(): meterWheelDepth = %f; mcnt1-4 = "
               "%f, %f, %f, %f", meterWheelDepth, mcnt1, mcnt2, mcnt3, mcnt4 );
     LOGPRINT( LVL_CRIT, "movePackageDown(): loop counts ( i ) = %d ", i );
  }else {
     LOGPRINT( LVL_DEBG, "movePackageDown(): Status = %d" );
     LOGPRINT( LVL_DEBG, "movePackageDown(): pressureDepth = %f; "
                         "Pm1(recent)-4(older) = " 
               "%f, %f, %f, %f", pressureDepth, Pm1, Pm2, Pm3, Pm4 );
     LOGPRINT( LVL_DEBG, "movePackageDown(): meterWheelDepth = %f; mcnt1-4 = "
               "%f, %f, %f, %f", meterWheelDepth, mcnt1, mcnt2, mcnt3, mcnt4 );
  }

  //
  // Take final pressure/meter wheel reading
  //
  pressure = getHydroPressure( hydroDeviceType, hydroFD );
  pressureDepth = convertDBToDepth( pressure );
  if ( mwPort )
    meterWheelDepth = readMeterWheelAdjusted( mwPort, opts.meterwheelCFactor );

  LOGPRINT( LVL_VERB, 
            "movePackageDown(): Ending: Meter Wheel Depth = %6.2f meters",
            meterWheelDepth );
  LOGPRINT( LVL_VERB, 
            "movePackageDown(): Ending: Water Pressure = %6.2f db = "
            "%6.2f meters", pressure, pressureDepth );

  return( status );
}


