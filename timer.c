/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * timer.c : Some basic timing routines  
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
#include <unistd.h>
#include <sys/time.h>


//
// NAME
//   getMilliSecSince - Function to retrieve the status of a timer.
//
// SYNOPSIS
//   #include "timer.h"
//
//   suseconds_t getMilliSecSince( struct timeval *startTime );
//
// DESCRIPTION
//   Given the start time as startTime calculate the difference
//   in milli seconds from that time to now.
//
// RETURNS
//   The number of milliseconds which have passed or -1 if
//   an error occurs.
//
suseconds_t getMilliSecSince( struct timeval *startTime ) {
  struct timeval timeNow;
  suseconds_t diff;

  if ( gettimeofday( &timeNow, NULL ) == 0 ) {
    diff = (( timeNow.tv_usec - startTime->tv_usec ) * (1.e-3)) + 
           (( timeNow.tv_sec - startTime->tv_sec ) * 1000);
    return ( diff );
  }else{
    return ( -1 );
  }

}

