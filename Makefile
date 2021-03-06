#####################################################################
# orcaD - ORCA Buoy Management System
#         Oceanic Remote Chemical Analyzer
#
# Makefile : GNU Makefile for orcaD project
#
# Created: August 2005
#
# Authors: Robert Hubley <rhubley@gmail.com>
#          Wendi Ruef <wruef@ocean.washington.edu>
# 
# See LICENSE for conditions of use.
#
# $Id$
#
#####################################################################
# $Log$
#
####################################################################
#
#  To build orcaD:
#    1. Modify the parameters in this section to meet
#       your needs.
#    2. Run: make
#    3. Run: make install
#    4. Run: make distclean
#
# Set the version here
VERSION = 0.9.11

# Set the installation directories here
#
# The top-level directory to create program directory "orcaD" in
prefix = /usr/local
exec_prefix = $(prefix)/orcaD

# Where to put the executable for the command `gcc'.
bindir  = $(exec_prefix)

# Where to put the logs generated by the program
logdir  = $(exec_prefix)/logs

# Where to put data generated by the program
datadir = $(exec_prefix)/data


# Platform.  Currently we support both the bitsyX and the
# newer bitsyXb platforms.  Please specify which here.
#PLATFORM=bitsyX
#PLATFORM=bitsyXb
PLATFORM=raspPi

############## DO NOT EDIT BELOW THIS LINE #######################
##################################################################

ifeq ($(PLATFORM),bitsyX)
  #
  # Of course we use GCC
  #
  # If we are building on a full debian system running on
  # the bitsyX then:
  #
  # CC = gcc
  #
  # If we are lucky enough to have a bounds checking compiler
  # then:
  #
  # CC = /local/bgcc/bin/gcc
  # 
  # Otherwise we are running a cross-platform build using the
  # standard toolchain installed here:
  CC = /usr/local/arm/3.3.2/bin/arm-linux-gcc

  # CFLAGS: To use the bounds checking compiler 
  #         use the following options:
  #
  # CFLAGS = -Wall -O2 -fbounds-checking 
  #
  #CFLAGS = -Wall -O2 -DBITSY -I. -I/usr/local/include -I/usr/local/include/libftdi1 -I/usr/include/libusb-1.0  -lusb-1.0
  CFLAGS = -Wall -O2 -DBITSY -I. -Iftdi/linux-2.4.27-abi/include -Imodbus/linux-2.4.27/include/modbus
  
  # LDFLAGS
  LDFLAGS = 

  #
  # Extra pre-compiled libraries for USB/FTDI communications
  #
  MODBUSOBS = modbus/linux-2.4.27/lib/libmodbus.a 
  FTDIOBS = ftdi/linux-2.4.27-abi/lib/libftdi.a \
            ftdi/linux-2.4.27-abi/lib/libusb.a
  PRGMS = iotest orcad orcactrl weatherd ftditest sunsaver_query auxiliaryd
  IOOBJS = bitsyxio.o
endif
ifeq ($(PLATFORM),bitsyXb)
  #
  # Of course we use GCC
  #
  # If we are building on a full debian system running on
  # the bitsyXb then:
  #
  # CC = gcc
  #
  # If we are lucky enough to have a bounds checking compiler
  # then:
  #
  # CC = /local/bgcc/bin/gcc
  # 
  # Otherwise we are running a cross-platform build using the
  # standard toolchain installed here:
  #
  CC = /usr/local/arm/arm-2008q1/bin/arm-none-linux-gnueabi-gcc

  # CFLAGS: To use the bounds checking compiler 
  #         use the following options:
  #
  # CFLAGS = -Wall -O2 -fbounds-checking 
  #
  CFLAGS = -Wall -O2 -DBITSY -I. -Iftdi/linux-2.6.24.5-eabi/include \
           -Imodbus/linux-2.6.24.5-eabi/include/modbus
  
  # LDFLAGS
  LDFLAGS = 

  #
  # Extra pre-compiled libraries for USB/FTDI communications
  #
  FTDIOBS = ftdi/linux-2.6.24.5-eabi/lib/libftdi.a \
            ftdi/linux-2.6.24.5-eabi/lib/libusb.a

  MODBUSOBS = modbus/linux-2.6.24.5-eabi/lib/libmodbus.a 
  PRGMS = orcad iotest orcactrl weatherd ftditest sunsaver_query auxiliaryd
  IOOBJS = bitsyxio.o
