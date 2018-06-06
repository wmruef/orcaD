#!/usr/bin/perl
use strict;


my @dataFieldDescs = ( "Barometer", 
                       "Outside Humidity", 
                       "Average Wind Speed", 
                       "High Wind Speed",
                       "Prevailing Wind Direction",
                       "Rainfall",
                       "Inside Temperature",
                       "Outside Temperature",
                       "High Outside Temperature",
                       "Low Outside Temperature",
                       "High Solar Radiation",
                       "Solar Radiation" );
my @dataFieldUnits = ( "Hg", 
                       "%", 
                       "mph", 
                       "mph",
                       "",
                       "in/hr",
                       "F",
                       "F",
                       "F",
                       "F",
                       "watts/m^2",
                       "watts/m^2" );

while ( <> ) 
{
  if ( /(\d+)\/(\d+)\/(\d+)\s+(\d+):(\d+):00\s+(.*)/ ) 
  {
    my $month = $1;
    my $day = $2;
    my $year = $3;
    my $hour = $4;
    my $minute = $5;
    print "$month/$day/$year $hour:$minute:00\n";
    print "===================\n";
    my @data = split(/\s+/, $6);
    for ( my $i = 0; $i <= $#data; $i++ )
    {
      if ( $i == 4 ) 
      {
        print   "    $dataFieldDescs[$i]" 
              . " "x( 30 - length($dataFieldDescs[$i]) )   
              . getCompassFromDavisCode($data[$i]) 
              . " "
              . $dataFieldUnits[$i]
              . "\n";
      }else {
        print   "    $dataFieldDescs[$i]" 
              . " "x( 30 - length($dataFieldDescs[$i]) )   
              . $data[$i] 
              . " "
              . $dataFieldUnits[$i] 
              . "\n";
      }
    }
    print "\n";
  }
}


sub getCompassFromDavisCode
{
  my $code = shift;

  my @dirs = ( "N", "NNE", 
               "NE", "ENE",
               "E", "ESE",
               "SE", "SSE",
               "S", "SSW",
               "SW", "WSW", 
               "W", "WNW",
               "NW", "NNW",
               "N" );

  if ( $code < 14 ) 
  {
    return( $dirs[$code] );
  }else {
    return( "No Data" );
  }

}
               

sub getCompassAliasFromDegrees 
{
  my $degrees = shift;

  if ( ( $degrees <= 360 && $degrees >= 348 ) ||
       $degrees < 11 )
  {
     return( "N" );
  }elsif ( $degrees >= 11 && $degrees < 33 )
  {
     return( "NNE" );
  }elsif ( $degrees >= 33 && $degrees < 56 )
  {
     return( "NE" );
  }elsif ( $degrees >= 56 && $degrees < 78 )
  {
     return( "ENE" );
  }elsif ( $degrees >= 78 && $degrees < 101 )
  {
     return( "E" );
  }elsif ( $degrees >= 101 && $degrees < 123 )
  {
     return( "ESE" );
  }elsif ( $degrees >= 123 && $degrees < 146 )
  {
     return( "SE" );
  }elsif ( $degrees >= 146 && $degrees < 168 )
  {
     return( "SSE" );
  }elsif ( $degrees >= 168 && $degrees < 191 )
  {
     return( "S" );
  }elsif ( $degrees >= 191 && $degrees < 213 )
  {
     return( "SSW" );
  }elsif ( $degrees >= 213 && $degrees < 236 )
  {
     return( "SW" );
  }elsif ( $degrees >= 236 && $degrees < 258 )
  {
     return( "WSW" );
  }elsif ( $degrees >= 258 && $degrees < 281 )
  {
     return( "W" );
  }elsif ( $degrees >= 281 && $degrees < 303 )
  {
     return( "WNW" );
  }elsif ( $degrees >= 303 && $degrees < 326 )
  {
     return( "NW" );
  }elsif ( $degrees >= 326 && $degrees < 348 )
  {
     return( "NNW" );
  }
}
