/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * ctd.c : CTD routines
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
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <time.h>
#include "general.h"
#include "ctd.h"
#include "log.h"
#include "orcad.h"
#include "serial.h"
#include "general.h"


// Useful info for time routines
static const char *const monthNamesList[] = {
  "Jan",
  "Feb",
  "Mar",
  "Apr",
  "May",
  "Jun",
  "Jul",
  "Aug",
  "Sep",
  "Oct",
  "Nov",
  "Dec",
  NULL
};


//
// Strings containing useful CTD constants
//    -  CTRL-Y CR
//    -  CTRL-Y CR LF
//    -  CTRL-Z CR
//    -  CTRL-Z CR LF
//
static char ctrlYStrCR[] = { 0x19, 0x0D, 0x00 };
static char ctrlYStrCRLF[] = { 0x19, 0x0D, 0x0A, 0x00 };


//
// NAME
//   testCTDCommunication - Test CTD communication integrity
//
// SYNOPSIS
//   #include "ctd.h"
//   
//   float testCTDCommunication( int hydroDeviceType, int hydroFD );
//
// DESCRIPTION
//   Determine the dataloss experienced while communicating with
//   a CTD over the hydrowire.  
//
//   Added extra verbosity as this is only used interactively in 
//   orcactrl.  Perhaps we should have a parameter for this.
//
// RETURN VALUE
//   The function returns the dataloss percent or -1.0 upon failure.
//
float testCTDCommunication ( int hydroDeviceType, int hydroFD ) 
{
  ssize_t ret;
  int i = 0;
  int j = 0;
  int numErr = 0;
  int numSent = 0;
  char responseBuf[128];
  char message[] = "The quick brown FOX jumped over the fence!";

  // Say hello
  LOGPRINT( LVL_ALWY, "Testing CTD Communication" );
  LOGPRINT( LVL_ALWY, "Port FD: hydroFD" );
  LOGPRINT( LVL_ALWY, "Test Phrase: %s", message );
  
  LOGPRINT( LVL_ALWY, "Attempting to obtain S> Prompt" );
  if ( hydroDeviceType == SEABIRD_CTD_19 )
    getCTD19SPrompt( hydroFD );
  else if ( hydroDeviceType == SEABIRD_CTD_19_PLUS )
    getCTD19PlusSPrompt( hydroFD );
  else
  { 
    LOGPRINT( LVL_DEBG, "testCTDCommunication(): Unknown hydro device "
              "type ( %d )", hydroDeviceType );
  }

  LOGPRINT( LVL_ALWY, "Starting Test..." );
 

  // Switch to printf for output at this point to reduce the system 
  // calls in this tight'ish loop.
  printf("   Attempt: >received message< ( return value )\n");

  for ( i = 0; i < 20; i++ )
  {
    // Flush SerialIN
    term_flush(hydroFD);

    // Send statement
    ret = write( hydroFD, message, strlen(message) );
    numSent += strlen(message);

    // Wait for response
    ret = serialGetLine( hydroFD, responseBuf, 128, 300L, "\r\n" );
    if ( ret >= 0 ) 
    {
      // 
      // DEBUG
      //
      printf("   %d: >%s< (%d)\n", i, responseBuf, ret );

      for ( j = 0; j < ret; j++ ) 
      {
        if ( message[j] != responseBuf[j] )
          numErr++;
      }

      if ( ret < strlen(message) )
        numErr += strlen(message) - ret;

      printf("   numSent = %d, numErr = %d\n", numSent, numErr );
    }else 
    {
      numErr += strlen(message);
    }
  }

  // Flush SerialIN
  term_flush(hydroFD);

  return( ( (float)numErr / (float)numSent ) * 100.0 );
}



//
// NAME
//   convertDBToDepth - Convert seawater pressure into depth
//
// SYNOPSIS
//   #include "ctd.h"
//
//   double convertDBToDepth( double db );
//
// DESCRIPTION
//   From the SEASOFT CTD Manual:
//
//    "For seawater depth, SEASOFT uses the formula in UNESCO 
//     technical papers in Marine Science No. 44. This is 
//     an empirical formula that takes compressibility 
//     (i.e., integrates density) into account. An ocean water
//     column at t = 0 and s = 35 is assumed.
//
//     The gravity variation with latitude is computed as:
//
//       gr = 9.780318 * (1.0 + (5.2788e-3 + 2.36e-5 * x) * x) + 
//            1.092e-6 * p;
//
//       where x = [sin (latitude / 57.29578)]^2
//       Then, depth d in meters is calculated from pressure p
//       in decibars:
//
//       seawater depth in meters = 
//             [(((-1.82e-15 * p + 2.279e-10) * 
//              p - 2.2512e-5) * p + 9.72659) * p] / gr
//
//   The input parameter db is the pressure from the CTD
//   in decibars.
//
// RETURNS
//   The depth in meters. 
//
double convertDBToDepth ( double db ) 
{
  double x, gr, depth = -1.0;

  x = pow( sin( BOUY_LATITUDE / 57.29578 ), 2.0 );
  gr = 9.780318 * ( 1.0 + ( 5.2788e-3 + 2.36e-5 * x ) * x ) +
       1.092e-6 * db;
  if ( gr > 0 )
    depth = ((((-1.82e-15 * db + 2.279e-10) * db - 2.2512e-5) * db + 9.72659) *
            db) / gr;
  return( depth );
}


