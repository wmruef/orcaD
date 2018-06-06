#!/bin/bash
##################################################################
# Modified by Robert Hubley ( 11/2014 )
#  - Log only when something is wrong to cut down on 
#    syslog traffic.
#  - Do not log to files directly instead use logger through
#    the crontab invocation.
# Modified by Robert Cudmore (20131212, 20131220)
# (1) Added ping instead of 
#         if ifconfig $wlan | grep -q "inet addr:" ; then
#             echo "Network is Okay"
# (2) Added a log of date/time we had a dropped connection /home/pi/wifi.ok
# (3) Added email alert when we come back up (startup_mailer.py)
#
# Title:     WiFi_Check
# Author:    Kevin Reed (Dweeber)
#            dweeber.dweebs@gmail.com
# Project:   Raspberry Pi Stuff
#
# Copyright: Copyright (c) 2012 Kevin Reed 
#            https://github.com/dweeber/WiFi_Check
#
# Purpose:
#
# Script checks to see if WiFi has a network IP and if not
# restart WiFi
#
# Uses a lock file which prevents the script from running more
# than one at a time.  If lockfile is old, it removes it
#
##################################################################
# Settings
# Where and what you want to call the Lockfile
lockfile='/var/run/wifiCheck.pid'
# Which Interface do you want to check/fix
wlan='wlan0'
pingip='192.168.0.1'
##################################################################
#echo
#echo "Starting WiFi check for $wlan"
#date
#echo
# Check to see if there is a lock file
if [ -e $lockfile ]; then
   # A lockfile exists... Lets check to see if it is still valid
   pid=`cat $lockfile`
   if kill -0 &>1 > /dev/null $pid; then
      # Still Valid... lets let it be...
      echo "Process still running, Lockfile valid...exiting."
      exit 1
   else
      # Old Lockfile, Remove it
      #echo "Old lockfile, Removing Lockfile."
      rm $lockfile
   fi
fi
# If we get here, set a lock file using our current PID#
#echo "Setting Lockfile"
echo $$ > $lockfile

# We can perform check
#echo "Performing Network check for $wlan"
/bin/ping -c 2 -I $wlan $pingip > /dev/null 2> /dev/null
if [ $? -ge 1 ] ; then
   echo "Network connection down! Attempting reconnection."
   #echo "`date` Network Down ..........." >> /home/pi/wifi.ok
   /sbin/ifdown $wlan
   sleep 5
   /sbin/ifup --force $wlan
   #echo "`date` Network toggled ..........." >> /home/pi/wifi.ok
#else
   #echo "Network is Okay"
   #echo "`date` is Okay" >> /home/pi/wifi.ok
fi

#echo
#echo "Current Setting:"
#/sbin/ifconfig $wlan | grep "inet addr:"
#echo

# Check is complete, Remove Lock file and exit
#echo "process is complete, removing lockfile"
rm $lockfile
exit 0
