#!/usr/bin/perl
##
## Perl script to test the new ORCA computer system
## -Robert Hubley 11/2014
##
use strict;



my $rPIHardwareRev = {
  "0002" => "Model B Revision 1.0, 256MB",
  "0003" => "Model B Revision 1.0 + ECN0001 (no fuses, D14 removed),	256MB",
  "0004" => "Model B Revision 2.0, 256MB",
  "0005" => "Model B Revision 2.0, 256MB",
  "0006" => "Model B Revision 2.0, 256MB",
  "0007" => "Model A, 256MB",
  "0008" => "Model A, 256MB",
  "0009" => "Model A, 256MB",
  "000d" => "Model B Revision 2.0, 512MB",
  "000e" => "Model B Revision 2.0, 512MB",
  "000f" => "Model B Revision 2.0, 512MB",
  "0010" => "Model B+, 512MB",
  "0011" => "Compute Module, 512MB",
  "0012" => "Model A+, 256MB",
  "a01041" => "Pi 2 Model B (Sony,UK), 1GB",
  "a21041" => "Pi 2 Model B (Embest, China), 1GB",
  "a21042" => "Pi 2 Model B (with BCM2837, Embest, China), 1GB",
  "900092" => "PiZero, 512MB (no camera connector)",
  "900093" => "PiZero, 512MB (camera connector)",
  "a02082" => "Pi 3 Model B (Sony, UK), 1GB",
  "a22082" => "Pi 3 Model B (Embest, China), 1GB",
  "9000c1" => "PiZero W, 512MB",
  "a020d3" => "Pi 3 Model B+ (Sony, UK), 1GB"
};

print "Status of ORCA Buoy Systems\n";
print "===========================\n";

open IN,"cat /proc/cpuinfo|" or die;
my $revision;
while ( <IN> )
{
  if ( /^Revision\s*:\s+(\S+)/ )
  {
    $revision = $1;
  }
}
close IN;
if ( $revision ne "" )
{
  if ( exists $rPIHardwareRev->{$revision} )
  {
    print "CPU: " . $rPIHardwareRev->{$revision} . "\n";
    if ( $revision =~ /a02082|a22082|a020d3/ )
    {
      print "  NOTE: Pi 3 Models have built in wifi chips.\n";
      print "        Do not install the USB wifi adapter on\n";
      print "        these computers.\n";
    }
  }
}
print "\n";

print "System Power\n";
print "------------\n";
system("iotest INTVOLT");
system("iotest EXTVOLT");
system("./getVoltsAmps.py");
print "\n";

print "Temperature\n";
print "-----------\n";
print "Rasp Pi CPU Temp:";
system("/opt/vc/bin/vcgencmd measure_temp");
print "\n";

print "Hardware Checks\n";
print "---------------\n";
my $retVal = `lsusb`;
my $hasKeyspan = "";
my $hasWireless = "";
while ( $retVal =~ /([^\n\r]+)[\n\r]/ig )
{
  my $line = $1;
  $hasKeyspan = $line if ( $line =~ /keyspan/i );
  $hasWireless = $line if ( $line =~ /Wireless/i );
}
print "USB Wireless Adapter                  : ";
if ( $hasWireless ne "" )
{
  print "Ok\n";
}else
{
  print "Missing\n";
}
print "USB Keyspan                           : ";
if ( $hasKeyspan ne "" )
{
  print "Ok\n";
}else
{
  print "Missing\n";
}

print "Keyspan devices ttyUSB0-3             : ";
if ( -e "/dev/ttyUSB0" && -e "/dev/ttyUSB1" &&
     -e "/dev/ttyUSB2" && -e "/dev/ttyUSB3" )
{
  print "Ok\n";
}else
{
  print "Missing\n";
}

$retVal = `i2cdetect -y 1`;
#
# Correct I2C address setup for Pi Filling board
#
#     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
# 00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
# 10: -- -- -- -- -- -- -- -- -- -- -- UU -- -- -- -- 
# 20: -- -- -- -- 24 -- -- -- -- -- -- -- -- -- -- -- 
# 30: -- -- -- -- -- -- -- -- -- -- -- UU -- -- -- -- 
# 40: 40 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
# 50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
# 60: -- -- -- -- -- -- -- -- UU -- 6a -- 6c -- -- -- 
# 70: -- -- -- -- 74 -- -- --                         
my @colLbl = qw( 0 1 2 3 4 5 6 7 8 9 a b c d e f );
my %hasI2CAddr = ();
while ( $retVal =~ /([^\n\r]+)[\n\r]/ig )
{
  my $line = $1;
  my $msb = 0;
  my $lsb = 0;
  if ( $line =~ /^(\d)\d: (.*)/ )
  {
    $msb = $1;
    my @cols = unpack("A3 A3 A3 A3 A3 A3 A3 A3 A3 A3 A3 A3 A3 A3 A3 A3", $2 ); 
    my $colIdx = 0;
    foreach my $col ( @cols )
    { 
      if ( $col eq "UU" )
      {
        $lsb = $colLbl[$colIdx];
        $hasI2CAddr{ $msb . $lsb . "U" }++; 
      }else
      {
        $hasI2CAddr{ $col }++; 
      }
      $colIdx++;
    }
  }
}
# Typical assignments:
# 24 - IO Pi 32 Chip #
# 68 - Chronodot RTC  ( or UU )
# 74 - I2c Mux
# 6A - ADC Chip #1 ( 0x68-0x6F )
# 6C - ADC Chip #2 ( 0x68-0x6F )
# 40 - Voltage Shunt
print "Pi Filling - IO Pi 32 Chip            : ";
if ( $hasI2CAddr{ "24" } )
{
  print "Ok\n";
}else
{
  print "Missing\n";
}
print "Pi Filling - Chronodot Real Time Clock: ";
if ( $hasI2CAddr{ "68U" } )
{
  print "Ok\n";
}else
{
  print "Missing\n";
}
print "Pi Filling - I2C Mux Chip             : ";
if ( $hasI2CAddr{ "74" } )
{
  print "Ok\n";
}else
{
  print "Missing [ newer PI-Filling boards do not have this chip ]\n";
}
print "Pi Filling - ADC Chip #1              : ";
if ( $hasI2CAddr{ "6a" } )
{
  print "Ok\n";
}else
{
  print "Missing\n";
}
print "Pi Filling - ADC Chip #2              : ";
if ( $hasI2CAddr{ "6c" } )
{
  print "Ok\n";
}else
{
  print "Missing\n";
}
print "Pi Filling - Shunt Chip               : ";
if ( $hasI2CAddr{ "40" } )
{
  print "Ok\n";
}else
{
  print "Missing\n";
}

