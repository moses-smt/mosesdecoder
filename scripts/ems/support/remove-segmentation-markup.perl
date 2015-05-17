#!/usr/bin/env perl

use warnings;
use strict;

$|++;

while(<STDIN>) {
  s/ \|\d+\-\d+\| / /g;
  s/ \|\d+\-\d+\|$//;
  print $_;
}
