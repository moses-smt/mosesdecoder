#!/usr/bin/perl -w

use strict;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

while(<STDIN>) {
  s/\&bar;/\|/g;
  s/\&lt;/\</g;
  s/\&gt;/\>/g;
  s/\&bra;/\[/g;
  s/\&ket;/\]/g;
  s/\&amp;/\&/g;
  print $_;
}
