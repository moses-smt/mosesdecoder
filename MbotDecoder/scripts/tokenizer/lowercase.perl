#!/usr/bin/perl -w

use strict;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

while(<STDIN>) {
  print lc($_);
}
