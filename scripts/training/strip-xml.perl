#!/usr/bin/env perl

# strip text file of any XML markup

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

use strict;

while(<STDIN>) {
  s/<\S[^>]*>/ /g;
  chomp;
  s/ +/ /g;
  s/^ //;
  print $_;
  print "\n";
}
