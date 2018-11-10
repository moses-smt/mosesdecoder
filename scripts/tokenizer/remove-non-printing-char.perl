#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use utf8;

while (@ARGV) {
    $_ = shift;
    /^-b$/ && ($| = 1, next); # not buffered (flush each line)
}

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

while (my $line = <STDIN>) {
  chomp($line);
  #$line =~ tr/\040-\176/ /c;
  #$line =~ s/[^[:print:]]/ /g;
  #$line =~ s/\s+/ /g;
  $line =~ s/\p{C}/ /g;

  print "$line\n";
}