endif
ifeq ($(PLATFORM),raspPi)
  #
  # Of course we use GCC
  #
  # If we are building on a full debian system running on
  # the raspberry PI then:
  #
  # CC = gcc
  #
  # If we are lucky enough to have a bounds checking compiler
  # then:
  #
  # CC = /local/bgcc/bin/gcc
  # 
  # Otherwise we are running a cross-platform build using the
  # standard toolchain installed here:
  #
  CC = arm-unknown-linux-gnueabi-gcc

  # CFLAGS: To use the bounds checking compiler 
  #         use the following options:
  #
  # CFLAGS = -Wall -O2 -fbounds-checking 
  #
  #CFLAGS = -Wall -O2 -mfloat-abi=hard -DRASPPI -I. -Iexternal/rpi-3.6.11/include
  CFLAGS = -Wall  -O2 -mfloat-abi=hard -DRASPPI -I. -Iexternal/rpi-4.5.59/include/modbus -Iexternal/rpi-4.5.59/include

  
  # LDFLAGS
  LDFLAGS = -lpthread -lrt 

  #
  # Extra pre-compiled libraries for USB/FTDI communications
  #
  #FTDIOBS = external/rpi-3.6.11/lib/libftdi1.a external/rpi-3.6.11/lib/libusb-1.0.a external/rpi-3.6.11/lib/libudev.a
  #FTDIOBS = external/rpi-4.5.59/lib/libftdi1.a external/rpi-4.5.59/lib/libusb-1.0.a external/rpi-4.5.59/lib/libudev.a
  FTDIOBS = external/rpi-4.5.59/lib/libftdi.a external/rpi-4.5.59/lib/libusb.a 
  #
  #MODBUSOBS = external/rpi-3.6.11/lib/libmodbus.a
  MODBUSOBS = external/rpi-4.5.59/lib/libmodbus.a
  #
  # Currently I am leaving off ftditest as it was written for an
  # older version of the libftdi library.  Once we have done away
  # with the bitsyX versions we can update ftditest.  Mostly it was
  # around to handle comunications with a slave arduino processor
  # which we don't use anymore anyway.
  #
  #PRGMS = orcad iotest orcactrl weatherd ftditest sunsaver_query auxiliaryd
  #
  PRGMS = orcad iotest orcactrl weatherd sunsaver_query auxiliaryd
  IOOBJS = pifilling.o

endif

#
# install program
#
INSTALL = /usr/bin/install -c

#
#
#
####################################################################


ORCAD_OBJS = orcad.o log.o parser.o $(IOOBJS) buoy.o ctd.o \
             serial.o term.o meterwheel.o timer.o \
             winch.o profile.o util.o weather.o crc.o \
             hydro.o version.o aquadopp.o $(FTDIOBS)

IOTEST_OBJS = iotest.o $(IOOBJS)

ORCACTRL_OBJS = orcactrl.o $(IOOBJS) buoy.o log.o term.o parser.o \
                ctd.o meterwheel.o serial.o timer.o winch.o \
                profile.o weather.o crc.o hydro.o version.o \
                aquadopp.o util.o $(FTDIOBS)

WEATHERD_OBJS = weatherd.o $(IOOBJS) log.o version.o util.o \
                parser.o buoy.o term.o hydro.o ctd.o serial.o \
                timer.o aquadopp.o weather.o crc.o $(FTDIOBS)

AUXILIARYD_OBJS = auxiliaryd.o $(IOOBJS) log.o version.o util.o \
                parser.o buoy.o term.o hydro.o ctd.o serial.o \
                timer.o aquadopp.o weather.o crc.o $(FTDIOBS)

