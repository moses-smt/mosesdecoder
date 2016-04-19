#!/usr/bin/perl -w

use strict;
while(<STDIN>) {
  chop;
  my $first = 1;
  foreach (split) {
    if (length($_)<200) {
      print " " unless $first;
      print $_;
      $first = 0;
    }
  }
  print "\n";
}
