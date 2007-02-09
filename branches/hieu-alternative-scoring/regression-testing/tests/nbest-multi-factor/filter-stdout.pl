#!/usr/bin/perl
$x=0;
while (<>) {
  chomp;
  print "TRANSLATION_$x=$_\n";
  $x++;
}
