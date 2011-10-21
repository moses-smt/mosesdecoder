#!/usr/bin/perl

use strict;
use warnings;

while (<>) {
  last if $_ =~ m/^diff --git/;
  print $_;
}
