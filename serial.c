/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * serial.c : The general purpose serial routines  
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
 * TODO: 
 *   I wonder if we should allow blocking reads and use
 *   use VTIME to handle timeouts?
 */
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include "general.h"
#include "term.h"
#include "timer.h"
#include "log.h"
#include "orcad.h"
#include <ftdi.h>


//
// NAME
//   serialGetByte - Get a byte from a serial port
//
// SYNOPSIS
//   #include "serial.h"
//
//   int serialGetByte( int fd, char *value, long timeout );
//
// DESCRIPTION
//   Get a byte from a serial port.  Wait up to timeout
//   milliseconds before giving up.  Return the byte
//   in the memory pointed to by value.  The file
//   descriptor fd is assumed to be open and ready for
//   reading.
//
// RETURNS
//   -1     : If an error occurs ( reason in errno )
//   0 or 1 : The number of bytes retreived upon 
//            successful completion of the operation.
//
int serialGetByte ( int fd, char *value, long timeout ) {
  int ret;
  ssize_t bytesRead;
  struct timeval startTime;

  // Get the time of day for later comparison
  //   I know this may be slow...especially when
  //   timeouts are not used.  You add at least
  //   one system call to the routine.  Should this
  //   be a problem we could optimize this routine.
  //
  ret = gettimeofday( &startTime, NULL );
  if ( ret == 0 ) {
    do {
      bytesRead = read( fd, value, 1 );
      if ( bytesRead < 0 ) {
        // Check errno ( EINTR && EAGAIN are ok )
        if ( errno != EINTR && errno != EAGAIN ) {
          return( FAILURE );
        }
      }
    } while ( timeout > 0 && ( getMilliSecSince( &startTime ) < timeout ) &&
              bytesRead < 1 );
  }else {
    // Couldn't get time of day???
    return( FAILURE );
  }

  return( (int)bytesRead );
}


//
// NAME
//   serialPutByte - Write a byte to a serial port
//
// SYNOPSIS
//   #include "serial.h"
//
//   int serialPutByte( int fd, char *value );
//
// DESCRIPTION
//   Write a byte pointed to by value to the serial
//   port file descriptor fd.  The file descriptor is
//   assumed to be open and ready for writing.
//
// RETURNS
//   0 or 1 : The number of bytes written.
//
// NOTE It's not a good idea to logPrint in any
//      of these routines as it may delay time
//      critical operations in the larger program.
//
int serialPutByte ( int fd, char value ) {
  int ret;
  
  ret = write( fd, &value, 1 );
  return ret;
}


// 
// NAME
//   serialGetLine - Read a line of data from a serial port
//
// SYNOPSIS
//   #include "serial.h"
//
//   ssize_t serialGetLine( int fd, char *buffer,
//                          long bufsize, long timeout, 
//                          char *lineTerm );
//
// DESCRIPTION
//   Read a line of data delimited by the lineTerm string
//   and store in buffer up to bufsize bytes.  This routine 
//   will wait for up to timeout milliseconds for a complete
//   line to be read before returning.   
//
// RETURNS
//   -1 : Failure
//   0 to bufsize:  The number of bytes read.  
//   NOTE: This routine may return an incomplete line 
//   if a timeout occurs before the line is read or if
//   the line is longer than bufsize bytes.  Checking
//   for lineTerm string at the end of the buffer is
//   one way of checking for this case.
//
// NOTE It's not a good idea to logPrint in any
//      of these routines as it may delay time
//      critical operations in the larger program.
//
ssize_t serialGetLine ( int fd, char *buffer, 
                        long bufsize, long timeout, char *lineTerm ) {
  int ret;
  long bPtr = 0;
  ssize_t bytesRead;
  struct timeval startTime;
  char value;

  // Just some sanity checks
  if ( buffer == NULL || bufsize < 1 ) 
    return ( FAILURE );

  buffer[0] = '\0';
  // Get the time of day for later comparison
  ret = gettimeofday( &startTime, NULL );
  if ( ret == 0 ) {
    do {
      bytesRead = read( fd, &value, 1 );
      if ( bytesRead < 0 ) {
        // Check errno ( EINTR && EAGAIN are ok )
        if ( errno != EINTR && errno != EAGAIN ) {
          return ( FAILURE );
        }
      }else {
        buffer[bPtr++] = value;
        buffer[bPtr] = '\0';
      }
    } while ( bPtr < bufsize-1 &&
              timeout > 0 && 
              ( getMilliSecSince( &startTime ) < timeout ) &&
              strstr(buffer,lineTerm) == NULL );
    LOGPRINT( LVL_DEBG, "serialGetLine(): final duration: %d", 
              getMilliSecSince( &startTime ) );
  }else {
    // Couldn't get time of day???
    return( FAILURE );
  }

  return( (ssize_t)bPtr );
}