//
// NAME
//   getCTD19SPrompt - Obtain an "S>" prompt from a Seabird CTD 19
//
// SYNOPSIS
//   #include "ctd.c"
//
//   int getCTD19SPrompt( int ctdFD );
//
// DESCRIPTION
//   The Seabird CTD 19 "S>" prompt is the main command
//   prompt.  Often the CTD is already waiting at an
//   "S>" prompt when this function is called.  Other times
//   the CTD may be in a data transfer mode ( such as "GL" )
//   or at a different baud rate than the program is expecting.
//   This function attempts to correct for both of these
//   cases.  The input parameter is an open file descriptor
//   to a Seabird CTD 19.
//
// RETURN VALUE
//   The function returns the baud rate of the CTD upon 
//   success and a -1 upon failure.
//
int getCTD19SPrompt ( int ctdFD )
{
  int bytesRead, i;
  char buffer[CTDBUFFLEN];
  int inDataStream = 0;

  // Say hello
  LOGPRINT( LVL_VERB, "getCTD19SPrompt(): Called" );

  //
  // See if we can talk to the CTD at 600 baud.
  //
  term_flush( ctdFD );
  if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
  {
    if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
    {
      if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
      {
        // All attempts at obtaining an S> prompt failed.
        // Could it be that we are getting something back but
        // don't know what it is???  Perhaps we are in logging
        // mode for some unknown reason?  Look at what the CTD
        // is saying and then decide what to do.
        serialGetLine( ctdFD, buffer, CTDBUFFLEN, 1000L, "\n" );
        if ( ( bytesRead = serialGetLine( ctdFD, buffer, 
                           CTDBUFFLEN, 1000L, "\n" ) ) == 26 )
        {
          // Let's see if we got a data line:
          //   79CA020C0000000000000E0E\r\n
          inDataStream = 1;
          for ( i = 0; i < 24; i++ )
          {
            if ( !(( buffer[ i ] >=48 && buffer[ i ] <= 57 ) ||
                   ( buffer[ i ] >=65 && buffer[ i ] <= 70 )) ) 
            {
              // Some strange data ...NOT hexadecimal ( capitals )
              inDataStream = 0;
              break;
            }
          }
          if ( inDataStream == 1 )
          {
            // Break out of data stream by sending CTRL-Z
            LOGPRINT( LVL_WARN, "getCTD19SPrompt(): Trying to break out of "
                      "data stream..." );
            serialPutByte( ctdFD, 0x1A );
            serialPutByte( ctdFD, '\r' );
            term_flush( ctdFD );
            if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
            {
              if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
              {
                if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
                {
                  if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
                  {
                    // Oh utter/udder failure!
                    // We are in the data stream but can't escape
                    return( FAILURE );
                  }
                }
              }
            }
            // We were sucessful!
            return( 600 );
          }
        } // if we could read something from the serial port

        // Attempting to change the baud rate to 1200 to
        // see if that is the problem.
        term_set_baudrate( ctdFD, 1200 );
        term_flush( ctdFD );
        if ( term_apply( ctdFD ) < 0 ) {
          LOGPRINT( LVL_WARN,
                    "getCTD19SPrompt(): Failed to change serial port "
                    "baud to 1200!!\n" );
          return( FAILURE );
        }
        term_flush( ctdFD );
        if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
        {
          if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
          {
            if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 ) 
            {
              if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 ) 
              {
                // Hmmm perhaps we have the right baud but we are
                // streaming data???
                serialGetLine( ctdFD, buffer, CTDBUFFLEN, 1000L, "\n" );
                if ( ( bytesRead = 
                          serialGetLine( ctdFD, buffer, CTDBUFFLEN, 
                                       1000L, "\n" ) ) == 26 )
                {
                  // Let's see if we got a data line:
                  //   79CA020C0000000000000E0E\r\n
                  inDataStream = 1;
                  for ( i = 0; i < 24; i++ )
                  {
                    if ( !(( buffer[ i ] >=48 && buffer[ i ] <= 57 ) ||
                           ( buffer[ i ] >=65 && buffer[ i ] <= 70 )) ) 
                    {
                      // Some strange data ...NOT hexadecimal ( capitals )
                      inDataStream = 0;
                      break;
                    }
                  }
                  if ( inDataStream == 1 )
                  {
                    // Break out of data stream by sending CTRL-Z
                    serialPutByte( ctdFD, 0x1A );
                    serialPutByte( ctdFD, '\r' );
                    term_flush( ctdFD );
                    if ( serialChat( ctdFD, "\r", "\r\nS>", 1000L, ">" ) < 1 )
                      if ( serialChat( ctdFD, "\r", "\r\nS>",
                                              1000L, ">" ) < 1 )
                        if ( serialChat( ctdFD, "\r", 
                                         "\r\nS>", 1000L, ">" ) < 1 )
                        {
                          // Oh utter/udder failure!
                          // We are in the data stream but can't escape
                          return( FAILURE );
                        }
                    // We were sucessful!
                    return( 1200 );
                  }
                }else {
                  return( FAILURE );
                } // if we could read something from the serial port
              }
            }
          } 
        }
        return( 1200 );
      }
    }
  }
  return ( 600 );
}


//
// NAME
//   stopLoggingCTD19 - Cease CTD logging and pressure output
//
// SYNOPSIS
//   #include "ctd.h"
//
//   int stopLoggingCTD19( int ctdFD );
//
// DESCRIPTION
//   Cease Seabird CTD 19 logging by sending a CTRL-Z
//   to the CTD.  NOTE: The use of CTRL-Z may be
//   specific to the UW's version of the CTD 19.
//
// RETURNS
//   Returns 1 for success and -1 for failure.  The
//   cause of failure is logged at level LVL_WARN.
//
int stopLoggingCTD19 ( int ctdFD )
{
  char buffer[CTDBUFFLEN];
  
  // Say hello
  LOGPRINT( LVL_VERB, "stopLoggingCTD19(): Called" );

  if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) != 26 )  
    if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) != 26 )  
      if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) != 26 )  
        if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) != 26 )  
        { 
          LOGPRINT( LVL_WARN, "stopLoggingCTD19(): CTD doesn't "
                    "appear to be sending data!" );
        }

  // TODO:....attempt this several times?
  serialPutByte( ctdFD, 0x1A );
  serialPutByte( ctdFD, '\r' );
  sleep( 1 ); 
  term_flush( ctdFD );
  if ( getCTD19SPrompt( ctdFD ) < 600 )  
  {
    LOGPRINT( LVL_WARN, "stopLoggingCTD19(): Could not "
              "get the S> prompt from the CTD" );
    return( FAILURE );
  }

  // Say goodbye
  LOGPRINT( LVL_VERB, "stopLoggingCTD19(): Returned: 1 - Success" );
  return( SUCCESS );
}


//
// NAME
//   startLoggingCTD19 - Initiate CTD logging and pressure output
//
// SYNOPSIS
//   #include "ctd.h"
//
//   int startLoggingCTD19( int ctdFD );
//
// DESCRIPTION
//   Initiate Seabird CTD 19 logging through the use of the
//   Go Log "gl" command.
//
// RETURNS
//   Returns 1 for success and -1 for failure.  The
//   cause of failure is logged at level LVL_WARN.
//
int startLoggingCTD19 ( int ctdFD )
{
  int bytesRead;
  char buffer[CTDBUFFLEN];
  int baud;

  // Say hello
  LOGPRINT( LVL_VERB, "startLoggingCTD19(): Called" );

  // Normalize the CTD state by getting an "S>" prompt
  if ( ( baud = getCTD19SPrompt( ctdFD ) ) != 600 )
  {
    // If the baud was strangely at 1200 set it back
    // to 600 and proceed
    if ( baud == 1200 )
    {
      if ( serialChat( ctdFD, "sb1\r", "sb1", 1000L, "1" ) < 1 ) 
      {
        LOGPRINT( LVL_WARN, "startLoggingCTD19(): Failed "
                  "to write sb1 command to CTD!");
        return( FAILURE );
      }
      term_set_baudrate( ctdFD, 600 );
      term_flush( ctdFD );
      if ( term_apply( ctdFD ) < 0 ) {
        LOGPRINT( LVL_WARN,
                  "startLoggingCTD19(): Failed to change "
                  "serial port baud to 600" );
        return( FAILURE );
      }
    }else {
      // All attempts at obtaining an S> prompt failed 
      LOGPRINT( LVL_WARN, "startLoggingCTD19(): All attempts "
                "at obtaining an S> prompt failed. retVal = %d", baud );
      return( FAILURE );
    }
  }

  // Ok...the moment you have been waiting for.  The
  // Go Log command.  I know you will enjoy this.
  if ( serialChat( ctdFD, "gl\r", "gl\r\n", 1000L, "\r\n" ) < 1 )
  {
    LOGPRINT( LVL_WARN, "startLoggingCTD19(): Failed sending gl "
              "command." );
    return( FAILURE );
  }

  // Read in the line "start logging Y/N ? "
  if ( ( bytesRead = 
           serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" )) != 20 )
  {
    LOGPRINT( LVL_WARN, "startLoggingCTD19(): Failed to receive "
              "confirmation string 'start logging Y/N ?'. Got '%s' "
              "instead", buffer );
    return( FAILURE );
  }

  if ( serialChat( ctdFD, "y\r", "y\r\n", 1000L, "\r\n" ) < 1 )
  {
    LOGPRINT( LVL_WARN, "startLoggingCTD19(): Failed to confirm gl "
              "command with a 'y' character" );
    return( FAILURE );
  }

  // Read in the line "are you sure ^Y/N ? "
  // NOTE: This may be custom to the UW's unit.  It appears
  //       that our firmware uses CTRL-Y instead of y for the
  //       second confirmation.
  if ( ( bytesRead = 
           serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" )) != 20 )
  {
    LOGPRINT( LVL_WARN, "startLoggingCTD19(): Failed to receive "
              "2nd confirmation string 'are you sure ^Y/N ?'. Got '%s' "
              "instead.", buffer );
    return( FAILURE );
  }

  // Send "CTRL-Y CR" and expect back "CTRL-Y CR LF" as the echo
  if ( serialChat( ctdFD, ctrlYStrCR, ctrlYStrCRLF, 1000L, "\r\n" ) < 1 )
  {
    LOGPRINT( LVL_WARN, "startLoggingCTD19(): Failed to confirm gl "
              "command a second time with a 'ctrl-y' character." );
    return( FAILURE );
  }

  // Ok....cross your fingers, wait a hell-of-a long time to see
  // if the CTD is sending data
  if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) != 26 )  
    if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) != 26 )  
      if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) != 26 )  
        if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) != 26 )  
        { 
          LOGPRINT( LVL_WARN, "startLoggingCTD19(): Failed to " 
                    " see the data stream of 26 HEX bytes.  Got %s instead.",
                    buffer );
          return( FAILURE );
        }
 
  // Say goodbye
  LOGPRINT( LVL_VERB, "startLoggingCTD19(): Returning: 1 - Success" );

  return( SUCCESS ); 
}


