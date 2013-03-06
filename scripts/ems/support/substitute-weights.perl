#!/usr/bin/perl -w 

# experiment.perl support script
# get filtered rule and reordering tables and place them into a configuration file

if (scalar @ARGV < 1 || ! -e $ARGV[0]) {
  die("ERROR: could not find pseudo-config with filtered tables");
}

# read initial ini file
my @arr;
my $inWeightSection = 0;
open(FILTERED, $ARGV[0]) or die "Cannot open: $!";
while(my $line = <FILTERED>) {
  chomp($line);
  if ($line =~ /\[weight\]/) {
   $inWeightSection = 1;
  }
  elsif ($line =~ /\[[a-zA-Z0-0]*\]/) {
   $inWeightSection = 0;
  }  

  if (!$inWeightSection) {
    print "$line\n";
  }
}
close(FILTERED);

# read tuned ini file
$inWeightSection = 0;
my $ind = 0;
while(my $line = <STDIN>) {
  chomp($line);
  if ($line =~ /\[weight\]/) {
   $inWeightSection = 1;
  }
  elsif ($line =~ /\[[a-zA-Z0-0]*\]/) {
   $inWeightSection = 0;
  }  

  if ($inWeightSection) {
    print "$line\n";
  }
}
