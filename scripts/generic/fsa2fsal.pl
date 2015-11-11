#!/usr/bin/env perl
# A very simple script that converts fsa format (openfst lattices) to the same
# thing represented one sentence per line. It uses '|||' to delimit columns and
# ' ' to delimit nodes (i.e. original lines).
# Some rudimentary sanity checks are done on the fly.
# Ondrej Bojar, bojar@ufal.mff.cuni.cz
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;

my $errs = 0;
sub err {
  my $nr = shift;
  my $msg = shift;
  print STDERR "$nr:$msg\n";
  $errs++;
}

my $onr = 0;
my @lines = ();
sub flush {
  return if 0 == scalar @lines;
  print join(" ", @lines);
  print "\n";
  $onr++;
  @lines = ();
}

my $nr = 0;
my $numscores = undef;
while (<>) {
  chomp;
  if ($_ eq "") {
    flush();
    next;
  }
  my ($a, $b, $label, $scores, $rest) = split /\s+/, $_, 5;
  err($nr, "The delimiter '|||' can't appear in the input!") if /\|\|\|/;
  err($nr, "Node id not numeric: $a") if $a !~ /^\d+$/;
  err($nr, "Node id not numeric: $b") if $b !~ /^\d+$/;
  err($nr, "Unexpected tail: '$rest'") if defined $rest && $rest !~ /^\s*$/;
  my $thisnumscores = ($scores =~ tr/,/,/);
  $numscores = $thisnumscores if !defined $numscores;
  err($nr, "Incompatible number of arc scores, previous lines had ".($numscores+1).", now ".($thisnumscores+1))
    if $numscores != $thisnumscores;
  push @lines, join("|||", ($a,$b,$label,$scores));
}
flush();

exit 1 if $errs;
