#!/usr/bin/perl
$x=0;
while (<>) {
  chomp;
  $x++;
  print "WEIGHT_$x=$_\n";
}
