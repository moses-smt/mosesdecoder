#!/usr/bin/env perl
# A very simple script that converts fsal back to fsa format (openfst lattices)
# Ondrej Bojar, bojar@ufal.mff.cuni.cz

use strict;

while (<>) {
  chomp;
  tr/ /\n/;
  s/\|\|\|/\t/g;
  print;
  print "\n";
  print "\n";
}

