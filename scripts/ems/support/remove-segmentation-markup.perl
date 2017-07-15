#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;

$|++;

while(<STDIN>) {
  chop;
  s/\|[^\|]+\|//g;
  s/\s+/ /g;
  s/^ //;
  s/ $//;
  print $_."\n";
}

#while(<STDIN>) {
#  s/ \|\d+\-\d+\| / /g;
#  s/ \|\d+\-\d+\|$//;
#  print $_;
#}
