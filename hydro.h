/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * hydro.h : Abstract interface to underwater package
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
#ifndef _HYDRO_H
#define _HYDRO_H 

//
// Prototypes
//
//    initHydro - Initialize the hydro device for general operations.
int initHydro ( int hydroDeviceType, int hydroFD );
//   stopHydroLogging - Cease logging of hydrowire device
int stopHydroLogging ( int hydroDeviceType, int hydroFD );
//   startHydroLogging - Initiate logging on a hydro wire device
int startHydroLogging ( int hydroDeviceType, int hdroFD );
//   getHydroPressure - Read the pressure from the hydro-wire data stream
double getHydroPressure ( int hydroDeviceType, int hydroFD );
//   downloadHydroData - Download data archives from hydro device
int downloadHydroData ( int hydroDeviceType, int hydroFD, FILE * outFile );
//   syncHydroTime - Sync the CTD clock with ours
int syncHydroTime ( int hydroDeviceType, int hydroFD );

#endif
