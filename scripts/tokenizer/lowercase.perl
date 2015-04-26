#!/usr/bin/env perl 

use warnings;
use strict;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

while(<STDIN>) {
  print lc($_);
}
