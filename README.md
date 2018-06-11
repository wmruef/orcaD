
# ORCA Control and Management Software
                    
## Written by Robert Hubley & Wendi Ruef ( July 2005 )


Overview
========
The Oceanic Remote Chemical Analyzer (ORCA) is an autonomous 
moored profiling system providing real-time data streams of
water and atmospheric conditions. It consists of a profiling 
underwater sensor package with a variety of chemical and 
optical sensors, and a surface mounted weather station, 
solar power system, winch, and custom computer and software 
package equipped with WIFI/cellular communication.

The ORCA buoy controller Daemon (orcaD) is a UNIX server deamon
and utilities which autonomously operate the mechanical systems
of the buoy. 

Design Concepts
===============
  The design of orcaD grew out of experience with the
  initial ORCA control program "WinchSurf" designed by 
  John Dunn and Wendi Ruef ( et al ).  WinchSurf was 
  designed to run on the TattleTale low-power embedded
  processor family in a DOS-like environment.  

  The redesign of the ORCA management software started
  in earnest in the Spring of 2005 and was initially 
  directed by the following design goals:

    1. Develop a data logging and device management 
       application which will run in a non-real-time
       multitasking environment...namely linux.
  
    2. Ease the management burden of the system and
       increase it's robustness.

    3. Allow for easy reconfiguration of intruments.

 
