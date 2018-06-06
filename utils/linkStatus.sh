#!/bin/bash
#
#  A simple little script to monitor the wifi link
#  status.  Each time its called it outputs to stdout
#  lines like:
#
#     Thu Mar 29 10:35:01 PST 2012   Link Quality=49/70 Signal level=-46 dBm 
#     Noise level=-95 dBm
#
#  To monitor the link status of the buoy over time place 
#  a call to this script in the /etc/crontab file.  Here
#  is an example which calls this script every 10 minutes:
#
#     0,10,20,30,40,50  * * * * root /usr/local/orcad/utils/linkStatus.sh >> 
#     /usr/local/orcad/logs/link.out 2>/dev/null
#
#  The output will be saved to logs/link.out and can be viewed
#  or downloaded at a later time.
#
TT=`iwconfig | grep Quality`
DD=`date`

# orcabase
ping -c 5 128.208.72.12>>/dev/null

if [ $? -eq  0 ]
then
SS="Ping to orcabase succeeded"
else
SS="Ping failure"
fi

echo ${DD}  " " ${TT} " " ${SS}

