#!/usr/bin/perl -w

use strict;

$|++;

while(<STDIN>) {
  s/ \|\d+\-\d+\| / /g; 
  s/ \|\d+\-\d+\|$//; 
  print $_;
}
