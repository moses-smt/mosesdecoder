#!/usr/bin/perl -w

use strict;

my $THRESHOLD = $ARGV[0];
die("please specify threshold (e.g., 0.00001)") unless defined($THRESHOLD) || $THRESHOLD > 0;

my ($filtered,$total) = (0,0);
while(my $line = <STDIN>) {
  my @ITEM = split(/ \|\|\| /,$line);
  my @SCORE = split(/ /,$ITEM[2]);
  $total++;
  if ($SCORE[0] < $THRESHOLD || $SCORE[2] < $THRESHOLD) {
	  $filtered++;
	  next;
  }
  print $line;
}

print STDERR "filtered out $filtered of $total phrase pairs.\n";
