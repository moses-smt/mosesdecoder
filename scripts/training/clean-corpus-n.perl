#!/usr/bin/perl -w

# $Id$
use strict;
use Getopt::Long;
my $help;
my $lc = 0; # lowercase the corpus?
my $ignore_ratio = 0;
my $ignore_xml = 0;
my $enc = "utf8"; # encoding of the input and output files
    # set to anything else you wish, but I have not tested it yet
my $max_word_length = 1000; # any segment with a word (or factor) exceeding this length in chars
    # is discarded; motivated by symal.cpp, which has its own such parameter (hardcoded to 1000)
    # and crashes if it encounters a word that exceeds it

GetOptions(
  "help" => \$help,
  "lowercase|lc" => \$lc,
  "encoding=s" => \$enc,
  "ignore-ratio" => \$ignore_ratio,
  "ignore-xml" => \$ignore_xml,
  "max-word-length|mwl=s" => \$max_word_length
) or exit(1);

if (scalar(@ARGV) < 6 || $help) {
    print "syntax: clean-corpus-n.perl corpus l1 l2 clean-corpus min max [lines retained file]\n";
    exit;
}

my $corpus = $ARGV[0];
my $l1 = $ARGV[1];
my $l2 = $ARGV[2];
my $out = $ARGV[3];
my $min = $ARGV[4];
my $max = $ARGV[5];

my $linesRetainedFile = "";
if (scalar(@ARGV) > 6) {
	$linesRetainedFile = $ARGV[6];
	open(LINES_RETAINED,">$linesRetainedFile") or die "Can't write $linesRetainedFile";
}

print STDERR "clean-corpus.perl: processing $corpus.$l1 & .$l2 to $out, cutoff $min-$max\n";

my $opn = undef;
my $l1input = "$corpus.$l1";
if (-e $l1input) {
  $opn = $l1input;
} elsif (-e $l1input.".gz") {
  $opn = "gunzip -c $l1input.gz |";
} else {
    die "Error: $l1input does not exist";
}
open(F,$opn) or die "Can't open '$opn'";
$opn = undef;
my $l2input = "$corpus.$l2";
if (-e $l2input) {
  $opn = $l2input;
} elsif (-e $l2input.".gz") {
  $opn = "gunzip -c $l2input.gz |";
} else  {
 die "Error: $l2input does not exist";
}
 
open(E,$opn) or die "Can't open '$opn'";

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
my $factored_flag;
while(my $f = <F>) {
  $innr++;
  print STDERR "." if $innr % 10000 == 0;
  print STDERR "($innr)" if $innr % 100000 == 0;
  my $e = <E>;
  die "$corpus.$l2 is too short!" if !defined $e;
  chomp($e);
  chomp($f);
  if ($innr == 1) {
    $factored_flag = ($e =~ /\|/ || $f =~ /\|/);
  }

  #if lowercasing, lowercase
  if ($lc) {
    $e = lc($e);
    $f = lc($f);
  }
  
  $e =~ s/\|//g unless $factored_flag;
  $e =~ s/\s+/ /g;
  $e =~ s/^ //;
  $e =~ s/ $//;
  $f =~ s/\|//g unless $factored_flag;
  $f =~ s/\s+/ /g;
  $f =~ s/^ //;
  $f =~ s/ $//;
  next if $f eq '';
  next if $e eq '';

  my $ec = &word_count($e);
  my $fc = &word_count($f);
  next if $ec > $max;
  next if $fc > $max;
  next if $ec < $min;
  next if $fc < $min;
  next if !$ignore_ratio && $ec/$fc > 9;
  next if !$ignore_ratio && $fc/$ec > 9;
  # Skip this segment if any factor is longer than $max_word_length
  my $max_word_length_plus_one = $max_word_length + 1;
  next if $e =~ /[^\s\|]{$max_word_length_plus_one}/;
  next if $f =~ /[^\s\|]{$max_word_length_plus_one}/;
  
  # An extra check: none of the factors can be blank!
  die "There is a blank factor in $corpus.$l1 on line $innr: $f"
    if $f =~ /[ \|]\|/;
  die "There is a blank factor in $corpus.$l2 on line $innr: $e"
    if $e =~ /[ \|]\|/;
  
  $outnr++;
  print FO $f."\n";
  print EO $e."\n";

  if ($linesRetainedFile ne "") {
	print LINES_RETAINED $innr."\n";
  }
}

if ($linesRetainedFile ne "") {
  close LINES_RETAINED;
}

print STDERR "\n";
my $e = <E>;
die "$corpus.$l2 is too long!" if defined $e;

print STDERR "Input sentences: $innr  Output sentences:  $outnr\n";

sub word_count {
  my ($line) = @_;
  if ($ignore_xml) {
    $line =~ s/<\S[^>]*\S>//g;
    $line =~ s/\s+/ /g;
    $line =~ s/^ //g;
    $line =~ s/ $//g;    
  }
  my @w = split(/ /,$line);
  return scalar @w;
}

