/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * util.h : Header for the utility functions
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
#ifndef _UTIL_H
#define _UTIL_H 1


FILE * openNewWeatherFile();
FILE * openNewCastFile();
int updateDataDir();
int writeLastCastInfo( long castIdx );
long readLastCastInfo();
int createLockFile( char *lckFileName, char *prgName );


#endif


