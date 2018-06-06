#!/bin/sh
#
# Update IP address script for ORCA project
#  -Robert Hubley 4/24/06
#      Updated: 3/17/18
#

# Set account details: 
# Buoy may be: TORCA, HORCA, DORCA, PORCA, NORCA, or CORCA
BUOY=
# See password sheet for current buoy passwords. Special characters
# may not be transmitted correctly using this approach.  Use only 
# a-z, A-Z, and 0-9 in passwords.
PASS=
# HOST must be an IP address no DNS resolution here.  This is the
# address of the server recording our current IP address
HOST=128.208.72.23

# Send updates to server
wget "http://$HOST/cgi-bin/processIPAddress?buoy=$BUOY&pd=$PASS" -O -