SUNSAVER_QUERY_OBJS = sunsaver_query.o $(MODBUSOBS)

FTDITEST_OBJS = ftditest.o $(FTDIOBS)


all: $(PRGMS)

version.c: Makefile
	echo "char const* Version = \"$(VERSION)\";" > version.c

# rule for orcad
orcad: $(ORCAD_OBJS) Makefile
	$(CC) $(CFLAGS) $(ORCAD_OBJS) -o orcad -lm $(LDFLAGS) 

# rule for iotest
iotest: $(IOTEST_OBJS) Makefile
	$(CC) $(CFLAGS) $(IOTEST_OBJS) -o iotest 

# rule for orcactrl 
orcactrl: $(ORCACTRL_OBJS) Makefile
	$(CC) $(CFLAGS) $(ORCACTRL_OBJS) -o orcactrl -lm $(LDFLAGS)

# rule for weatherd
weatherd: $(WEATHERD_OBJS) Makefile
	$(CC) $(CFLAGS) $(WEATHERD_OBJS) -o weatherd -lm $(LDFLAGS) 

# rule for auxiliaryd
auxiliaryd: $(AUXILIARYD_OBJS) Makefile
	$(CC) $(CFLAGS) $(AUXILIARYD_OBJS) -o auxiliaryd -lm $(LDFLAGS)

# rule for ftditest
ftditest: $(FTDITEST_OBJS) Makefile
	$(CC) $(CFLAGS) $(FTDITEST_OBJS) -o ftditest $(LDFLAGS)

# rule for sunsaver_query
sunsaver_query: $(SUNSAVER_QUERY_OBJS) Makefile
	$(CC) $(CFLAGS) $(SUNSAVER_QUERY_OBJS) -o sunsaver_query $(LDFLAGS)


clean:
	rm -f *.o *~ core*

distclean: clean
	rm -f $(PRGMS)
	-rm -rf dist

install: all
	-mkdir $(bindir)
	$(INSTALL) orcad $(bindir)/orcad
	$(INSTALL) weatherd $(bindir)/orcad
	$(INSTALL) orcactrl $(bindir)/orcactrl
	$(INSTALL) iotest $(bindir)/iotest
	$(INSTALL) auxiliaryd $(bindir)/auxiliaryd
	-mkdir $(datadir)
	-mkdir $(logdir)


distfile: all
	-mkdir dist 
	-mkdir dist/orcaD
	-mkdir dist/orcaD/data
	-mkdir dist/orcaD/logs
	-mkdir dist/orcaD/utils
	$(INSTALL) README.md dist/orcaD
	$(INSTALL) orcad dist/orcaD
	$(INSTALL) weatherd dist/orcaD
	$(INSTALL) orcactrl dist/orcaD
	$(INSTALL) iotest dist/orcaD
	$(INSTALL) auxiliaryd dist/orcaD
	$(INSTALL) orcad.cfg.tmpl dist/orcaD
	$(INSTALL) sunsaver_query dist/orcaD/utils
	-$(INSTALL) ftditest dist/orcaD/utils
	$(INSTALL) utils/startOrcad.sh dist/orcaD/utils
	$(INSTALL) utils/stopOrcad.sh dist/orcaD/utils
	$(INSTALL) utils/startWeatherd.sh dist/orcaD/utils
	$(INSTALL) utils/stopWeatherd.sh dist/orcaD/utils
	$(INSTALL) utils/startAuxiliaryd.sh dist/orcaD/utils
	$(INSTALL) utils/stopAuxiliaryd.sh dist/orcaD/utils
	(cd dist; tar zcvf orcaD-$(VERSION)-$(PLATFORM).tar.gz orcaD)
	

distribution: distfile
	cp dist/orcaD-$(VERSION)-$(PLATFORM).tar.gz ..
	make distclean
	(cd ..; tar zcvf orcaD-src-$(VERSION).tar.gz orcaD)

backup:
	cd ..; tar zcvf orcaD-rpi-backup.tar.gz orcaD/