print "\n";

print "Networking\n";
print "----------\n";
my $gwAddr;
my $wDev = "wlan0";
$retVal = `iwconfig wlan0 | grep ESSID`;
if ( $retVal =~ /^\s*$/ )
{
  $wDev = "wlan1";
  $retVal = `iwconfig wlan1 | grep ESSID`;
}
if ( $retVal !~ /^\s*$/ )
{
  $retVal =~ s/.*ESSID:(.*)/$1/;
  $retVal =~ s/[\n\r]+//g;
  print "$wDev association:  $retVal\n";
  $retVal = `ifconfig $wDev | grep "inet addr:"`;
  $retVal =~ s/^\s+(\S.*)/$1/g;
  $retVal =~ s/[\n\r]+//g;
  print "$wDev IP Addr    :  $retVal\n";
  $retVal = `route -n | grep 'UG[ \t]' | awk '{print \$2}'`;
  print "$wDev Gateway    :  $retVal";
  $gwAddr = $retVal;
  $gwAddr =~ s/[\n\r\s]//g;
  $retVal = `ping -c 2 $retVal`;
  print "$wDev Gateway Ping:\n";
  print "$retVal\n";
}else
{
  print "Could not find wireless device using iwconfig!\n";
}
# TODO:
# iwlist wlan0 scanning | grep orcawifi
#


print "\n";
print "Configuration\n";
print "-------------\n";
print "OrcaD                 : ";
if ( -s "/etc/inittab" )
{
  $retVal = `grep orcad /etc/inittab`;
  if ( $retVal =~ /^#od:23/ || $retVal eq "" )
  {
    print "Disabled\n";
  }else
  {
    print "Enabled\n";
  }
}else {
  $retVal = `systemctl --no-pager status orcad.service`;
  if ( $retVal =~ /enabled;/ )
  {
    print "Enabled\n";
  }else {
    print "Disabled\n";
  }
}
print "WeatherD              : ";
if ( -s "/etc/inittab" )
{
  $retVal = `grep weatherd /etc/inittab`;
  if ( $retVal =~ /^#wd:23/ || $retVal eq "" )
  {
    print "Disabled\n";
  }else
  {
    print "Enabled\n";
  }
}else {
  $retVal = `systemctl --no-pager status weatherd.service`;
  if ( $retVal =~ /enabled;/ )
  {
    print "Enabled\n";
  }else {
    print "Disabled\n";
  }
}
print "WIFI Router Power     : ";
if ( -s "/etc/inittab" )
{
  $retVal = `grep WIFI /etc/inittab`;
  if ( $retVal =~ /^#wf:23/ || $retVal eq "" )
  {
    print "Disabled at reboot\n";
  }else
  {
    print "Enabled at reboot\n";
  }
}else{
  $retVal = `systemctl --no-pager status wifi-power.service`;
  if ( $retVal =~ /enabled;/ )
  {
    print "Enabled\n";
  }else {
    print "Disabled\n";
  }
}

print "rebootRouter.sh Script: ";
$retVal =`grep rebootRouter.sh /etc/crontab`;
if ( $retVal =~ /^#/ || $retVal eq "" )
{
  print "Disabled in chrontab\n";
}else
{
  print "Enabled in chrontab\n";
}
print "updateIP.sh Script    : ";
$retVal =`grep updateIP.sh /etc/crontab`;
if ( $retVal =~ /^#/ || $retVal eq "" )
{
  print "Disabled in chrontab\n";
}else
{
  print "Enabled in chrontab ( check script user/pw settings )\n";
}
print "wifiCheck.sh Script   : ";
$retVal =`grep wifiCheck.sh /etc/crontab`;
if ( $retVal =~ /^#/ || $retVal eq "" )
{
  print "Disabled in chrontab\n";
}else
{
  print "Enabled in chrontab\n";
  $retVal = `fgrep pingip /usr/local/bin/wifiCheck.sh`;
  if ( $retVal =~ /(\d+\.\d+\.\d+\.\d+)/ )
  {
     if ( $gwAddr ne "" )
     {
       if ( $gwAddr ne $1 )
       {
         print "  - WARNING: wifiCheck.sh gateway = $1 is not the same as wlan0 ( $gwAddr )\n";
       }else
       {
         print "  - gateway = $1\n";
       }
     }else
     {
       print "  - gateway = $1\n";
     }
  }
}
print "\n";
print "Logs\n";
print "----\n";
$retVal = `fgrep -c "wifiCheck: Network connection down" /var/log/syslog`;
print "# WIFI resets: $retVal";
$retVal = `fgrep "wifiCheck: Network connection down" /var/log/syslog | tail -n 1 | awk '{ print \$1 " " \$2 " " \$3 }'`;
print "  -- last attempt: $retVal";


print "\n";
