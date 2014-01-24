#!/usr/bin/perl -w 

use strict;

while (my $line = <STDIN>) {
  chomp($line);
  #print "$line\n";

  my $len = length($line);
  my $inXML = 0;
  my $prevSpace = 1;

  for (my $i = 0; $i < $len; ++$i) {
    my $c = substr($line, $i, 1);
    if ($c eq "<") {
      ++$inXML;
    }
    elsif ($c eq ">") {
      --$inXML;
    }
    elsif ($prevSpace == 1 && $c eq " ")
    { # duplicate space. Do nothing
    }
    elsif ($inXML == 0) {
      if ($c eq " ") {
        $prevSpace = 1;
      }
      else {
        $prevSpace = 0;
      }
      print $c;
    }
  }

  print "\n";
}

