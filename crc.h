/*********************************************************************
 * orcaD - ORCA Buoy Management System
 *         Oceanic Remote Chemical Analyzer
 *
 * crc.h : Header for CRC calculation functions
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
#ifndef _CRC_H
#define _CRC_H 1

int crc16AddByte(unsigned char data, unsigned int crc);

int crc16AddData(unsigned char *data, long numBytes, unsigned int crc);

#endif