//
// NAME
//   getCTD19Pressure - Read a CTD line in logging mode and extract the pressure
//
// SYNOPSIS
//   #include "ctd.h"
//
//   double getCTD19Pressure( int ctdFD );
//
// DESCRIPTION
//   The Seabird CTD 19 streams measurement data to the serial port
//   in addition to saving it in the on-board data-logger.  This
//   is useful if you want to monitor the sampling as it happens.
//   The 4 bytes of the 26 byte data stream record store the
//   current pressure measurement.  This routine obtains a current
//   record, extracts the pressure bytes and converts them to
//   meters.
//
// RETURNS
//   The depth in meters or -1 for in the event of failure.
//
// WARNING: Do not add LOGGING TO THIS FUNCTION.  We do not 
//          want this function to block! It is used by 
//          the movePackageUp/Down functions!
double getCTD19Pressure ( int ctdFD )
{
  int bytesRead;
  char buffer[CTDBUFFLEN];
  double P=0.0;

  term_flush( ctdFD );
  if ( ( bytesRead = 
           serialGetLine( ctdFD, buffer, CTDBUFFLEN, 1000L, "\n" ) ) != 26 ) 
    if ( ( bytesRead = 
             serialGetLine( ctdFD, buffer, CTDBUFFLEN, 1000L, "\n" ) ) != 26 ) 
      if ( ( bytesRead = 
              serialGetLine( ctdFD, buffer, CTDBUFFLEN, 1000L, "\n" ) ) != 26 ) 
      {
        // Couldn't get a data record!
        return( FAILURE );
      }
            
  htoP(buffer,20,23,&P); 
  return( P );
}


// 
// NAME
//   downloadCTD19Data - Download CTD 19 historical data to a file
// 
// SYNOPSIS
//   #include "ctd.h"
//
//   int downloadCTD19Data( int ctdFD, FILE *outFile );
//
// DESCRIPTION
//   Communicate with a Seabird 19 CTD and download
//   all the stored casts to a file.  Communications
//   to the CTD oocur through a pre-initialized file
//   descriptor port. Data output goes directly to
//   a pre-initialized output file descriptor.
//
//   The communications with the CTD are provided through
//   an open/initialized file descriptor ctdFD.  The output
//   file is passed in as outFile and should be already opened
//   and writable.
//
// RETURNS
//     The CTD's status paragraph (ds), header paragraph (dh)
//     and cast hex data ( dc ), formated  and written to
//     the data file.  The data file remains open.  Upon
//     success a 1 is returned otherwise -1 is returned.
//     Specific problems are logged at a LVL_WARN level.
//     
//   TODO: I would like to clean this up so that it only
//         writes the data expected by the SEABIRD software 
//         and nothing extraneous.  At this point it will
//         include (or not) any echoed commands.  It also
//         should check the data before committing to the 
//         success of the download...perhaps we can read the
//         number of samples and check the data width of
//         each as they come in.
//
int downloadCTD19Data ( int ctdFD, FILE * outFile ) 
{
  int baudRate = 600;
  int bytesRead;
  char buffer[CTDBUFFLEN];

  // Say hello
  LOGPRINT( LVL_VERB, "downloadCTD19Data(): Called" );

  // Normalize the CTD state by getting an "S>" prompt
  baudRate = getCTD19SPrompt( ctdFD );
  if ( baudRate != 600 && baudRate != 1200 )
  {
    // All attempts at obtaining an S> prompt failed 
    LOGPRINT( LVL_WARN, "downloadCTD19Data(): All attempts "
              "at obtaining an S> prompt failed. retVal = %d", baudRate );
    return( FAILURE );
  }

  // can up it to 1200 to speed up the data
  // transfer.  See the Seabird manual for details
  // on baud rate and cable length ( the limiting
  // factor ).  BTW....if the data comes in 
  // noisy...you can keep the CTD at 600 by disabling
  // this code.
  //
  if ( baudRate == 600 ) {
    // Change the CTD baud to 1200
    LOGPRINT( LVL_WARN, "downloadCTD19Data(): Sending sb2 command..." );
    if ( serialChat( ctdFD, "sb2\r", "sb2", 1000L, "2" ) < 1 ) 
    {
      LOGPRINT( LVL_WARN, 
                "downloadCTD19Data(): Failed to write sb2 command to CTD!");
      return( FAILURE );
    }
    
    term_drain( ctdFD );
    // Change our serial port baud to 1200
    term_set_baudrate( ctdFD, 1200 );
    if ( term_apply( ctdFD ) < 0 ) {
      LOGPRINT( LVL_WARN, "downloadCTD19Data(): Failed to change "
                "serial port baud to 1200! NOTE: The CTD is probably "
                "still set for 1200 baud!");
      return( FAILURE );
    }
    term_flush( ctdFD );

    // Let's see if we still can talk to the CTD
    LOGPRINT( LVL_WARN, "downloadCTD19Data(): Checking communications "
              "at 1200..." );
    if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
      if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
        if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 ) 
          if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 ) {
            LOGPRINT( LVL_WARN, "downloadCTD19Data(): Failed to get "
                      "the attention of the CTD after changing baud "
                      "rate! NOTE: The CTD is probably still set "
                      "for 1200 baud!");
            return( FAILURE );
          }
  } // if ( baudRate == 600 ) 

  // Print out the status of the CTD
  serialPutLine( ctdFD, "ds\r" );
  while ( ( bytesRead = 
             serialGetLine( ctdFD, buffer, CTDBUFFLEN, 4000L, "\n" ) ) > 0 ) 
  {
    fwrite( buffer, 1, bytesRead, outFile );
  } 

  // Print out the header from the CTD
  serialPutLine( ctdFD, "dh\r" );
  while ( ( bytesRead = 
             serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) ) > 0 ) 
  {
    fwrite( buffer, 1, bytesRead, outFile );
  }  

  // Print out the data from the CTD
  serialPutLine( ctdFD, "dc\r" );
  while ( ( bytesRead = 
             serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) ) > 0 ) 
  {
    fwrite( buffer, 1, bytesRead, outFile );
  }  
  term_flush( ctdFD );

  // Change the CTD baud back to 600
  if ( serialChat( ctdFD, "sb1\r", "sb1", 1000L, "1" ) < 1 )
  { 
    LOGPRINT( LVL_WARN, "downloadCTD19Data(): Failed to write sb1 "
              "command to CTD! NOTE: The CTD is probably still set "
              "for 1200 baud!" );
    return( FAILURE );
  }
  
  // Change our serial port baud back to 600
  term_set_baudrate( ctdFD, 600 );
  if ( term_apply( ctdFD ) < 0 ) 
  {
    LOGPRINT( LVL_WARN, "downloadCTD19Data(): Failed to change serial "
              "port baud back to 600!");
    return( FAILURE );
  }
  term_flush( ctdFD );

  // Let's see if we changed the baud
  if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
    if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
      if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 ) 
        if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 ) {
        LOGPRINT( LVL_WARN, "downloadCTD19Data(): Failed to get the "
                  "attention of the CTD after changing baud rate!" );
        return( FAILURE );
      }
 

  // Try putting the CTD to sleep
  if ( serialPutLine( ctdFD, "qs\r") < 1 ) {  
    LOGPRINT( LVL_WARN, "downloadCTD19Data(): Failed to write qs "
              "command to CTD!" );
    return( FAILURE );
  }

  // Say goodbye 
  LOGPRINT( LVL_VERB, "downloadCTD19Data(): Returning: 1 - Success" );

  return( SUCCESS );

}

