/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * winch.h : Header for winch routines
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
#ifndef _WINCH_H
#define _WINCH_H


#define ESUCC  1  // Success
#define EUNKN -1  // A general unclassified error
#define EPROP -2  // According to the pressure the winch is moving oppositely
#define ECTOP -3  // According to the counter the winch is moving oppositely
#define ECTST -4  // According to the counter the winch is not moving
#define EPRES -5  // Error obtaining pressure data
#define ECOUN -6  // Error obtaining counter data
#define EBATT -7  // Error battery was too low to continue
#define WCTST -8  // Warning counter may be occasionally sticking
#define EPRST -9  // Error the pressure is static -- winch has stopped?

//
// Meterwheel threshold for change in winch routines
//
//  In case of sticking....we have sometimes used:
//     #define EPDELTA_STATICMW 0.3
//
// Normal is 0.5  
// Slow is 0.3
// Very Slow is 0.2
// Changed to 0.2 for TORCA ( very slow winch )
#define EPDELTA_STATICMW 0.25



int movePackageDown( int hydroFD, int hydroDeviceType, struct sPort * mwPort, int tgtDepth );
int movePackageUp( int hydroFD, int hydroDeviceType, struct sPort * mwPort, int tgtDepth );
int movePackageUpDiscretely( int hydroFD, int hydroDeviceType, 
                             struct sPort * mwPort, int tgtDepth,
                             int *discreteDepths, int numDiscreteDepths,
                             int equilibrationTime );

#endif
