#!/usr/bin/perl -w
use strict;

# 0 ||| a b c  ||| d: 0 lm: -2.30259 tm: 2 2 w: -3 ||| 0.469741
# 0 ||| a b c  ||| d: 0 lm: -2.30259 tm: 3 3 w: -3 ||| 0.669742
# 0 ||| a b c  ||| d: 0 lm: -2.30259 tm: 3 5 w: -3 ||| 0.869741
# 0 ||| a c b  ||| d: -3 lm: -2.30259 tm: 3 5 w: -3 ||| 0.569741

my $Z = 0;
my $lid = 0;
my %total = ();
while(<>) {
  chomp;
  my ($id, $sent, $fvals, $score) = split / \|\|\| /;
  die "Only expect output from a single sentence!" unless $lid == $id;
  my $escore = exp($score);
  $total{$sent} += $escore;
  $Z += $escore;
}

for my $sent (sort { $total{$b} <=> $total{$a} } keys %total) {
  my $prob = $total{$sent} / $Z;
  print "$sent ||| $prob\n";
}