//
// NAME
//   getCTD19Time - Get the date/time from the CTD 19.
//
// SYNOPSIS
//   #include "ctd.h"
// 
//   struct tm *getCTD19Time( int ctdFD );
//
// DESCRIPTION
//   Get the date and time in UNIX struct tm format from 
//   the Seabird CTD 19
//
// RETURNS
//   A pointer to a tm structure or NULL upon
//   failure.
//
struct tm *getCTD19Time ( int ctdFD ) 
{
  static struct tm ctdTime;
  int bytesRead = 0;
  char buffer[CTDBUFFLEN];
  char chompBuff[21];
  int day = 0, year = 0, month = 0, hour = 0, min = 0, sec = 0;

  // Say hello
  LOGPRINT( LVL_VERB, "getCTD19Time(): Called" );

  // Normalize the CTD state by getting an "S>" prompt
  if ( getCTD19SPrompt( ctdFD ) < 1 )
  {
    // All attempts at obtaining an S> prompt failed 
    LOGPRINT( LVL_WARN, "getCTD19Time(): All attempts "
              "at obtaining an S> prompt failed." );
    return( (struct tm *)NULL );
  }

  serialPutLine( ctdFD, "DS\r" );
  while ( ( bytesRead = 
             serialGetLine( ctdFD, buffer, CTDBUFFLEN, 6000L, "\n" ) ) > 0 ) 
  {
    //SEACAT PROFILER V1.5 SN 936  02/10/94 14:02:13.439
    if ( strstr( buffer, "SEACAT PROFILER" ) )
    {
      sscanf( buffer, "SEACAT %20s %20s %20s %20s "
                      "%d/%d/%d %d:%d:%d", chompBuff, chompBuff,
                      chompBuff, chompBuff,
                      &month, &day, &year, &hour, &min, &sec );
    }else if ( strstr( buffer, "logdata =" ) )
    {
      break;
    }
  } 

  // Check time 
  if ( month == 0 )
  {
    LOGPRINT( LVL_WARN, "getCTD19Time(): Couldn't parse CTD DS output!" );
    return ( (struct tm *)NULL );
  }
  LOGPRINT( LVL_INFO, "getCTD19Time(): Got Time %d/%d/%d  %d:%d:%d",
               month, day, year, hour, min, sec );

  // Setup the data structure
  ctdTime.tm_sec  = sec; 
  ctdTime.tm_min  = min; 
  ctdTime.tm_hour = hour; 
  ctdTime.tm_mday = day; 
  ctdTime.tm_mon  = month-1; 
  ctdTime.tm_year = year + 100; 

  return ( &ctdTime );
}

//
// NAME
//   setCTD19Time - Set the date/time on the CTD 19
//
// SYNOPSIS
//   #include "ctd.h"
//
//   int setCTD19Time( int ctdFD, struct tm *time );
//
// DESCRIPTION
//   Set the date/time on the Seabird CTD 19 to
//   the values stored in the UNIX tm format.
//
// RETURNS
//    1 Upon success
//   -1 Upon failure
//
int setCTD19Time ( int ctdFD, struct tm *time ) {
  char cmdBuff[CTDBUFFLEN];
  char respBuff[CTDBUFFLEN];

  // Normalize the CTD state by getting an "S>" prompt
  if ( getCTD19SPrompt( ctdFD ) < 1 )
  {
    // All attempts at obtaining an S> prompt failed 
    LOGPRINT( LVL_WARN, "setCTD19Time(): All attempts "
              "at obtaining an S> prompt failed." );
    return( FAILURE );
  }

  // Seabird commands: MMDDYY=mmddyy
  //                   HHMMSS=hhmmss
  // Both must appear in that order to result in a time
  // change.  The commands only return a fresh "S>" prompt.
  snprintf( cmdBuff, CTDBUFFLEN, "%02d%02d%02d\r", 
            time->tm_mon + 1, time->tm_mday, ( time->tm_year - 100 ) );
  strcpy( respBuff, cmdBuff );
  strncat( respBuff, "\n", CTDBUFFLEN );

  if ( serialChat ( ctdFD, "ST\r", "ST\r\n", 2000L, 
                    "\r\ndate (MMDDYY) = " ) < 1 ) 
  {
    LOGPRINT( LVL_WARN, "setCTD19Time(): Failed sending "
              "ST command!" );
    return( FAILURE );
  }

  LOGPRINT( LVL_INFO, "setCTD19Time(): Sending command %s", cmdBuff );

  if ( serialChat( ctdFD, cmdBuff, respBuff,
                   2000L, "\r\ntime (HHMMSS) = " ) < 1 )
  {
    LOGPRINT( LVL_WARN, "setCTD19Time(): Failed sending "
              "%s date data.", cmdBuff );
    return( FAILURE );
  }

  snprintf( cmdBuff, CTDBUFFLEN, "%02d%02d%02d\r", 
            time->tm_hour, time->tm_min, time->tm_sec );
  strcpy( respBuff, cmdBuff );
  strncat( respBuff, "\n", CTDBUFFLEN );

  LOGPRINT( LVL_INFO, "setCTD19Time(): Sending command %s", cmdBuff );

  if ( serialChat( ctdFD, cmdBuff, respBuff,
                   1000L, "\r\nS>" ) < 1 )
  {
    LOGPRINT( LVL_WARN, "setCTD19Time(): Failed sending "
              "%s command.", cmdBuff );
    return( FAILURE );
  }

  return ( SUCCESS );
}

