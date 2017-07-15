#!/usr/bin/env perl
# Collects and prints all n-grams that appear in the given corpus both
# tokenized as well as untokenized.
# Ondrej Bojar
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use strict;
use warnings;

use Getopt::Long;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

my $usage = 0;
my $lowercase = 0;
my $n = 2;
GetOptions(
  "n=i" => \$n,  # the n-grams to search for (default: bigrams)
  "lc|lowercase" => \$lowercase, # ignore case
  "h|help|usage" => \$usage, # show info
) or exit 1;

my $nl = 0;
my $ngrams;
my $words;
while (<>) {
  $nl++;
  print STDERR "." if $nl % 100000 == 0;
  print STDERR "($nl)" if $nl % 500000 == 0;
  chomp;
  $_ = lc($_) if $lowercase;
  my @words = split /\s+/;
  foreach my $w (@words) {
    $words->{$w}++;
  }
  $ngrams = ngrams($n, \@words, $ngrams); # add ngram counts from this
}
print STDERR "Done.\n";

# Find suspicious
my $report;
foreach my $ngr (keys %$ngrams) {
  my $w = $ngr;
  $w =~ s/ //g;
  my $untokcnt = $words->{$w};
  next if ! $untokcnt; # never seen untokenized
  my $tokcnt = $ngrams->{$ngr};
  $report->{$ngr}->{"tok"} = $tokcnt;
  $report->{$ngr}->{"untok"} = $untokcnt;
  $report->{$ngr}->{"diff"} = abs($untokcnt-$tokcnt);
  $report->{$ngr}->{"sum"} = $untokcnt+$tokcnt;
}

# Report
foreach my $ngr (sort {
                         $report->{$a}->{"diff"} <=> $report->{$b}->{"diff"}
                      || $report->{$b}->{"sum"} <=> $report->{$a}->{"sum"}
                      }
                  keys %$report) {
  print "$ngr\t$report->{$ngr}->{untok}\t$report->{$ngr}->{tok}\t$report->{$ngr}->{diff}\n";
}

sub ngrams {
  my $n = shift;
  my @words = @{shift()};
  my $out = shift;
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
