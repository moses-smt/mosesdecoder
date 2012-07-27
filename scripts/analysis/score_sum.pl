#!/usr/bin/perl

use strict;
use warnings;

# usage: score-sum.pl nbest-list score-name [score-index]
# score index is 0-based; assumed to be 0 if not given

my $ZCAT = "gzip -cd";
my $BZCAT = "bzcat";

my ($nbestfile, $scorename, $scoreidx) = @ARGV;
die "error: Wrong arguments" if ! defined $scorename;

$scoreidx = 0 if ! defined $scoreidx;

my $hdl = my_open($nbestfile);
my $current_sentid = -1;
my $sum = 0;

while (<$hdl>) {
  my @cols = split / \|\|\| /, $_;
  my $sentid = $cols[0];
  next if $sentid eq $current_sentid; # only take the 1-best output
  $current_sentid = $sentid;
  my @score_tokens = split " ", $cols[2];
  my $token = "";
  while ($token ne "$scorename:") {
    $token = shift @score_tokens;
  }
  if (scalar(@score_tokens) < $scoreidx) {
    die "Score not found: $scorename [ $scoreidx ]";
  }
  $sum += $score_tokens[$scoreidx];
}
print "$scorename [ $scoreidx ] sum = " . $sum . "\n";


sub my_open {
  my $f = shift;
  die "Not found: $f" if ! -e $f;

  my $opn;
  my $hdl;
  my $ft = `file $f`;
  # file might not recognize some files!
  if ($f =~ /\.gz$/ || $ft =~ /gzip compressed data/) {
    $opn = "$ZCAT $f |";
  } elsif ($f =~ /\.bz2$/ || $ft =~ /bzip2 compressed data/) {
    $opn = "$BZCAT $f |";
  } else {
    $opn = "$f";
  }
  open $hdl, $opn or die "Can't open '$opn': $!";
  binmode $hdl, ":utf8";
  return $hdl;
}