//
// NAME
//   htoP - Convert CTD 4 Byte Hex and convert to 14bit signed number
//
// SYNOPSIS
//   #include "ctd.h"
//
//   void htoP( char str[], int startc, int endc, double *P);
//
// DESCRIPTION
//   Written for the original ORCA project.  This function has
//   be borrowed for the new project.  Here is the original
//   description:
//     "htoP: take the four hexadecimal (15 bit) character pressure,
//      disregards bit 15, interprets bit 14 as the sign (1=neg, 0=pos) 
//      and interprets bits 0-13 as the pressure magnitude"
//
// RETURNS
//   The converted number is stored in the double pointed to by
//   the input parameter P. 
//
void htoP ( char str[], int startc, int endc, double *P)
{
  int i, sign;
  long n;

  n=0L;
  sign=1;

  // turn first hex character into decimal 
  i=startc;
  if (str[i] >= '0' && str[i] <= '9') n=16*n + (str[i]-'0');
  else if (str[i] >= 'A' && str[i] <= 'F') n=16*n + (str[i]-'A'+10);
  else if (str[i] >= 'a' && str[i] <= 'f') n=16*n + (str[i]-'a'+10);
  else 
  {
    LOGPRINT( LVL_ALRT, "htoP(): Error in hex value! (%c)", str[i] );
  } 

  // disregard bit 15 
  if(n>=8) n=n-8;
  // interpret bit 14 as the sign 
  if(n>=4) 
  {
    n=n-4;
    sign=-sign;
  }

  // interpret bits 0-13 as the magnitude 
  for(i=startc+1; i<=endc && str[i] != '\0';i++)
  {
    if (str[i] >= '0' && str[i] <= '9') n=16*n + (str[i]-'0');
    else if (str[i] >= 'A' && str[i] <= 'F') n=16*n + (str[i]-'A'+10);
    else if (str[i] >= 'a' && str[i] <= 'f') n=16*n + (str[i]-'a'+10);
    else printf("error in hex conv");
  }
  n=sign*n;
  // 
  //  CONVERT P TO DECIBARS OF WATER ASSUMING ATMOSPHERE IS 9.5 DECIBARS
  // 
  //*P=(248.24555-6.524518E-2*n+5.430179E-8*n*n)*0.689476-9.6;
  *P=( opts.ctdCalPA1 + opts.ctdCalPA2*n + opts.ctdCalPA3*n*n )*0.689476-9.6;
} 



/////////////////////SEABIRD 19+ FUNCTIONS/////////////////////////////

//
// NAME
//   initCTD19Plus - Initialize the CTD for normal operations
//
// SYNOPSIS
//   #include "ctd.c"
//   
//   int initCTD19Plus( int ctdFD );
//
// DESCRIPTION
//   The 19 Plus CTD can be configured in many different ways.
//   In order to be sure it is setup for our use this routine
//   sets specific settings on the CTD.  Currently we
//   want the CTD set to:
//
//      MP mode
//      AUTORUN=n
//      IGNORESWITCH=y
//      OUTPUTFORMAT=3
//
// RETURN VALUE
//   The function returns 1 upon success and a -1 upon failure.
//
int initCTD19Plus ( int ctdFD ) {
  char buffer[CTDBUFFLEN];

  // Say hello
  LOGPRINT( LVL_VERB, "initCTD19Plus(): Called" );

  // Start off with the S> prompt.  This ensures
  // that we are not logging when we attempt to
  // set the CTDs parameters.
  getCTD19PlusSPrompt ( ctdFD );

  // Clear the buffered input so that we don't
  // have to contend with junk.
  term_flush( ctdFD );

  // First send MP command.  
  if ( serialChat( ctdFD, "MP\r", "MP\r\n", 500L, "\r\n" ) < 1 )
  {
    LOGPRINT( LVL_WARN, "initCTD19Plus(): Failed to send MP command." );
    return( FAILURE );
  }

  //
  // The MP command will either return a line to confirm
  // the change with the following prompt:
  //   "this command will change the scan length and initialize logging
  //    proceed Y/N ?"
  // Or it will return an "S>" prompt if it's already in this
  // mode.
  // Or if it is the new 19+ V2 then it will ask for the command to
  // be repeated:
  //   "this command will change the scan length and initialize logging
  //    repeat the command to verify"
  //
  serialGetLine( ctdFD, buffer, CTDBUFFLEN, 500L, "\r\n" );
  if ( strstr( buffer, "this command will change" ) ) 
  {
    serialGetLine( ctdFD, buffer, CTDBUFFLEN, 500L, "\r\n" );
    if ( strstr( buffer, "repeat the command" ) )
    {
      term_flush( ctdFD );
      if ( serialChat( ctdFD, "MP\r", "MP\r\n", 500L, "\r\n" ) < 1 )
      {
        LOGPRINT( LVL_WARN, "initCTD19Plus(): Failed to send 2nd MP command." );
        return( FAILURE );
      }
      serialGetLine( ctdFD, buffer, CTDBUFFLEN, 500L, "\r\n" );
      if ( ! strstr( buffer, "S>" ) ) 
      {
        LOGPRINT( LVL_WARN, "initCTD19Plus(): Strange response to 2nd MP "
                 "command: %s", buffer );
        return( FAILURE );
      }
    }else {
      term_flush( ctdFD );
      if ( serialChat( ctdFD, "y\r", "y\r\n", 500L, "y\r\n" ) < 1 ){
        LOGPRINT( LVL_WARN, "initCTD19Plus(): Could not send 'y' to confirm "
                  "MP command." );
        return( FAILURE );
      }
      serialGetLine( ctdFD, buffer, CTDBUFFLEN, 500L, "\r\n" );
      if ( ! strstr( buffer, "scan length has changed" ) ) 
      {
        LOGPRINT( LVL_WARN, "initCTD19Plus(): Error confirming change to "
                  "MP mode." );
        return( FAILURE );
      }
      term_flush( ctdFD );
    }
  }else if ( ! strstr( buffer, "S>" ) ) 
  {
    LOGPRINT( LVL_WARN, "initCTD19Plus(): Strange response to MP "
              "command: %s", buffer );
    return( FAILURE );
  }

  term_flush( ctdFD );
  if ( serialChat( ctdFD, "IGNORESWITCH=y\r", "IGNORESWITCH=y", 500L, "S>" ) 
       < 1 )
  {  
    sleep(1); // To overcome noisy communications 
    if ( serialChat( ctdFD, "IGNORESWITCH=y\r", "IGNORESWITCH=y", 500L, "S>" ) 
       < 1 )
    {
      LOGPRINT( LVL_WARN, "initCTD19Plus(): Could not set IGNORESWITCH=y"
                          " after two attempts." );
      return( -1 );
    }
  }

  if ( serialChat( ctdFD, "AUTORUN=n\r", "AUTORUN=n", 500L, "S>" ) < 1 )
  { 
    sleep(1); // To overcome noisy communications
    if ( serialChat( ctdFD, "AUTORUN=n\r", "AUTORUN=n", 500L, "S>" ) < 1 )
    {
      LOGPRINT( LVL_WARN, "initCTD19Plus(): Could not set AUTORUN=n after "
                          "two attempts." );
      return( -1 );
    }
  }

  // NAVG is important.  It indicates how many scans to average
  // before a pressure is reported on the serial lines.  If
  // this is set >1 we might crash into the buoy before we even
  // know we are close to the stopping pressure.
  if ( serialChat( ctdFD, "NAVG=1\r", "NAVG=1", 500L, "S>" ) 
       < 1 )
  {
    sleep( 1 ); // To overcome noisy communications
    if ( serialChat( ctdFD, "NAVG=1\r", "NAVG=1", 500L, "S>" ) 
         < 1 )
    {
      LOGPRINT( LVL_WARN, "initCTD19Plus(): Could not set NAVG=1 after "
                          "two attempts." );
      return( -1 );
    }
  }

  if ( serialChat( ctdFD, "OUTPUTFORMAT=3\r", "OUTPUTFORMAT=3", 500L, "S>" ) 
       < 1 )
  {
    sleep( 1 ); // To overcome noisy communications
    if ( serialChat( ctdFD, "OUTPUTFORMAT=3\r", "OUTPUTFORMAT=3", 500L, "S>" ) 
         < 1 )
    {
      LOGPRINT( LVL_WARN, "initCTD19Plus(): Could not set OUTPUTFORMAT=3 "
                          "two attempts." );
      return( -1 );
    }
  }

  // Say goodbye 
  LOGPRINT( LVL_VERB, "initCTD19Plus(): Returning: 1 - Success" );

  // Success
  return ( 1 );
}


