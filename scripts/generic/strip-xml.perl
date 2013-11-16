#!/usr/bin/perl -w 

use strict;

while (my $line = <STDIN>) {
  chomp($line);
  #print "$line\n";

  my $len = length($line);
  my $inXML = 0;

  for (my $i = 0; $i < $len; ++$i) {
    my $c = substr($line, $i, 1);
    if ($c eq "<") {
      ++$inXML;
    }
    elsif ($c eq ">") {
      --$inXML;
    }
    elsif ($inXML == 0) {
      print $c;
    }
  }
  print "\n";
}

