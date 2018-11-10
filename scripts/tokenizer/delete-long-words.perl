#!/usr/bin/perl -w

use strict;

while (@ARGV) {
    $_ = shift;
    /^-b$/ && ($| = 1, next); # not buffered (flush each line)
}

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