//
// NAME
//   getCTD19PlusSPrompt - Obtain an "S>" prompt from a Seabird CTD 19+
//
// SYNOPSIS
//   #include "ctd.c"
//
//   int getCTD19PlusSPrompt( int ctdFD );
//
// DESCRIPTION
//   The Seabird CTD 19+ "S>" prompt is the main command
//   prompt.  Often the CTD is already waiting at an
//   "S>" prompt when this function is called.  Other times
//   the CTD may be in a data transfer mode ( such as "STARTNOW" ).
//   Unlike the older models ( 19  etc ) the 19+ can communicate
//   at higher baud rates ( currently 9600 ) so we do not need
//   to switch the baud rate.  If the CTD is logging this
//   routine will first shut off logging using the STOP
//   command and then attempt to deliver a clean S> prompt.
//
// RETURN VALUE
//   The function returns 1 upon success and a -1 upon failure.
//
int getCTD19PlusSPrompt ( int ctdFD )
{
  int i = 0;
  char buffer[CTDBUFFLEN];

  // Say hello
  LOGPRINT( LVL_VERB, "getCTD19PlusSPrompt(): Called" );

  // Send some carriage returns to try and get a prompt.
  // If the CTD is sleeping it will take up to 1450ms
  // to get a response.
  serialPutLine( ctdFD, "\r\r\r");
  serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "S>" );

/*
  // The CTD 19+ does allow one to obtain an S> prompt
  // even while data is streaming.  One might argue
  // that this routine as named shouldn't care what
  // mode it's in as long as we can get a S> prompt.
  // Unfortunately the existing code which calls this
  // routine does believe that an S> prompt indicates
  // that we are not logging.  Long story short. If
  // we send a CTRL-Z command we can ensure that we
  // are sitting an S> with logging turned off. 
  //
  // Send a control-Z 
  serialPutByte( ctdFD, 0x1A );
  serialPutByte( ctdFD, '\r' );
  sleep( 1 ); 

  // Old code which tried to use the STOP command.  Why would
  // Seabird create this command if it's not reliably 
  // recognized????
  //serialPutLine( ctdFD, "STOP\r" );
  //serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "S>" );

  //
  // Since we are about to query the state of the 
  // serial line we should first start with a clean
  // slate.  Flush out all writes and remove anything
  // waiting to be read.  This way we ensure that
  // the data we read is in response to something we
  // say here and not something done before this
  // routine was called.
  //
  term_flush( ctdFD );

  // Sanity check that we are not logging. Start
  // by resyncing ourselves to a line boundry
  if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 500L, "\n" ) < 1 ) 
    serialGetLine( ctdFD, buffer, CTDBUFFLEN, 500L, "\n" );  

  // Now check if we are getting any significant amount of data
  // from the CTD
  if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 1000L, "\n" ) > 20 )  
  {
    // Sometimes it takes awhile? TODO:...test this more
    sleep(1);
    if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 1000L, "\n" ) > 20 ) 
    { 
      LOGPRINT( LVL_WARN, "getCTD19PlusSPrompt(): CTD appears to be ignoring "
                "our attempt to STOP logging.  Its saying: %s", buffer );
      return( FAILURE );
    }
  } 
  */

  // New test code
  for ( i = 0; i < 10; i++ )
  {
    LOGPRINT( LVL_VERB, "getCTD19PlusSPrompt(): Sending Ctrl-Z" );
    serialPutByte( ctdFD, 0x1A );
    serialPutByte( ctdFD, '\r' );
    sleep( 1 ); 
    term_flush( ctdFD );
    if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 1000L, "\n" ) < 1 )  
      break;
  }
  // end new test code

  // Now for the biggest trick of all.  Look for an 
  // actual S> prompt.
  if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
    if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
      if ( serialChat( ctdFD, "\r", "\r\nS>", 500L, ">" ) < 1 )
      {
        LOGPRINT( LVL_WARN, "getCTD19PlusSPrompt(): Failed attempt at "
                  "getting an S> prompt." );
        return( FAILURE );
      }
    
  // Say goodbye
  LOGPRINT( LVL_VERB, "getCTD19PlusSPrompt(): Returning: 1 - Success" );
  
  return( SUCCESS );

}

//
// NAME
//   stopLoggingCTD19Plus - Cease CTD logging and pressure output
//
// SYNOPSIS
//   #include "ctd.h"
//
//   int stopLoggingCTD19Plus( int ctdFD );
//
// DESCRIPTION
//   Cease Seabird CTD 19+ logging by issueing a STOP
//   command.  This routine is really a wrapper for the
//   getCTD19PlusSPrompt() routine.  This is to preserve
//   the interface that was used for the CTD 19 code which
//   has a more complicated stop procedure.
//
// RETURNS
//   Returns 1 for success and -1 for failure.  The
//   cause of failure is logged at level LVL_WARN.
//
int stopLoggingCTD19Plus ( int ctdFD )
{
  // Say hello
  LOGPRINT( LVL_VERB, "stopLoggingCTD19(): Called" );

  return( getCTD19PlusSPrompt( ctdFD ) );
}


