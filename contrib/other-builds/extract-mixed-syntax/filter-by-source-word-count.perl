#!/usr/bin/perl

use strict;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

my $maxNumWords = $ARGV[0];

while (my $line = <STDIN>) {
  chomp($line);
  my @toks = split(/ /,$line);
  
  my $numSourceWords = 0;
  my $tok = $toks[$numSourceWords];
  while ($tok ne "|||") {
    ++$numSourceWords;
    $tok = $toks[$numSourceWords];
  }

  if ($numSourceWords <= $maxNumWords) {
    print "$line\n";
  }
}


