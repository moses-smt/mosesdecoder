#!/usr/bin/env perl
# A very simple script that converts fsal back to fsa format (openfst lattices)
# Ondrej Bojar, bojar@ufal.mff.cuni.cz
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;

while (<>) {
  chomp;
  tr/ /\n/;
  s/\|\|\|/\t/g;
  print;
  print "\n";
  print "\n";
}