//
// NAME
//   startLoggingCTD19Plus - Initiate CTD logging and pressure output
//
// SYNOPSIS
//   #include "ctd.h"
//
//   int startLoggingCTD19Plus( int ctdFD );
//
// DESCRIPTION
//   Initiate Seabird CTD 19+ logging through the use of the
//   Go Log "STARTNOW" command.
//
// RETURNS
//   Returns 1 for success and -1 for failure.  The
//   cause of failure is logged at level LVL_WARN.
//
int startLoggingCTD19Plus ( int ctdFD )
{
  int retries;
  int failed;
  int bytesRead;
  char buffer[CTDBUFFLEN];

  // Say hello
  LOGPRINT( LVL_VERB, "startLoggingCTD19Plus(): Called" );

  // New code to support both 19Plus V1 and V2
  retries = 0;
  failed = 1;
  do { 

    // Normalize the CTD state by getting an "S>" prompt
    if ( getCTD19PlusSPrompt( ctdFD ) < 1 )
    {
      LOGPRINT( LVL_ALRT, "startLoggingCTD19Plus(): Could not get "
                "initial S> prompt" );
    }

    LOGPRINT( LVL_DEBG, "startLoggingCTD19Plus(): Sending INITLOGGING command " 
                        "( retries = %d )", retries );
    if ( serialChat( ctdFD, "INITLOGGING\r", 
                     "INITLOGGING\r\n", 1000L, "\r\n" ) < 1 )
    {
       LOGPRINT( LVL_WARN, "startLoggingCTD19Plus(): Failed sending "
                 "first INITLOGGING command ( retries = %d ).", retries );
    }
  
    // V2 prints out the lines:
    //   "this command will change the scan length and/or initialize logging
    //    repeat the command to verify"
    if ( ( bytesRead = 
             serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" )) > 0 )
    {
       LOGPRINT( LVL_DEBG, "startLoggingCTD19Plus(): Read %d bytes.", 
                 bytesRead );
       LOGPRINT( LVL_DEBG, "startLoggingCTD19Plus(): >%s<", buffer );
       if ( strstr( buffer, "this command will" ) != NULL )
       {
         // V2 response
         serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" );
         //term_flush( ctdFD );
         if ( serialChat( ctdFD, "INITLOGGING\r", 
                          "INITLOGGING\r\n", 1000L, "\r\nS>" ) < 1 )
         {
           LOGPRINT( LVL_WARN, "startLoggingCTD19Plus(): Failed sending "
                     "confirmation INITLOGGING command ( retries = %d ).", 
                     retries );
         }else 
         {
           failed = 0;
         }
       }else if ( strstr( buffer, "S>" ) )
       {
         // V1 response 
         failed = 0;
       }else 
       {
         // Not V1 response 
         LOGPRINT( LVL_WARN, "startLoggingCTD19Plus(): Couldn't parse "
                   "output of INITLOGGING command: >%s< ( retries = %d ).", 
                   buffer, retries );
       } 
    }
  }while ( failed && (retries++ < 2) );

  if ( failed ) 
  {
    LOGPRINT( LVL_WARN, "startLoggingCTD19Plus(): Failed to issue "
              "INITLOGGING command to CTD after %d retries.", retries );
    return( FAILURE );
  }

  // Sanitize the communications before moving on
  term_flush( ctdFD );

  // Put the CTD in decimal engineering units outputmode
  // This is a nifty new feature of the 19+ which allows
  // us to use the built in calibrations to convert the
  // pressure output into decibars automatically.  Putting
  // the CTD into decimal engineering units output format
  // doesn't alter the way data is recorded in the onboard
  // data log.  In this way we can monitor the pressure
  // in engineering units and download the data in raw
  // format later.
  if ( serialChat( ctdFD, "OUTPUTFORMAT=3\r", 
                   "OUTPUTFORMAT=3\r\n", 1000L, "\r\nS>" ) < 1 )
  {
    sleep(1); // To overcome noisy communications
    if ( serialChat( ctdFD, "OUTPUTFORMAT=3\r", 
                     "OUTPUTFORMAT=3\r\n", 1000L, "\r\nS>" ) < 1 )
    {
      LOGPRINT( LVL_WARN, "startLoggingCTD19Plus(): Failed sending "
                "OUTPUTFORMAT=3 command after two attempts." );
      return( FAILURE );
    }
  }

  // Same as the "GL" command used in the 19
  if ( serialChat( ctdFD, "STARTNOW\r", 
                   "STARTNOW\r\n", 1000L, "\r\n" ) < 1 )
  {
    sleep(1); // To overcome noisy communications
    if ( serialChat( ctdFD, "STARTNOW\r", 
                     "STARTNOW\r\n", 1000L, "\r\n" ) < 1 )
    {
      LOGPRINT( LVL_WARN, "startLoggingCTD19Plus(): Failed sending "
                "STARTNOW command after two attempts." );
      return( FAILURE );
    }
  }

  // Ok....cross your fingers, wait a sort-of-a long time to see
  // if the CTD is sending data
  if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) < 20 )  
    if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) < 20 )  
      if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) < 20 )  
        if ( serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) < 20 )  
        { 
          LOGPRINT( LVL_WARN, "startLoggingCTD19Plus(): Failed to " 
                    " see the data stream of values after 8 seconds.  Got %s instead.",
                    buffer );
          return( FAILURE );
        }
 
  // Say goodbye
  LOGPRINT( LVL_VERB, "startLoggingCTD19Plus(): Returning: 1 - Success" );

  return( SUCCESS ); 
}


//
// NAME
//   getCTD19PlusPressure - Read a CTD line in logging mode and 
//                          extract the pressure.
//
// SYNOPSIS
//   #include "ctd.h"
//
//   double getCTD19PlusPressure( int ctdFD );
//
// DESCRIPTION
//   The Seabird CTD 19+ streams measurement data to the serial port
//   in addition to saving it in the on-board data-logger.  This
//   is useful if you want to monitor the sampling as it happens.
//   This routine assumes that the CTD 19+ has been placed in
//   OUTPUTFORMAT=3 or Decimal Engineering Units.  In this
//   format the CTD 19+ displays as the first 3 comma seperated
//   floating point numbers: temperature, conductivity and 
//   pressure ( in decibars ).  This routine captures the
//   pressure value and returns it.
//
// RETURNS
//   The pressure in decibars or -1 for in the event of failure.
//
// WARNING: Do not add LOGGING TO THIS FUNCTION.  We do not 
//          want this function to block! It is used by 
//          the movePackageUp/Down functions!
double getCTD19PlusPressure ( int ctdFD )
{
  int numAttempts = 3;
  int numConv = 0;
  char buffer[CTDBUFFLEN];
  double temperature, conductivity, pressure;
    
  // Flush and resync ourselves on a line
  term_flush( ctdFD );
  serialGetLine( ctdFD, buffer, CTDBUFFLEN, 1000L, "\n" ); 
  do  
  {
    serialGetLine( ctdFD, buffer, CTDBUFFLEN, 1000L, "\n" ); 
    numConv = sscanf( buffer, "%lf,%lf,%lf", 
                      &temperature, &conductivity, &pressure );
  }while ( numAttempts-- > 0 && numConv != 3 );

  if ( numConv == 3 )  
  {
    return( pressure );    
  }

  return( FAILURE );
}


// 
// NAME
//   downloadCTD19PlusData - Download CTD 19 historical data to a file
// 
// SYNOPSIS
//   #include "ctd.h"
//
//   int downloadCTD19PlusData( int ctdFD, FILE *outFile );
//
// DESCRIPTION
//   Communicate with a Seabird 19+ CTD and download
//   all the stored casts to a file.  Communications
//   to the CTD oocur through a pre-initialized file
//   descriptor port. Data output goes directly to
//   a pre-initialized output file descriptor.
//
//   The communications with the CTD are provided through
//   an open/initialized file descriptor ctdFD.  The output
//   file is passed in as outFile and should be already opened
//   and writable.
//
//   NOTE: This routine also puts the CTD to sleep.
//
// RETURNS
//     The CTD's status paragraph (ds), header paragraph (dh)
//     and cast hex data ( dc ), formated  and written to
//     the data file.  The data is returned in OUTPUTFORMAT=0
//     which is raw HEX.  The data file remains open.  Upon
//     success a 1 is returned otherwise -1 is returned.
//     Specific problems are logged at a LVL_WARN level.
//     
//   TODO: I would like to clean this up so that it only
//         writes the data expected by the SEABIRD software 
//         and nothing extraneous.  At this point it will
//         include (or not) any echoed commands.  It also
//         should check the data before committing to the 
//         success of the download...perhaps we can read the
//         number of samples and check the data width of
//         each as they come in.
//
int downloadCTD19PlusData ( int ctdFD, FILE * outFile ) 
{
  int bytesRead;
  char buffer[CTDBUFFLEN];

  // Say hello
  LOGPRINT( LVL_VERB, "downloadCTD19PlusData(): Called" );

  // Normalize the CTD state by getting an "S>" prompt
  if ( getCTD19PlusSPrompt( ctdFD ) < 1 )
  {
    // All attempts at obtaining an S> prompt failed 
    LOGPRINT( LVL_WARN, "downloadCTD19PlusData(): All attempts "
              "at obtaining an S> prompt failed." );
    return( FAILURE );
  }

  // Put the CTD in HEX outputmode
  if ( serialChat( ctdFD, "OUTPUTFORMAT=0\r", 
                   "OUTPUTFORMAT=0\r\n", 1000L, "\r\nS>" ) < 1 )
  {
    sleep(1); // To overcome noisy communications
    if ( serialChat( ctdFD, "OUTPUTFORMAT=0\r", 
                   "OUTPUTFORMAT=0\r\n", 1000L, "\r\nS>" ) < 1 )
    {
      LOGPRINT( LVL_WARN, "downloadCTD19PlusData(): Failed sending "
                "OUTPUTFORMAT=0 command after three attempts" );
      return( FAILURE );
    }
  }

  // Print out the status of the CTD
  serialPutLine( ctdFD, "DS\r" );
  while ( ( bytesRead = 
             serialGetLine( ctdFD, buffer, CTDBUFFLEN, 4000L, "\n" ) ) > 0 ) 
  {
    fwrite( buffer, 1, bytesRead, outFile );
  } 

  // Print out the header from the CTD
  serialPutLine( ctdFD, "DH\r" );
  while ( ( bytesRead = 
             serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) ) > 0 ) 
  {
    fwrite( buffer, 1, bytesRead, outFile );
  }  

  // Print out the data from the CTD
  serialPutLine( ctdFD, "DC\r" );
  while ( ( bytesRead = 
             serialGetLine( ctdFD, buffer, CTDBUFFLEN, 2000L, "\n" ) ) > 0 ) 
  {
    fwrite( buffer, 1, bytesRead, outFile );
  }  
  term_flush( ctdFD );

  // Try putting the CTD to sleep
  if ( serialPutLine( ctdFD, "QS\r") < 1 ) {  
    LOGPRINT( LVL_WARN, "downloadCTD19PlusData(): Failed to write qs "
              "command to CTD!" );
    return( FAILURE );
  }

  // Say goodbye 
  LOGPRINT( LVL_VERB, "downloadCTD19PlusData(): Returning: 1 - Success" );

  return( SUCCESS );
}



