/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * serial.h : Header for general purpose serial functions
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
 * These routines are for use with non-blocking, raw, read/write
 * file descriptors. 
 * 
 */
#ifndef _SERIAL_H
#define _SERIAL_H

int serialGetByte( int fd, char *value, long timeout );
int serialPutByte( int fd, char value );
ssize_t serialGetLine( int fd, char *buffer, long bufsize, 
                       long timeout, char *lineTerm );
// Write contents of the buffer ( must be '\0' terminated!!!!! ) to the
// serial port (fd).
ssize_t serialPutLine( int fd, char *buffer );
// Sends the string "statement" to the serial port (fd) and waits up
// to "timeout" milliseconds for a response line ( terminated by
// "lineTerm" characters ).  This line is searched for the "response"
// string and the return value set accordingly.  Returns 1 for success,
// -1 or 0 for failure.
int serialChat( int fd, char *statement, char *response, 
                long timeout, char *lineTerm );
ssize_t serialGetData ( int fd, char *buffer, long nBytes, long timeout );
ssize_t serialPutData ( int fd, char *buffer, long nBytes );

ssize_t usbGetLine ( struct sPort *port, char *buffer,
                     long bufsize, long timeout, char *lineTerm );


#endif
