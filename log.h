/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * log.h : Header for logging functions
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
#ifndef _LOG_H
#define _LOG_H


// Macro to make it easy to switch 
// the function it is calling
#define LOGPRINT logPrint

// Loging levels
#define LVL_ALWY 0  // I have something really really important to say
#define LVL_EMRG 0  // Really really bad...or you have something 
#define LVL_CRIT 1  // Action must be taken immediately
#define LVL_ALRT 2  // Something is definately wrong
#define LVL_WARN 3  // Probably just a transient problem
#define LVL_NOTC 4  // Important status information
#define LVL_INFO 5  // More detailed status information
#define LVL_VERB 6  // i.e - Going in and out of functions 
                    //     - Wakeup and sleeping in main loop
#define LVL_DEBG 7  // Gory details
                    // i.e - anything inside a subroutine

// Destination splitting
#define LOG_FILE_ONLY 0
#define LOG_SCREEN_AND_FILE 1

extern struct optionsStruct opts;

FILE *logFile;  // The global variable for the logFile
int  logDest;   // The global variable which holds destination splits
char *progName; // The name of program calling the log functions

// Function prototypes
int startLogging( char * fileName, char * pName, int dest );
void logPrint( int level, char *message, ... );
int logRotate( char *fileName );
int rotateLargeLog( char *fileName, int size );


#endif