//
// NAME
//   getCTD19PlusTime - Get the date/time from the 19Plus CTD.
//
// SYNOPSIS
//   #include "ctd.h"
// 
//   struct tm *getCTD19PlusTime( int ctdFD );
//
// DESCRIPTION
//   Get the date and time in UNIX struct tm format from 
//   the Seabird CTD 19plus
//
// RETURNS
//   A pointer to a tm structure or NULL upon
//   failure.
//
struct tm *getCTD19PlusTime ( int ctdFD ) 
{
  static struct tm ctdTime;
  int bytesRead = 0;
  int i = 0;
  char buffer[CTDBUFFLEN];
  char chompBuff[21];
  int day = 0, year = 0, hour = 0, min = 0, sec = 0;
  char month[4];

  LOGPRINT( LVL_VERB, "getCTD19PlusTime(): Called" );

  // Normalize the CTD state by getting an "S>" prompt
  if ( getCTD19PlusSPrompt( ctdFD ) < 1 )
  {
    // All attempts at obtaining an S> prompt failed 
    LOGPRINT( LVL_WARN, "getCTD19PlusTime(): All attempts "
              "at obtaining an S> prompt failed." );
    return( (struct tm *)NULL );
  }

  serialPutLine( ctdFD, "DS\r" );
  while ( ( bytesRead = 
             serialGetLine( ctdFD, buffer, CTDBUFFLEN, 4000L, "\n" ) ) > 0 ) 
  {
    //SeacatPlus V 1.5 SERIAL No. 4000  22 May 2005 14:02:13
    if ( ( i = sscanf( buffer, "SeacatPlus %20s %20s %20s %20s %20s "
                       "%d %3s %d %d:%d:%d", chompBuff, chompBuff,
                       chompBuff, chompBuff, chompBuff,
                       &day, month, &year, &hour, &min, &sec ) ) == 11 )
    {
      break;
    // SBE 19plus V 2.0c  SERIAL NO. 6087    11 Apr 2010 21:05:54
    }else if ( ( i = sscanf( buffer, "SBE 19plus %20s %20s %20s %20s %20s "
                       "%d %3s %d %d:%d:%d", chompBuff, chompBuff,
                       chompBuff, chompBuff, chompBuff,
                       &day, month, &year, &hour, &min, &sec ) ) == 11 )
    {
      break;
    // SBE 19plus V 2.6 RC1 SERIAL NO. 7507 24 Mar 2015 11:56:28
    }else if ( ( i = sscanf( buffer, "SBE 19plus %20s %20s %20s %20s %20s %20s "
                       "%d %3s %d %d:%d:%d", chompBuff, chompBuff,
                       chompBuff, chompBuff, chompBuff, chompBuff,
                       &day, month, &year, &hour, &min, &sec ) ) == 12 )
    {
      break;
    } 
  } 

  // Check time 
  if ( day == 0 || i < 11 || i > 12 )
  {
    LOGPRINT( LVL_INFO, "getCTD19PlusTime(): Couldn't parse CTD DS output!: %s",
              buffer );
    return ( (struct tm *)NULL );
  }

  // Convert string month name into an index from 0-11
  for (i = 0; monthNamesList[i]; ++i)
  {
    if ( strncasecmp( month, monthNamesList[i],
                      strlen(monthNamesList[i])) == 0)
    {
      break;
    }
  }

  if ( strncasecmp( month, monthNamesList[i],
                    strlen(monthNamesList[i])) != 0)
  {
    LOGPRINT( LVL_INFO, "getCTD19PlusTime(): Couldn't parse the month field: "
                        "%s", month );
    return ( (struct tm *)NULL );
  }

  // Setup the data structure
  ctdTime.tm_sec  = sec; 
  ctdTime.tm_min  = min; 
  ctdTime.tm_hour = hour; 
  ctdTime.tm_mday = day; 
  ctdTime.tm_mon  = i; 
  ctdTime.tm_year = year - 1900; 
  // See the getWSTime() routine for an explanation of this
  ctdTime.tm_isdst = -1; 

  return ( &ctdTime );
}

 
//
// NAME
//   setCTD19PlusTime - Set the date/time on the CTD 19plus
//
// SYNOPSIS
//   #include "ctd.h"
//
//   int setCTD19PlusTime( int ctdFD, struct tm *time );
//
// DESCRIPTION
//   Set the date/time on the Seabird CTD 19plus to
//   the values stored in the UNIX tm format.
//
// RETURNS
//    1 Upon success
//   -1 Upon failure
//
int setCTD19PlusTime ( int ctdFD, struct tm *time ) {
  char cmdBuff[CTDBUFFLEN];
  char respBuff[CTDBUFFLEN];

  // Normalize the CTD state by getting an "S>" prompt
  if ( getCTD19PlusSPrompt( ctdFD ) < 1 )
  {
    // All attempts at obtaining an S> prompt failed 
    LOGPRINT( LVL_WARN, "setCTD19PlusTime(): All attempts "
              "at obtaining an S> prompt failed." );
    return( FAILURE );
  }

  // Seabird commands: MMDDYY=mmddyy
  //                   HHMMSS=hhmmss
  // Both must appear in that order to result in a time
  // change.  The commands only return a fresh "S>" prompt.
  snprintf( cmdBuff, CTDBUFFLEN, "MMDDYY=%02d%02d%02d\r", 
            time->tm_mon + 1, time->tm_mday, ( time->tm_year - 100 ) );
  strcpy( respBuff, cmdBuff );
  strncat( respBuff, "\n", CTDBUFFLEN );

  LOGPRINT( LVL_INFO, "setCTD19PlusTime(): Sending command %s", cmdBuff );
  if ( serialChat( ctdFD, cmdBuff, respBuff,
                   1000L, "\r\nS>" ) < 1 )
  {
    LOGPRINT( LVL_WARN, "setCTD19PlusTime(): Failed sending "
              "%s command.", cmdBuff );
    return( FAILURE );
  }

  snprintf( cmdBuff, CTDBUFFLEN, "HHMMSS=%02d%02d%02d\r", 
            time->tm_hour, time->tm_min, time->tm_sec );
  strcpy( respBuff, cmdBuff );
  strncat( respBuff, "\n", CTDBUFFLEN );

  LOGPRINT( LVL_INFO, "setCTD19PlusTime(): Sending command %s", cmdBuff );
  if ( serialChat( ctdFD, cmdBuff, respBuff,
                   1000L, "\r\nS>" ) < 1 )
  {
    LOGPRINT( LVL_WARN, "setCTD19PlusTime(): Failed sending "
              "%s command.", cmdBuff );
    return( FAILURE );
  }

  return ( SUCCESS );
}