// 
// NAME
//   serialPutLine - Write a null termined line to the serial port.
//
// SYNOPSIS
//   #include "serial.h"
//
//   ssize_t serialPutLine( int fd, char *buffer );
//
// DESCRIPTION
//   Write a null terminated ( \0 ) string to the
//   file descriptor ( fd ).  The file descriptor
//   should be previously opened to a serial port.  
//   The file descriptor is not altered in this 
//   function.
//
// RETURNS
//   The number of bytes written to the file descriptor.
// 
// NOTE It's not a good idea to logPrint in any
//      of these routines as it may delay time
//      critical operations in the larger program.
//
ssize_t serialPutLine ( int fd, char *buffer ) {
  ssize_t ret;
  ret = write( fd, buffer, strlen(buffer) );
  return ret;
}


// 
// NAME
//   serialChat - Conduct a serial port "statement/response" communication.
//
// SYNOPSIS
//   #include "serial.h"
//
//   int serialChat( int fd, char *statement, char *response,
//                   long timeout, char *lineTerm);
//  
// DESCRIPTION
//   Send a null terminated string ( statement ) to the serial 
//   port and wait for a specific string in return ( response ).
//   The function will wait for the response for up to timeout
//   milliseconds or until a line termination string ( lineTerm )
//   is received ( whichever comes first ).  
//
// RETURNS
//   -1 Upon failure
//    0 If a response string was received but doesn't match
//      what was expected.
//    1 If the correct response was received. 
//
// NOTE It's not a good idea to logPrint in any
//      of these routines as it may delay time
//      critical operations in the larger program.
//
int serialChat ( int fd, char *statement, char *response, 
                 long timeout, char *lineTerm) 
{
  ssize_t ret;
  char responseBuf[128];

  // Flush SerialIN
  term_flush(fd);

  // Send statement
  ret = write( fd, statement, strlen(statement) );

  // Wait for response
  ret = serialGetLine( fd, responseBuf, 128, timeout, lineTerm );
  if ( ret >= 0 ) {
    // 
    // DEBUG
    //
    //printf("Returned from serialGetLine with %s (%d)\n", responseBuf, ret );
    //int i =0;
    //printf("Returned from serialGetLine with:\n");
    //for ( i = 0; i < ret; i++ ) 
    //  printf("  byte[ %d ] = %x\n", i, responseBuf[i] );
    //printf("Looking for:\n");
    //i =0;
    //while ( response[i] != '\0' )
    //{
    //  printf("  byte[ %d ] = %x\n", i, response[i] );
    //  i++;
    //}
    //
    // Compare response to expected
    if( strstr(responseBuf, response ) ) 
    {
      return( 1 );
    }else {
      return( 0 );
    }
  }

  return( FAILURE );
}

 
// 
// NAME 
//   serialGetData - Read up to nBytes from a serial port.
//
// SYNOPSIS
//   #include "serial.h"
//
//   ssize_t serialGetData( int fd, char *buffer,
//                          long nBytes, long timeout );
//
// DESCRIPTION
//   Read up to nBytes from the open file descriptor ( fd ).
//   Waits up to timeout milliseconds before giving up on
//   the read. 
//
// RETURNS
//   The number of bytes read or -1 upon failure.
// 
// NOTE It's not a good idea to logPrint in any
//      of these routines as it may delay time
//      critical operations in the larger program.
//
ssize_t serialGetData ( int fd, char *buffer, 
                        long nBytes, long timeout ) 
{
  int ret;
  long bPtr = 0;
  ssize_t bytesRead;
  struct timeval startTime;
  char value;

  // Just some sanity checks
  if ( buffer == NULL || nBytes < 1 ) 
    return -1;

  // Get the time of day for later comparison
  ret = gettimeofday( &startTime, NULL );
  if ( ret == 0 ) {
    do {
      bytesRead = read( fd, &value, 1 );
      if ( bytesRead < 0 ) {
        // Check errno ( EINTR && EAGAIN are ok )
        if ( errno != EINTR && errno != EAGAIN ) {
          return( FAILURE );
        }
      }else {
        buffer[bPtr++] = value;
      }
    } while ( bPtr < nBytes &&
              timeout > 0 && 
              ( getMilliSecSince( &startTime ) < timeout ) );
    LOGPRINT( LVL_DEBG, "serialGetData(): final duration: %d", 
              getMilliSecSince( &startTime ) );
  }else {
    // Couldn't get time of day???
    return( FAILURE );
  }

  return( (ssize_t)bPtr );
}


