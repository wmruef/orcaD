#!/usr/bin/perl

my $state = "NONE";
my $fileName = $ARGV;
my @doc = ();
my %docs = ();
my $name;
print "$fileName" . "\n";
print "=" x length($fileName) . "\n";
while ( <> ) {

  if ( $fileName ne $ARGV ) {
    $fileName = $ARGV;
    print "$fileName" . "\n";
    print "=" x length($fileName) . "\n";
  }
  if ( /^\/\/\s*NAME/ )
  {
    $state = "IN_NAME";
  }elsif ( /^\/\/\s*SYNOPSIS/ ) 
  {
    $state = "IN_SYNOPSIS";
  }elsif ( $state ne "" && /^[^\/]/ ) {
    $state = ""; 
    if ( @doc ) {
      $docs{$name} = [@doc];
      @doc = ();
    }
    $name = "";
  }else {
    if ( $state eq "IN_NAME" ) 
    {
      if ( /^\/\/\s+\S.*$/ )
      {
        if ( /^\/\/\s+(\S+)\s*-/ ) {
          $name = $1;
        }
        my $entry = $_;
        $entry =~ s/\/\///g;
        print $entry;
      }
    }
  }

  if ( $state ne "" ) {
    s/\/\//  /g;
    push @doc, $_;
  }

}


foreach my $docName ( sort keys( %docs ) ) {
  print "=============================================================\n";
  foreach my $line ( @{$docs{$docName}} ) {
    print $line;
  }
}
