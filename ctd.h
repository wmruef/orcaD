/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * ctd.h: Header for CTD functions 
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
#ifndef _CTD_H
#define _CTD_H 

//
// The length of the input buffer for CTD communications
//
#define CTDBUFFLEN 256
#define TIMESTRMAX 16

//
// Prototypes
//

// testCTDCommunication - Test the communication integrity of the CTD
float testCTDCommunication( int hydroDeviceType, int hydroFD );

//   convertDBToDepth - Convert seawater pressure into depth
double convertDBToDepth( double db );

//   getCTD19SPrompt - Obtain an "S>" prompt from a Seabird CTD 19
int getCTD19SPrompt( int ctdFD );

//   stopLoggingCTD19 - Cease CTD logging and pressure output
int stopLoggingCTD19( int ctdFD );

//   startLoggingCTD19 - Initiate CTD logging and pressure output
int startLoggingCTD19( int ctdFD );

//   getCTDPressure - Read a CTD line in logging mode and extract the pressure
double getCTD19Pressure( int ctdFD );

//   downloadCTD19Data - Download CTD 19 historical data to a file
int downloadCTD19Data( int ctdFD, FILE * outFile );

//   getCTD19Time - Get the real time clock time from the CTD 19
struct tm *getCTD19Time ( int ctdFD );

//   setCTD19Time - Set the real time clock time on the CTD 19
int setCTD19Time ( int ctdFD, struct tm *time );



//   htoP - Convert CTD 4 Byte Hex and convert to 14bit signed number
void htoP( char str[], int startc, int endc, double *P);

//////////////////////////CTD 19+ FUNCTIONS/////////////////////////

//   initCTD19Plus - Initialize the CTD for general operations
int initCTD19Plus( int ctdFD );

//   getCTD19PlusSPrompt - Obtain an "S>" prompt from a Seabird CTD 19+
int getCTD19PlusSPrompt( int ctdFD );

//   stopLoggingCTD19Plus - Cease CTD logging and pressure output
int stopLoggingCTD19Plus( int ctdFD );

//   startLoggingCTD19Plus - Initiate CTD logging and pressure output
int startLoggingCTD19Plus( int ctdFD );

//   getCTD19PlusPressure - Read a CTD line and extract the pressure
double getCTD19PlusPressure( int ctdFD );

//   downloadCTD19PlusData - Download CTD 19+ historical data to a file
int downloadCTD19PlusData( int ctdFD, FILE * outFile );

//   getCTD19PlusTime - Get the real time clock time from the CTD 19+
struct tm *getCTD19PlusTime ( int ctdFD );

//   setCTD19PlusTime - Set the real time clock time on the CTD 19+
int setCTD19PlusTime ( int ctdFD, struct tm *time );

#endif