// 
// NAME
//   serialPutData - Write up to nBytes to a serial port.
// 
// SYNOPSIS
//   #include "serial.h"
//
//   ssize_t serialPutData( int fd, char *buffer, long nBytes );
//
// DESCRIPTION
//   Write the contents of buffer ( up to nBytes ) to an
//   open file drescriptor ( fd ).  Presumably fd is open
//   to a serial port.  The file descriptor is left
//   unaltered.
//
// RESULTS
//   The number of bytes written to the file descriptor.
// 
ssize_t serialPutData ( int fd, char *buffer, long nBytes ) 
{
  ssize_t ret;
  ret = write( fd, buffer, nBytes );
  return ret;
}


// 
// NAME
//   usbGetLine - Read a line of data from a usb port
//
// SYNOPSIS
//   #include "serial.h"
//
//   ssize_t usbGetLine( struct sPort *port, char *buffer,
//                          long bufsize, long timeout, 
//                          char *lineTerm );
//
// DESCRIPTION
//   Read a line of data delimited by the lineTerm string
//   and store in buffer up to bufsize bytes.  This routine 
//   will wait for up to timeout milliseconds for a complete
//   line to be read before returning.   
//
// RETURNS
//   -1 : Failure
//   0 to bufsize:  The number of bytes read.  
//   NOTE: This routine may return an incomplete line 
//   if a timeout occurs before the line is read or if
//   the line is longer than bufsize bytes.  Checking
//   for lineTerm string at the end of the buffer is
//   one way of checking for this case.
//
// NOTE It's not a good idea to logPrint in any
//      of these routines as it may delay time
//      critical operations in the larger program.
//
ssize_t usbGetLine ( struct sPort *port, char *buffer, 
                        long bufsize, long timeout, char *lineTerm ) {
  int ret;
  long bPtr = 0;
  ssize_t bytesRead;
  struct timeval startTime;
  char value;
  struct ftdi_context *ftdic;
  
  ftdic = port->ftdiContext;
  if ( ! ftdic )
    return( FAILURE );

  // Just some sanity checks
  if ( buffer == NULL || bufsize < 1 ) 
    return ( FAILURE );

  buffer[0] = '\0';
  // Get the time of day for later comparison
  ret = gettimeofday( &startTime, NULL );
  if ( ret == 0 ) {
    do 
    {
      bytesRead = ftdi_read_data(ftdic, (unsigned char *)&value, 1 );
      if ( bytesRead < 0 ) 
         return ( FAILURE );
      if ( bytesRead > 0 )
      {
        buffer[bPtr++] = value;
        buffer[bPtr] = '\0';
      }
    } while ( bPtr < bufsize-1 &&
              timeout > 0 && 
              ( getMilliSecSince( &startTime ) < timeout ) &&
              strstr(buffer,lineTerm) == NULL );
    LOGPRINT( LVL_DEBG, "usbGetLine(): final duration: %d, bytesRead = %d", 
              getMilliSecSince( &startTime ), (ssize_t)bPtr );
  }else {
    // Couldn't get time of day???
    return( FAILURE );
  }

  return( (ssize_t)bPtr );
}


