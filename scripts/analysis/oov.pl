#!/usr/bin/perl
# Display OOV rate of a test set against a training corpus or a phrase table.
# Ondrej Bojar

use strict;
use warnings;

use Getopt::Long;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

my $verbose = 0;
my $n = 1;
my $srcfile = undef;
GetOptions(
  "n=i" => \$n,  # the n-grams to search for (default: unigrams)
  "verbose" => \$verbose, # emit the list of oov words
  "src=s" => \$srcfile, # use this source file
) or exit 1;

my $testf = shift;
if (!defined $testf) {
  print STDERR "usage: $0 test-corpus < training-corpus
Options:
  --n=1  ... use phrases of n words as the unit
  --verbose  ... emit OOV phrases at the end
  --src=test-src ... a word in the test-corpus not deemed OOV if present in the
                     corresponding source sentence in test-src.
                     The files test-corpus and test-src must be of equal length.
Synopsis:
  Check OOV of a training corpus against a test set:
    cat corpus.src.txt | $0 testset.txt
  Check target-side OOV of a phrase table against a reference:
    cat ttable | sed 's/ ||| /<ctrl-v><tab>/g' | cut -f 2 \
      | $0 reference.txt
";
  exit 1;
}

# load source file to accept ngrams from source
my $source_confirms = undef;
my $srcfilelen = undef;
if (defined $srcfile) {
  my $fh = my_open($srcfile);
  my $nr = 0;
  my $srctokens = 0;
  while (<$fh>) {
    $nr++;
    chomp;
    s/^\s+//;
    s/\s+$//;
    my $ngrams = ngrams($n, [ split /\s+/, $_ ]);
    foreach my $ngr (keys %$ngrams) {
      $source_confirms->[$nr]->{$ngr} += $ngrams->{$ngr};
      $srctokens += $ngrams->{$ngr};
    }
  }
  close $fh;
  print "Source set sents\t$nr\n";
  print "Source set running $n-grams\t$srctokens\n";
  $srcfilelen = $nr;
}

my %needed = ();
my $fh = my_open($testf);
my $nr = 0;
my $testtokens = 0;
my %testtypes = ();
while (<$fh>) {
  $nr++;
  chomp;
  s/^\s+//;
  s/\s+$//;
  my $ngrams = ngrams($n, [ split /\s+/, $_ ]);
  foreach my $ngr (keys %$ngrams) {
    $needed{$ngr} += $ngrams->{$ngr}
      unless $source_confirms->[$nr]->{$ngr};
    $testtokens += $ngrams->{$ngr};
    $testtypes{$ngr} = 1;
  }
}
close $fh;
my $testtypesneeded = scalar(keys(%needed));
my $testtypes = scalar(keys(%testtypes));
print "Test set sents\t$nr\n";
print "Test set running $n-grams\t$testtokens\n";
print "Test set unique $n-grams needed\t$testtypesneeded\n";
print "Test set unique $n-grams\t$testtypes\n";

die "Mismatching sent count: $srcfile and $testf ($srcfilelen vs. $nr)"
  if defined $srcfile && $srcfilelen != $nr;

my %seen;
$nr = 0;
my $traintokens = 0;
while (<>) {
  $nr++;
  print STDERR "." if $nr % 10000 == 0;
  print STDERR "($nr)" if $nr % 500000 == 0;
  chomp;
  s/^\s+//;
  s/\s+$//;
  my $ngrams = ngrams($n, [ split /\s+/, $_ ]);
  foreach my $ngr (keys %$ngrams) {
    $seen{$ngr} = 1 if $ngrams->{$ngr};
    $traintokens += $ngrams->{$ngr};
  }
}
foreach my $ngr (keys %needed) {
  delete $needed{$ngr} if defined $seen{$ngr};
}
print STDERR "Done.\n";
my $traintypes = scalar(keys(%seen));
print "Training set sents\t$nr\n";
print "Training set running $n-grams\t$traintokens\n";
print "Training set unique $n-grams\t$traintypes\n";


my $oovtypes = scalar(keys(%needed));
my $oovtokens = 0;
foreach my $v (values %needed) {
  $oovtokens += $v;
}
printf "OOV $n-gram types\t%i\t%.1f %%\n", $oovtypes, $oovtypes/$testtypes*100;
printf "OOV $n-gram tokens\t%i\t%.1f %%\n", $oovtokens, $oovtokens/$testtokens*100;

if ($verbose) {
  foreach my $ngr (sort {$needed{$b} <=> $needed{$a}} keys %needed) {
    print "$needed{$ngr}\t$ngr\n";
  }
}

sub my_open {
  my $f = shift;
  if ($f eq "-") {
    binmode(STDIN, ":utf8");
    return *STDIN;
  }

  die "Not found: $f" if ! -e $f;

  my $opn;
  my $hdl;
  my $ft = `file '$f'`;
  # file might not recognize some files!
  if ($f =~ /\.gz$/ || $ft =~ /gzip compressed data/) {
    $opn = "zcat '$f' |";
  } elsif ($f =~ /\.bz2$/ || $ft =~ /bzip2 compressed data/) {
    $opn = "bzcat '$f' |";
  } else {
    $opn = "$f";
  }
  open $hdl, $opn or die "Can't open '$opn': $!";
  binmode $hdl, ":utf8";
  return $hdl;
}

sub ngrams {
  my $n = shift;
  my @words = @{shift()};
  my $out;
  if ($n == 1) {
    foreach my $w (@words) {
      $out->{$w}++;
    }
  } else {
    while ($#words >= $n-1) {
      $out->{join(" ", @words[0..$n-1])}++;
      shift @words;
    }
  }
  return $out;
}