Installation
============
  Dependencies: 
        Hardware:
             BitsyX or BitsyXb XScale CPU
             Keyspan USB 4-port Serial Port Extender
             Custom I/O circuitry -- See Specs  
             
  Installing From A Binary Software Distribution:
     
     1. Extract the contents of the compressed archive distribution
        into the program directory.
 
        Make sure the directory doesn't already exist and if
        it does move it aside as a backup:

        % file /usr/local/orcaD
        /usr/local/orcaD: ERROR: cannot open `/usr/local/orcaD' 
        (No such file or directory)

        or if it exists move it aside
 
        % file /usr/local/orcaD
        /usr/local/orcaD: directory
        % mv /usr/local/orcaD /usr/local/orcaD.old
  
     
        Extract the software in the /usr/local directory:

        % cp orcaD-#.#.tar.gz /usr/local
        % cd /usr/local
        % tar zxvf orcaD-#.#.tar.gz 
        
     2. Check the distribution:

        % cd orcaD
        % ls
        data/  iotest*  logs/  orcactrl*  orcad* orcad.cfg.tmpl readme
        utils/ 

  Installing From A Source Software Distribution:

      1. Extract the contents of the compressed archive distribution.
         ( usually in a source directory seperate from the installation 
           directory ).  Lets say /root/sources/orcaD for this example.
 
        % cp orcaD-src-#.#.tar.gz /root/sources
        % cd /root/sources
        % tar zxvf orcaD-src-#.#.tar.gz 
        
      2. Check the distribution:

        % cd orcaD
        % ls
          general.h    serial.h      util.c       LICENSE.txt
          hydro.c      orcactrl.c    util.h       Makefile
          hydro.h      orcad.c       sigtest.c    version.c
          README       iotest.c      orcad.cfg    term.c            
          bitsyxio.c   log.c         orcad.h      term.h      
          weather.c    bitsyxio.h    log.h        parser.c    
          weather.h    crc.c         meterwheel.c parser.h    
          crc.h        meterwheel.h  profile.c    winch.c
          ctd.c        mytest.log    profile.h    timer.c  
          winch.h      ctd.h         notebook     serial.c  
          timer.h

      3. Remove older object files if they exist:
        % make distclean

      4. Build the distribution:
        % make

      5. Install the program ( note this will overwrite existing versions )
        % make install

  You are ready to use the software.  See the PROCEDURES
  section for information on how to start the daemon up 
  automatically at boot time.


Manual Operations
=================

  The orcactrl program is used to manually operate the buoy
  systems when the automated program ( orcaD ) is *NOT* running.
  It is very important not to run both simultaneously. 

  Orcactrl may be run as a command driven shell or as a
  single use command.  Here is a list of the current 
  command line options:

  usage: orcactrl [-d debuglevel] [-c config file] [command]

   The main ORCA control utility.  NOTE: This utility should only be
   used when the orcaD process has been stopped!
  
   General options:
    -d (debuglevel) - debug level
    -c (config file) - hardware config file
  
   Commands:
    view      config | voltage | weather
              pressure | meterwheel | all       - Retreive buoy state.
    set       minDepth (meters) |
              parkingDepth (meters) |
              maxDepth (meters) |
              debugLevel (number)               - Set various parameters.
    monitor   voltage|pressure|meterwheel|all   - Watch buoy state.
    move      up (target meters)                - Move the package up.
              down (target meters)              - Move the package down.
              up-discretely (target meters)
                            (sample1 meters)
                            (sample2 meters)
                            ...                 - Move the package up discretely.
    profile   (mission name)                    - Run through a profile.
    download  ctd|weather (filename)            - Download data to a file.
  
  

  See PROCEDURES section for detailed descriptions of its use.


Automated Operation
===================

  The orcad program is responsible for running the 
  buoy in un-attended mode.  It has been written as
  a unix "daemon".  A daemon is a program which is
  often started when the operating system boots and
  remains running in the background ( i.e. without
  user interaction through a terminal ) until the
  system is shutdown.   

  Orcad is configured through the use of a config
  file ( defaults to /usr/local/orcaD/orcad.cfg )
  and is started by the operating system process
  management program "init" or systemd (see installation 
  section for details).  Init/systemd has the ability 
  to not only start a program when the system is booted,
  but to also restart it should it automatically should it
  crash at any time.  It is important that orcad
  is started in this fashion as it offers some amount
  of insurance that the winch will be shut off should
  the program crash during a profile.

  Orcad accepts several parameters:

   orcad [-v][-d #][-c conf_file][-f]
   
  Where:
     -c conf_file - configuration file ( default: /usr/local/orcaD/orcad.cfg )
     -d #         - set debug level and flags ( default: 4 )
     -v           - show version info
     -f           - stay in the foreground, do not run as a daemon
     -h           - prints this information

  For testing purposes you can run orcad in the foreground ( allowing
  for terminal control of the process ) by using the -f option.  

  Orcad has the following general program flow:

       1. Startup and become a UNIX daemon
       2. Initialize bitsyX hardware ( shuts off the winch! )
       3. Open up the log file ( defaults to /usr/local/orcaD/logs/orcad.log )
       4. Open up & set the PID file ( defaults to /var/run/orcad.pid )
            ** This makes sure that you don't run more than
               one orcad at one time **
       5. Read the config file ( defaults to /usr/local/orcaD/orcad.cfg )
       6. Go into main loop:

            A. Sleep for a minute 
            B. Check to see if there are profiles to run
              C. Run profiles
            D. Check to see if we should log weather archive
            E. Check to see if we should log weather status
            F. Repeat  

        7. Exit the main loop only if a "kill -QUIT" signal is received
        8. Reset the hardware
        9. Close all the files
       10. exit the program.
              

PROCEDURES
==========

  Setting up Orcad To Be Run By The Init Daemon:

    This procedure will configure the operating system to run
    orcaD when the system is booted.  It will also restart
    the program automatically should it crash or be killed
    by a user ( See procedure for stopping orcaD for details
    on how to shut it down without the init daemon restarting it ).

    1. Edit the init daemon's configuration file /etc/inittab and
       add orcad to it's list of managed processes:

       % nano /etc/inittab

          # Run orcad in runlevel 3
          od:3:respawn:/usr/local/orcaD/orcad -d 8 -f

       You could also create two copies of the inittab file
       in etc...say /etc/inittab_no_orcad  and /etc/inittab_with_orcad
       ( with and without the orcad line above ) and switch between them 
       by simply using a copy command:

       % cp /etc/inittab_with_orcad /etc/inittab

       and

       % cp /etc/inittab_no_orcad /etc/inittab
     

    2. Tell the init daemon to re-read the /etc/inittab file:

       % /sbin/telinit q
 
       Upon re-reading the configuration file the init daemon
       detects that it should start orcad.

    3. Check that orcad has been started correctly:

       % ps -e | grep orcad
       #### ?        00:00:00 orcad
     
       where #### is the process identifier.

       If orcad didn't startup due to some configuration error the
       init daemon will continue trying to restart it.  It does this
       for up to 10 times before figuring out that something is wrong.
       When init figures out that there is a problem it discontinues
       attempts to restart the process and writes a line to the 
       /var/log/messages file indicating the problems as "respawning
       orcad too rapidly".  You will have to look at the log file
       /usr/local/orcaD/logs/orcad.log for more details on why
       orcad is not starting up.  

       Once you have corected the problem go back to step 1 and
       start again.



  Stopping Orcad:

    1. First check that orcaD is running:

        % ps -e | grep orcad
        #### ?        00:00:00 orcad
        
       Where #### is the process identifier.  If orcad is *not*
       running this will return to a command prompt and not
       return anything.

    2. If you are running orcaD using the init daemon you will
       have to disable the "respawn" feature first:

        % nano /etc/inittab

       Find the line which starts orcad.  Might look
       something like this:

          # Run orcad in runlevel 3
          od:3:respawn:/usr/local/orcaD/orcad -d 8 -f
    
       And comment out the command using a # character:

          # Run orcad in runlevel 3
          #od:3:respawn:/usr/local/orcaD/orcad -d 8 -f

    3. Tell the init daemon to re-read the /etc/inittab file:

       % /sbin/telinit q
        
    4. Check that a profile isn't currently running:
  
       % tail -f /usr/local/orcaD/logs/orcad

       This will continuously display the last few lines of
       orcad's log file.  If you see the last message looks 
       something like:

       Apr 01 2010 12:14:41  orcad: runJobs(): Completed mission: Deep_Cast_Direct
 
       Than the buoy is *not* running a profile and it is 
       safe to stop the daemon.

       % ps -e | grep orcad
       10 ?        00:00:00 orcad

       You can kill a process by running the kill command.  For 
       example, using the above example orcad is running as 
       process #10 ( the second column in the output ).  The 
       kill command would be:

       % kill -HUP 10
       %

       It's always a good idea to check that it worked by
       looking at the process list again.

       % ps -e | grep orcad
       %
       
 
    Weatherd ( if used ) is activated in a similar fashion. For the 
    convenience of the administrator we have added four scripts to the
    util/ directory in the orcaD distribution which automate the a few
    of the steps listed above.  Once the inittab is setup you can use
    the scripts:

      util/startOrcad       [ Uncomments initab line and restarts init ]
      util/startWeatherd
    and
      util/stopOrcad        [ Comments inittab line and kills process ]
      util/stopWeatherd



    
Servicing Procedure:

  - Connect computer cable to the computer

  - Login and shutdown the wifi amplifier ( for amplified WIFI buoys )
      % iotest WIFI OFF
    
  - Check orcad's log file:
      % more /usr/local/orcaD/logs/orcad
   
      Check that it's not currently profiling...

  - Check that it's not about to profile ( see setting profile schedule ).
      - Look at schedule in /usr/local/orcaD/orcad.cfg
      - Check current status in the logs "cat /usr/local/orcaD/logs/orcad"

  - Stop orcad ( see procedure above )

  - Run orcactrl if buoy operations are necessary

  - Open up the computer
      - Leave the communications cable attached.

  - halt the proccessor ( this will stop orcad automatically )
     % halt
     NOTE: Once you halt the computer you have approx 60 seconds
           before the hardware ( watchdog timer ) detects the
           computer is down and restarts the system.  I usually
           wait until I have the computer open to halt it.  I
           then unplug the power immediately after the halt
           operation completes.

  - Replace CF card if necessary.
  
  - Reboot ( simply plug power back in and monitor the reboot ).

  - Double check things like:
      - You can log in
      - You can see orcaD running
      - You have checked the startup lines in the log file for errors.
    

Care and Feeding
================

  The orcad package does not require any special 
  administration procedures above and beyond what
  a typical UNIX system does.  With that in mind
  we mention here a few key UNIX administration 
  functions and their impact on the orcad package:

  - Log Rotation
      Typically UNIX systems store log files in the /var/log
      of /var/adm directories.  Orcad currently stores
      its logs in "/usr/local/orcaD/logs/" ( this will become
      a configurable option in future versions).  UNIX log
      files are typically rotated so that sufficient log
      history is maintained with less risk of 
      filling up the filesystem.  Orcad's logs are not
      currently rotated and need to be removed periodically
      to avoid filling up the filesystem.

      See: UNIX logrotate software, UNIX "df" command 

  - RAM & Disk Utilization:
      It's a good idea to periodically check that the
      system has adequate RAM and disk space.  Use
      the UNIX "df" command to check disk space and
      "top" to check RAM utilization.  

      If RAM is low check the process list for abnormally
      high RAM utilization by a single process.  If disk
      space is low check that the logs or data directories
      contain old data which can be removed.
          
  
Data Files
==========

  OrcaD creates several types of data files during normal operations.
  All data files are stored in the programs data subdirectory.  This
  is typically set to /usr/local/orcad/data.  Here is a typical example
  of the files you will find in this directory:

  /usr/local/orcad/data/
                        castIndex.txt
                        weather-status.dat
                        200512/
                               ORCA2_CAST0001.HEX
                               ORCA2_CAST0002.HEX
                               ..
                               ORCA2_MET200512021119.MET
                               ORCA2_MET200512021758.MET
                               ..
                        200601/
                               ORCA2_CAST0100.HEX
                               ORCA2_CAST0101.HEX
                               ..
                               ORCA2_MET200601051119.MET
                               ORCA2_MET200601051758.MET
                               ..
                        [YYYYMM]/
                               [Buoy Prefix]_CAST[####].HEX
                               [Buoy Prefix]_MET[YYYYMMDDHHMM].MET


  The cast index file ( default data/castIndex.txt ) contains the
  integer number of the last cast index used by the orcad program.
  This file is used to save the state of the index between invocations
  of the program.  

  The weather status file ( default data/weather-status.dat ) contains
  the last Davis Weather Station LOOP record.  This file is available
  for download by a weather monitoring package or webservice and is
  updated periodically ( default 10 minutes ). 


MET Files: 
    
   Davis Weather Stations:

     mm/dd/yy hh:mm:00\tTAB_DELIMITED_DATA

      mm = month
      dd = day
      yy = year
      hh = hour
      mm = minute
      TAB_DELIMITED_DATA = barometer ( davis internal value / 1000 )
                           outside humidity
                           average wind speed
                           high wind speed
                           prevailing wind direction
                           rainfall ( davis internal value / 100 )
                           inside temp ( " " " / 10 )
                           outside temp ( " " " / 10 )
                           high outside temp ( " " " / 10 )
                           low outside temp ( " " " / 10 )
                           high solar radiation
                           solar radiation


    AIRMAR Stations:

     mm/dd/yy hh:mm:ss\tTAB_DELIMITED_DATA

      mm = month
      dd = day
      yy = year
      hh = hour
      mm = minute
      ss = second
      TAB_DELIMITED_DATA = barometer  [ inches Hg ]
                           humidity [ percent ]
                           temperature [ degrees celsius ]
                           dewpoint [ degrees celsius ]
                           wind direction - centerline [ degrees True ]
                           wind speed [ knots ]
                           max wind direction - centerline [ degrees True ]
                           max wind speed [ knots ]
                           solar radiation [ Watts/m^s ]
                           avg solar radiation [ Watts/m^s ]
                           latitude [ degrees ]
                           longitude [degrees ]
                           


Revisions
=========
DORCA  0.7.0 ( Slow winch )
  - bitsyX
  - timezone/localtime
    7c7836c1a47b82d402e88495c6001abf  PST8PDT
    1017 Dec 30 2004
HORCA  0.7.0 ( Slow winch -- just slower than the 0.5 thresh )
  - bitsyX
  * Resolved: comm problems due to bad anteanna dir/height
TORCA  0.7.0 ( Slow winch )
  - bitsyXb
  - timezone/localtime
    7c7836c1a47b82d402e88495c6001abf  PST8PDT
    1017 Dec 30 2004
  [ Bad AIRMAR - windspeed always zero ]
NORCA  0.7.0 ( Slow winch )
  -bitsyXb
  - timezone/localtime
    7c7836c1a47b82d402e88495c6001abf  PST8PDT
    1017 Dec 30 2004
  [ Winch intermittent - perhaps control box ]
  * 10/2013: currently down for servicing
CORCA  0.7.0 (Slow winch)
  - bitsyXb
  - No /usr/share/zoneinfo
    /etc/localtime
    c9452f6b9e08d83c6815c38600798964  localtime
PORCA  0.7.0
  - bitsyX
  - timezone/localtime
    7c7836c1a47b82d402e88495c6001abf  PST8PDT
    1017 Dec 30 2004
  [ Bad AIRMAR - windspeed 69 knots all the time ]
  * 10/2013: currently out of the water for servicing

------
New settings for 19+V2
   biowiper=n
   outputexecutedtag=n
------

 0.9.9 - orcad.c     : Sync the CTD time with the computer time at the end
                       of every cast.
         aquadopp.c  : Fixed many communication problems that emerged after
                       firmware updates. It appears that the undocumented
                       recorder commands for obtaining file data leave the
                       Aquadopp in a non-command state. Now obtaining the
                       command prompt before trying to send erase command.
                       It also appears that the timing of sending commands
                       is sensitive.  It works best if we formulate the 
                       command and paramters and send in one transmit 
                       call rather than several calls.

 0.9.8 - pifilling.c : Fixed a potential deadlock situation in the getADValue()
                       code.  We found that some PIFilling boards (v0.3 batch) 
                       have a bad ADC chip ( ports 0-3 ).  The chip reports back
                       a "9c" status when the configuration register is read. The
                       high bit is an indicator that ADC data is ready to be read
                       ( "RDY" active low ).  On these bad boards the RDY is not
                       ever being set low and thus the loop waited indefinitely.  
                       I installed a loop counter that permits up to 5k attempts
                       before quiting and returning with a value of 0.  I am not
                       aware of any condition that would cause these chips to 
                       lock up permanently in this way -- assuming bad chips.
 
                       Updated the modbus/ftdi/usb libraries after developing
                       a new Raspberry PI operating system image.  This new
                       image is compatible with RPI Model 1B, and Model 3B
                       boards and all PI Filling Boards as well as T-Rex boards.
                       
                       The modbustest code has been more appropriately named
                       sunsaver-query.  

                       The standard utility scripts updateIP, wifiCheck,
                       getStatus etc. have been copied to/updated in the utils/
                       directory.

                       start/stopAuxiliaryD.sh have been added to the distribution.
                       And all the start/stop scripts have been updated to switch
                       between inittab and systemctl depending on the system they are
                       installed on.


 0.9.7 - hardio.h    : added definitions for controlling Odroid and Zoocam power
         iotest.c    : added functions for controlling Odroid and Zoocam power
	 parser.c    : added function for ttyUSB serial ports 4-7
	** in "-lowVoltage" version MINEXTVOLT is set to 16.0 volts to account for
	** erroneously low external voltage readings.

 0.9.6 - weatherd.c  : added driver for Soundnine compass and Vaisala 
		       anemometer.

 0.9.5 - weatherd.c  : Wind direction averaging changed from mean to 
	               vector average.

 0.9.4 - winch.h     : Pressure change threshold was lowered in winch.h
	               to accomodate a slower start up of the package 
	               at depth using the new winch controller module, 
		       changed from 0.3 to 0.25.

 0.9.3 - weatherd.c  : magnetic declination changed from (-) to (+).

 0.9.2 - weatherd.c  : PAR conversion equation for UW op-amp board
		       was corrected.

 0.9.1 - weatherd.c  : PAR is now reported in umol instead of watts;
		       changed variable name to eliminate confusion
		       changed file headers to reflect PAR instead of 
                       SOLAR
 0.9.0 - weatherd.c  : weather-status.dat now reports averaged
         weatherd.h    wind speed/direction readings as well 
                       as maximum wind speed/direction.  The
                       wind direction is now corrected for 
                       local magnetic declination.  Support
                       for Licor PAR amplifier is now included.
           
 0.8.9 - weatherd.c  : fixed an error in the conversion
		       between raw counts and wind speed
		       for the RM Young anemometer/compass.

 0.8.8 - weatherd.c  : fixed an error in the baromater
		       conversion between hPa (reported
		       by Gill MetPak) and inhg (calculated
		       for the output file).

 0.8.7 - bitsyxio.c  : Added a backwards compatible 
         bitsyxio.h  : API call for getADVoltage() so
                       that we can compile versions for
                       all computers.
                   
 0.8.6 - profile.c   : If a profile is started and it bombs
         profile.h     out in the setup ( ie. before CTD
         orcad.c       INITLOGGING is performed ) the system
                       would save the same old CTD data to
                       subsequent CAST files. Now it will
                       only attempt to create a cast file if
                       the error happened after INITLOGGING.
                       
 0.8.5 - profile.h   : Created a new hardcoded constant for
                       the minimum external voltage allowed
                       when running a cast.  Typical this is
                       set to 22.0 Volts. Because of a problem
                       we are seeing at NORCA with the battery 
                       voltage measurement being off ( low ) 
                       I have set it to 19.0 Volts in this 
                       version.
 0.8.4 - iotest.c
         hardio.h    : A confusing API for the ADC led to a 
                       bug in the 0.8.3 version.  A new
                       API function getADVoltage was created
                       to clear up the confusion and restore
                       the PAR readings.  Also the "iotest status"
                       function now shows the raw voltages for all
                       the PI Filling ports.
 0.8.3 - profile.c   : Contention for the i2c bus appears to 
                       occasionally cause the A/D reading to 
                       fail and return a negative voltage.  This
                       in turn was causing the profile to error
                       out due to low voltage.  Added some checks
                       for negative values and a second retry 
                       at reading the voltage before erroring out.
 0.8.2 - Makefile    : First version using Raspberry PI cross
                       compiler installed on orcabase.  Also
                       added a new external directory to 
                       contain all external static libraries and
                       header files.
 0.8.1 - ctd.c       : Fixed a problem with new CTD firmware.  The new
                       version adds a field to the DS line.  
                       Also fixed some old code that was supposed to be
                       checking for streaming data after initiating the
                       "STARTNOW" command.  It was *not* checking and allowing
                       the code to continue. With the newer 19Plus firmware
                       it appears there is much more of a delay between 
                       commanding STARTNOW and the start of data streaming.
                       The fixed code now tries for up to 8 seconds ( 2 seconds
                       at a time ) to see if data is coming from the CTD.
 0.8.0 - profile.c   : Print out the actual voltage values when we 
                       have made the choice to abort a profile for low
                       voltage.
         pifilling.c : getScaledADValue() should have been returning 
                       -1.0 when it failed to obtain a value.
         hardio.h    : PAR Sensor ADC port was set to 3.  Should have
                       been 2 ( 0 based ).
 0.7.9 - pifilling.c : IO routines opened I2C device but neglected to
                       close it aftwards. I suspect this ended up 
                       using all the file descriptors and then started
                       to fail to open the device.
 0.7.8 - Improvements for iotest
       - stop/start utils now work on both raspberry PI and Bitsy models

 0.7.7 - Fixes for RaspPi version.

 0.7.6 - Fixed hardware initialization to set the winch
         direction to down.  This keeps the FET off and the
         solenoids off by default. 

 0.7.5 - New weatherd to support new weatherstations

 0.7.4 - Major update
	  
         This includes a build target for the new Raspberry PI
         and PI Filling boards.  Currently there is not a 
         cross-compiler on orcabase so this needs to be compiled
         on a RPI system directly.  

         Also:

             - This version defaults to the slower winch
               setting.  


 0.7.3  - Two versions of this release were created 10/17/2013:

         This version corrects a bug in the previous version where
	 power was toggled off of the hydrowire only after a successful
	 profile; power remained on after a profile erred out.

	 In this version power is toggled in the orcad.c code, instead
	 of the profile.c code.

         The ones marked "SlowWinch" set the Pressure Threshold       
         ( in meter.h ) to 0.2.                 

         orcaD-0.7.3-bitsyXb-SlowWinch.tar.gz ( CORCA, DORCA, TORCA )
         orcaD-0.7.3-bitsyX-SlowWinch.tar.gz ( HORCA ) 


 0.7.2  - Two versions of this release were created 10/17/2013:

          All buoys have been switched from an underwater
	  battery pack to a supercapacitor bank.  Therefore
	  there is no need to keep the power on the cable while
	  we are not profiling.  Power is toggled on at the
	  beginning of the profile, and toggled off at the end
  	  of the profile.

          The ones marked "SlowWinch" set the Pressure Threshold
          ( in meter.h ) to 0.2.

          orcaD-0.7.2-bitsyXb-SlowWinch.tar.gz ( CORCA, DORCA, TORCA )
          orcaD-0.7.2-bitsyX-SlowWinch.tar.gz ( HORCA )
	  
  0.7.1 - NOT A RELEASE VERSION -- for testing

        - Orcactrl now has the option of running despite 
          failure of the hardware initialization routines.
         
        - test ctd_comm ( in orcactrl ) now prints out lots-o-info
          to assist with testing the CTD comm.

        - Added some logging code to weatherd.  It now detects
           when GPS acquisition occurs and reports missing data
           for the WMDA record ( we have seen this problem before
           with bad sensors ).
 
           Added the device number to the "hasSerialDevice()" 
           debugging line.

  0.7.0  - Two versions of this release were created.

          I improved the error reporting to indicate
          what the pressure threshold is and what the observed pressure
          difference is when it logs a EPRST error. 

          The ones marked "SlowWinch" set the Pressure Threshold
          ( in meter.h ) to 0.2.

          orcaD-0.7.0-bitsyX.tar.gz ( HORCA )
          orcaD-0.7.0-bitsyXb-SlowWinch.tar.gz ( TORCA )
          orcaD-0.7.0-bitsyX-SlowWinch.tar.gz ( PORCA )

  0.6.9  - All buoys

           This release comes in three different distributions which
           currently map to buoys as:

              orcaD-0.6.9-bitsyX.tar.gz (DORCA, HORCA, TORCA)
              orcaD-0.6.9-bitsyXb.tar.gz ( CORCA )
              orcaD-0.6.9-bitsyXb-WINCH_DIR_SWITCHED.tar.gz ( PORCA )

           This release contains modbus support for both architectures.
           The TORCA specific changes have been removed.
           Nothing else should be different.

           Built on Friday, May 20th, 2011

  0.6.8  - PORCA specific

           New release with winch direction flip still in place.
           Also now a bitsyXb buoy
 
  0.6.7  - TORCA specific
           The new winch on TORCA is very slow.  Its only running at .19-.2
           pressure change per reading.  Had to change the minimum in the
           winch.h file to make this work.

           Also added a -p flag to the modbustest program.

  0.6.6  - New release for all buoys
              - iotest reading of current state fixed.
              - Meterwheel can be commented out in the cfg file.
              - New "test ctd_comm" program in orcactrl
            
           Produced a version for bitsyX, bitsyXb, and bitsyX with
           the winch direction switched ( for PORCA -- still broken ).

           NOTE: Currently modbus support isn't compiled on the bitsyX 
                 versions.
            
           Installed at CORCA, PORCA, HORCA

  0.6.5  - New release to coincide with CORCA deployment.  
           Changes include:
               - Support for bitsyXb EABI binaries.
               - MODBUS Library support ( for solar charger comm ).
               - Meterwheel comm through USB rather than serial.
               - Li-Cor parameters are now in the config file.
               - USB devices are now configured in the config file.
               - New modbustest/ftditest utilities

           New Arduino meterwheel counter hardware.
  
  0.6.4  - Lowered max log size.  500kb was a bit large for the
           cellular buoys -- especially since this is downloaded
           once an hour.
          
          - Normal winch logic ( NOT PORCA compatible )
          - Meterwheel enabled ( NOT PORCA compatible )

          Deployed to TORCA on 8/13/10

  0.6.3  - Added functions to orcactrl for debugging aquadopp
         - Changes to aquadopp.c to make communcations with the
           aquadopp more robust.
         
         NPB BUOY SPECIFIC RELEASE!!!! The NPB computer still has
         the reversed winch logic.

         Deployed to NPB(PORCA) on July 13th 2010
               
  0.6.2  - Fixed several problems for the NPB buoy.  

           NPB BUOY SPECIFIC RELEASE:

           - For some reason the winch direction was reversed on this
             computer. We need to fix this once the computer returns to
             the shop. There is a workaround in the bitsyxio.h for this
             computer only!  
 
           - The meter wheel counter failed on this computer so there is
             a workaround in winch.c which sets a constant for the 
             counter so that the other routines continue to work correcctly.
             Also adjusted down the sensitivity of the pressure change
             required to keep moving the package.

           GENERAL BUOY/EQUIPMENT FIXES:

           - Fixed a problem with INITLOGGING not being recognized by
             the new 19Plus V2.

           - Found a problem with the new Aquadopp data download.  
             Disabling in the config file for now.  Will look into
             firmware changes so what we can update our driver.
 
           - Weatherd was not updating the MET files at the right
             time.  Fixed.

           Deployed to NBP on April 9th 2010

  0.6.1  - Fixed a problem with static linking of weatherd.  Added
           meterwheel_cfactor parameter to config file.  

  
  0.6.0  - Major release for new hardware.  This release supports
           the new Seabird 19+ V2 CTD.  The device is still known
           as a 19Plus to the software but the support for both
           versions in the driver.  Also we wrote a new daemon called
           weatherd.  This is an independent data logging application
           which currently reads from a AirMar weather station attached
           to a USB port and a solar sensor attached to an A/D pin.
           This is a development release -- not installed on the buoys.

  0.5.9  - Skipped release.  No changes

  0.5.8  - Changed the external voltage threshold from 20 to 22.


  0.5.7  - Fixed a bug in profile.c.  The aquadopp was being started
           even if the config file didn't have the device setup.
           
  0.5.6  - Created a new config file option.  This option is
           set per mission and controls the auxillary 
           sampling instrument ( on the auxilary serial port ).
           if "aux_sample_direction" is set to "up" the auxilary
           instrument will be turned on only for the upcast phase
           ( note this is on,off,on,off during a normal 1.0 cycle ).
           If the parameter is set to "down" sampling will only 
           occur during the downcast.  A third setting of "both" 
           will sample during the complete cast.

           Fixed a bug in the aquadop getAquadoppTime() function.  
           The function sent the RC command and then looked for a
           double "Ack" word to delimit the returned data.  Unfortunately
           in June the month is set to "6" ( the Ack character ) and so
           the returned data is truncated by one byte.  NOTE: This could 
           also have happened when any of the date bytes formed a 
           consecutive "0x06 0x06" set.  Now this reads 8 data bytes
           and doesn't look for delmitter.

           Fixed a minor debug message in getAquadoppTime().
           Fixed a bug in the runJobs() function where the 
           aquaFD file descriptor wasn't opened.
           
           Deployed to HORCA on Jun 14th 2007

  0.5.5  - Changed getAquadoppTime() isdst to -1
         
           Deployed to TORCA on Nov 8th.

  0.5.4  - Added code to power down the aquadopp ( PD command ) 
           at the end of the downloadAquadoppFiles() routine.

           Back in 0.5.1 we found a problem with the getCTD*Time
           routines in which the time was off by an hour.  It
           was determined that the time_t.isdst flag needed to
           be set.  It looks like we should have set this to -1
           instead of 1.  -1 indicates that the status of DST is
           unknown and should be inferred from the date/time 
           and the bitsy's timezone.  This was breaking the getWSTime()
           ( also changed in 0.5.1 ) but oddly not getCTD*Time ( yet ).
           I set the isdst flag to -1 in getWSTime() and fixed this
           problem for now. NOTE: Of course this happened the first
           time we restarted orcad after the Fall DST time change.
           Also changed the isdst flag in getCTD*Time to -1.
           NOTE: Aquadopp is still 1.  Need to consider changing that
           as well.

           Deployed Monday, October 30th at 10:30am to:
             NORCA, DORCA, TORCA, HORCA

  0.5.3  - Added code to set NAVG=1 in the CTD 19+ initialization
           routines.

           Added code to work with a Nortek Aquadopp instrument
           on the second hydro wire communications line.

           Deployed on NORCA with an aquadop on Oct 23rd @ 2pm

  0.5.2  - It appears that TORCA is slow at moving the winch up
           when the battery voltage is low ( ~22v ).  This causes
           the winch routines error out with a low pressure change
           in 5 counts.  It used to be < 1 meter.  I changed
           it to < 0.5.

           I also changed the order of the output pressures
           in the LOGPRINT statements.

           Deployed to TORCA

  0.5.1  - Stupid little bug in the getCTD19PlusTime routine. The
           hour was one off.  
  
           Deployed to TORCA, NORCA, and HORCA July 24th 2006 @ 17

  0.5.0  - New release numbering as we have finally hit a stable
           product. 

           Moved the 19+ time sync'ng code into its own routines.

           Added 19 code to get/set time.

           Made a general syncHydroTime routine to call either
           19 or 19+ code depending on the configuraton. 

           Only set the date/time of devices if they are off
           by alot.

           Modified winch.c: Added code to debug the pressure/meter
             values inside the critical loop at debug level 6 or higher.
             Also changed the way a meter wheel failure is handled.
             We now set a warning flag if the meter wheel count is
             constant for more than 4 scans.  If it's constant for
             more scans and the pressure is not increasing
             than fail out.  Also added code to recognize if the pressure
             goes static.  Static is hard coded as less than 1 meter 
             change in 4 scans. 

           Fixed a problem with parsing the "depths" field in the
           config file. 

           Deployed to HORCA: July 19th 2006 @ 3pm
                       NORCA: July 19th 2006 later

  0.4.26 - Lowered the log rotation limit to 500kb
           
           Added a loging statement to the critical loop
           of the movePackageDown() routine.  At LVL_VERB
           this routine will log every pressure/meter wheel
           count. 

           Modified the logRotate code to rename before it
           closes the logfile.  This allows for error reporting
           on the rename process.

           Deployed to horca on 6/21/2006
           
  0.4.25 - Added "outsideHumidity" to the instant weather report.
           
           ( On all the buoys as of Mon June 19th -- installed 
             way before this date...not sure when ).

  0.4.24 - Previous attempts at retrying failed commands to the ctd
           didn't really improve the situation all that much. 
           After sitting down and manually testing commands over
           the noisy line we determined that the a one second
           delay after a failed command vastly improved the chances
           that the retry would succeed.  This version makes this
           change on the CTD19Plus functions.

           Deployed to NORCA, HORCA

  0.4.23 - Changed ctd19plus INITLOGGING and OUTPUTFORMAT
           commands to 3 retries ( from 2 ).  Hopefully this
           will get around the noisy comm line problems.

           Improved the getCTD19PlusSPrompt routine to 
           be more robust to CTRL-Z failures.

           Deployed on HORCA

  0.4.22 - Intermediate version.  For testing purposes only.

  0.4.21 - Added code to stop CTD logging whenever an error occurs
           in the profile subroutine.  This was leaving the CTD
           on and draining the batteries.

           Added code to sync the CTD19Plus clock with the bitsyX
           during initialization.  Need to write a similar function
           for the CTD19.

           Deployed on HORCA Mar 24 2006 17:06:39

  0.4.20 - Added code to winch.c to decypher the status codes
           coming from the move routines.  Also added the last
           pressure and meterwheel counts to the error status
           output.

           Deployed on Hansville Friday Mar 3rd 2006 @ 14:43

  0.4.19 - Added function to rotate log files when they 
           reach 1 MB.  This enables us to move them
           off the CF card with a download script without
           disturbing the running daemon.  Also added the
           year to log messages.

           Still had problem with iotest.  It had the aliases
           for WINCHDIR reversed! Orcad didn't though.  Thus
           we didn't notice the problem.

           Excessive signal loss in a communications line 
           is causing occasional problems with commands to
           the CTD to be received correctly the first time.
           Added some retry attempts on the CTD 19+ commands
           which are long like "INITLOGGING" and "STARTNOW".
 
           Added the year to the instant weather output. I
           know it's extra data but for so many programs 
           this is really useful to know as you parse the data.

           Deployed on Hoodsport Tuesday Feb 21st 2006 @ 2:30pm
           Deployed on Hansville Wednesday Mar 1st 2006 @ 11am

  0.4.18 - Changed PORTA_DIRMASK to 0x04 from 0x03.  The former
           was wrong and would have allowed writing to one of
           the A/D lines.  This is bad since writing to a an
           A/D line enables pull up resistors thus messing up
           the readings.  Unfortunately this is not the cause
           of our current voltage reading problem but worth
           changing non-the-less.

           Found problem with A/D pullup resistors.  It was 
           in setOutputLine().  The bitmask procedure was
           only protecting writing over I/O line states.  If
           PORTA was mixed I/O and A/D *and* the A/D port had
           a voltage on it, the subroutine was attempting to
           preserve the logic "1" on this line when changing
           other lines on the same port.  I introduced a 
           PORTA_ADMASK for this port which always make sure
           that a zero is sent to the A/D lines being used.

           Added INTVOLT and EXTVOLT commands to iotest so one
           may query the system voltage without too much of 
           a headache.

           Deployed on the North Buoy on Jan 30th, Deployed
           on Hoodsport on Jan 26th.

  0.4.17 - Added a last goodbye message at LVL_ALRT so that we
           can ensure that all cleanup duties have been run.  Somtimes
           the process does not quit cleanly ( sits there with DW status ).
           In situations like that this message will inform us if it
           at least finished all of its tasks.
         
         - Added "pressureDepth" to the logging at the end of the critical
           loop in movePackageDown/Up().  This will help us debug why we
           are sometimes leaving this loop before reaching the target
           depth.  Also added code to print out the 4 pressure/meterwheel
           values if we are in debug level logging or if something strange
           with the pressure occurs.

           Deployed on Hoodsport on Jan 23rd.

  0.4.16 - Added date/time and 10 minute avg windspeed
           to the instant weather log.

           Use CTRL-Z to stop the logging on the
           CTD 19+ per Seabird's suggestion.

           Added code to orcad's cleanup routine to 
           power off all devices and save the weather
           archive on shutdown.

           Worked around a problem with ADS's non-standard
           read behaviour in iotest.c  I had already employed
           this in bitsyxio.c but hadn't worked it back into
           iotest.  Perhpaps this will fix the line status
           problem.

           Fixed the units of rainfall in the archive download.
           To obtain in/hr you need to divide by 100.
 
           Fixed readLastCastInfo().  It was not returning the
           last cast number and instead was always returning 1.

           Deployed at Hoodsport only.

  0.4.15 - Documentation changes.
           
           The ability to download the last archive
           from the Davis Weather Station is flaky.
           So for instant web-based weather data this
           version uses the logInstantWeather routines 
           instead.

           Deployed this version at hoodsport on Jan 3rd 2006.

  0.4.14 - Changed CELL power i/o port to WIFI Amplifier
           power in bitsyxio.h and iotest.c

           Deployed this version at hoodsport and north buoy
           on November 17 and 18th respectively.
           

  0.4.13 - Moved cast index file routines to the util.c file.
    
           Decreased winch resolution to 1m.  Hysterisis
           is causing a delay up to .5 meters.

           Changed how a failure status from stopHydroLogging
           is treated in the profile subroutine.  Instead of
           propogating the failure up ( causing it not to
           attempt to download the data ) we log it and 
           return success.

           Fixed a bug with the downloading of MET data.  The
           daemon was assuming that upon startup it was ready
           to download the archive. 

           Changed .MET file format to be a tab delimited record
           format.

           This version was never deployed...you know...
           superstition and all...


  0.4.12 - Deployed Monday Oct 31st - Halloween Day on the
           Hoodsport buoy.

           Fixed a bug in the bitsyxio.c::emergencyPortClear() 
           routine.  The routine was incorrectly identifying an
           open system call return status as bad when it was good.
 
           Made config file parse more robust ( ie. name/value
           pairs with/without spaces. Added code to print out
           a line number for syntax errors.

           Added sync() to the logPrint function.  We have observed
           that the fflush() is not always sufficient to commit 
           the data to the disk.

           Fixed problem with profile loop where attempt is made
           to move the package up to a location it is already at.

           Changed up/down movement position tolerance from
           0.2m resolution to 0.5m.  This is due to the faster
           speed of this winch and the hysteris of shutting it
           off.           
           
           Added code to initialize the CTD 19 Plus.  Also worked
           on getCTD19PlusSPrompt to make it more robust.
 
           Added an extra check in the getCTD19PlusSPrompt() for
           the successful halt to logging.  The old code appeared
           to halting the logging but thinking it didin't.
           
           Documentation changes.


  0.4.11 - First version to see the light of day.  This release
           was placed on the Hoodsport buoy for one week on
           Oct 19th 2005.  

 

Contacts
========
  Robert Hubley <rhubley@gmail.com>
  Wendi Ruef <wruef@ocean.washington.edu>


