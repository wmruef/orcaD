/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * profile.h : Header for profiling routines
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
#ifndef _PROFILE_H
#define _PROFILE_H

// Minimum external voltage allowed for a profile
//  - Typical minimum is 22.0 volts.
//  - Using 19.0 volts for a buoy which has a 
//    questionably low A/D measurement for extvolt.
#define MINEXTVOLT 19.0


// Return values for profile()
#define ESTUP -2  // Return value for errors during profile setup
#define EPROF -3  // Return value for errors during profiling

int profile( struct mission *missn );

#endif
