/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * meterwheel.h : Header for meter wheel functions
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
#ifndef _METERWHEEL_H
#define _METERWHEEL_H

struct sPort *getMeterWheelPort();
float readMeterWheelAdjusted( struct sPort *mwPort, double factor );
float readMeterWheelCount( struct sPort *mwPort );

#endif
