#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;

while (my $line = <STDIN>) {
  chomp($line);
  #print "$line\n";

  my $len = length($line);
  my $inXML = 0;
  my $prevSpace = 1;
  my $prevBar = 0;

  for (my $i = 0; $i < $len; ++$i) {
    my $c = substr($line, $i, 1);
    if ($c eq "<" && !$prevBar) {
      ++$inXML;
    }
    elsif ($c eq ">" && $inXML>0) {
      --$inXML;
    }
    elsif ($prevSpace == 1 && $c eq " ")
    { # duplicate space. Do nothing
    }
    elsif ($inXML == 0) {
      if ($c eq " ") {
        $prevSpace = 1;
        $prevBar = 0;
      }
      elsif ($c eq "|") {
        $prevSpace = 0;
        $prevBar = 1;
      }
      else {
        $prevSpace = 0;
        $prevBar = 0;
      }
      print $c;
    }
  }

  print "\n";
}

