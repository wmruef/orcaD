/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * buoy.h : Header for buoy hardware routines
 *
 * Created: August 2010
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
#ifndef _BUOY_H
#define _BUOY_H

int initializeHardware();
int closeDeviceFileDescriptor( int deviceType );
int getDeviceFileDescriptor( int deviceType );
int getHydroWireDeviceType();
struct sPort *getSerialDevice( int deviceType );
int hasSerialDevice( int deviceType );
struct ftdi_context * getUSBDeviceContext( int deviceType );


#endif
