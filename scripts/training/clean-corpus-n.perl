#!/usr/bin/perl -w

use strict;
use Getopt::Long;
my $help;
my $lc = 0; # lowercase the corpus?
my $enc = "utf8"; # encoding of the input and output files
    # set to anything else you wish, but I have not tested it yet

GetOptions(
  "help" => \$help,
  "lowercase|lc" => \$lc,
  "encoding" => \$enc,
) or exit(1);

if (scalar(@ARGV) != 6 || $help) {
    print "syntax: clean-corpus-n.perl corpus l1 l2 clean-corpus min max\n";
    exit;
}

my $corpus = $ARGV[0];
my $l1 = $ARGV[1];
my $l2 = $ARGV[2];
my $out = $ARGV[3];
my $min = $ARGV[4];
my $max = $ARGV[5];

print STDERR "clean-corpus.perl: processing $corpus.$l1 & .$l2 to $out, cutoff $min-$max\n";

open(F,"$corpus.$l1") or die "Can't read $corpus.$l1";
open(E,"$corpus.$l2") or die "Can't read $corpus.$l2";
open(FO,">$out.$l1") or die "Can't write $out.$l1";
open(EO,">$out.$l2") or die "Can't write $out.$l2";

# necessary for proper lowercasing
my $binmode;
if ($enc eq "utf8") {
  $binmode = ":utf8";
} else {
  $binmode = ":encoding($enc)";
}
binmode(F, $binmode);
binmode(E, $binmode);
binmode(FO, $binmode);
binmode(EO, $binmode);

my $innr = 0;
my $outnr = 0;
while(my $f = <F>) {
  $innr++;
  print STDERR "." if $innr % 10000 == 0;
  print STDERR "($innr)" if $innr % 100000 == 0;
  my $e = <E>;
  die "$corpus.$l2 is too short!" if !defined $e;
  chomp($e);
  chomp($f);

  #if lowercasing, lowercase
  if ($lc) {
    $e = lc($e);
    $f = lc($f);
  }
  
  # $e =~ s/\|//g;  # kinda hurts in factored input
  $e =~ s/\s+/ /g;
  $e =~ s/^ //;
  $e =~ s/ $//;
  # $f =~ s/\|//g;  # kinda hurts in factored input
  $f =~ s/\s+/ /g;
  $f =~ s/^ //;
  $f =~ s/ $//;
  next if $f eq '';
  next if $e eq '';
  my @E = split(/ /,$e);
  my @F = split(/ /,$f);
  next if scalar(@E) > $max;
  next if scalar(@F) > $max;
  next if scalar(@E) < $min;
  next if scalar(@F) < $min;
  next if scalar(@E)/scalar(@F) > 9;
  next if scalar(@F)/scalar(@E) > 9;
  
  # An extra check: none of the factors can be blank!
  die "There is a blank factor in $corpus.$l1 on line $innr: $f"
    if $f =~ /[ \|]\|/;
  die "There is a blank factor in $corpus.$l2 on line $innr: $f"
    if $e =~ /[ \|]\|/;
    
  
  $outnr++;
  print FO $f."\n";
  print EO $e."\n";
}
print STDERR "\n";
my $e = <E>;
die "$corpus.$l2 is too long!" if defined $e;

print STDERR "Input sentences: $innr  Output sentences:  $outnr\n";
