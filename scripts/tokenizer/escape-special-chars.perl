#!/usr/bin/perl -w

use strict;

while(<STDIN>) {
  chop;

  # avoid general madness
  s/\s+/ /g;
	s/^ //g;
	s/ $//g;
  s/[\000-\037]//g;

  # special characters in moses
  s/\&/\&amp;/g;
  s/\|/\&bar;/g;
  s/\</\&lt;/g;
  s/\>/\&gt;/g;
  s/\[/\&bra;/g;
  s/\]/\&ket;/g;
  
  print $_."\n";
}
