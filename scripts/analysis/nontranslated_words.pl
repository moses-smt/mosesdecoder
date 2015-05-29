#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# $Id$
# Reads a source and hypothesis file and counts equal tokens. Some of these
# are punctuation, some are numbers, but most of the remaining are simply
# unknown words that the decoder just copied. This script tells you how often
# this happens.
#
# Ondrej Bojar


use strict;
use warnings;
use Getopt::Long;

my $ignore_numbers = 0;
my $ignore_punct = 0;
my $usage = 0;
my $top = 10;

GetOptions(
  "help" => \$usage,
  "top=i" => \$top,
  "ignore-numbers" => \$ignore_numbers,
  "ignore-punct" => \$ignore_punct,
) or exit 1;
my $src = shift;
my $tgt = shift;

if ($usage || !defined $src || !defined $tgt) {
  print STDERR "nontranslated_words.pl srcfile hypothesisfile
...counts the number of words that are equal in src and hyp. These are
typically unknown words.
Options:
  --top=N  ... list N top copied tokens
  --ignore-numbers  ... numbers usually do not get translated, but do
     not count them (it is not an error)
  --ignore-punct ... same for punct, do not include it in the count
";
  exit 1;
}

binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

open SRC, $src or die "Can't read $src";
open TGT, $tgt or die "Can't read $tgt";
binmode(SRC, ":utf8");
binmode(TGT, ":utf8");

my $nr=0;
my $outtoks = 0;
my $intoks = 0;
my $copiedtoks = 0;
my %copiedtok;
while (<SRC>) {
  $nr++;
  chomp;
  s/^\s+|\s+$//g;
  my @src = split /\s+/;
  my %src = map {($_,1)} @src;
  $intoks += scalar @src;
  my $t = <TGT>;
  die "$tgt too short!" if !defined $t;
  $t =~ s/^\s+|\s+$//g;
  foreach my $outtok (split /\s+/, $t) {
    $outtoks++;
    next if !defined $src{$outtok}; # this word did not appear in input, we generated it
    next if $ignore_numbers && $outtok =~ /^-?[0-9]*([.,][0-9]+)?$/;
    next if $ignore_punct && $outtok =~ /^[[:punct:]]+$/;
    $copiedtoks++;
    $copiedtok{$outtok}++;
  }
}
my $t = <TGT>;
die "$tgt too long!" if defined $t;
close SRC;
close TGT;

print "Sentences:\t$nr
Source tokens:\t$intoks
Output tokens:\t$outtoks
Output tokens appearing also in input sent:\t$copiedtoks\t"
  .sprintf("%.2f %%", $copiedtoks/$outtoks*100)
  ."\t".($ignore_punct?"ignoring":"including")." punctuation"
  ."\t".($ignore_numbers?"ignoring":"including")." numbers"
  ."\n";

if ($top) {
  my $cnt = 0;
  print "Top $top copied tokens:\n";
  foreach my $t (sort {$copiedtok{$b}<=>$copiedtok{$a} || $a cmp $b} keys %copiedtok) {
    print "$copiedtok{$t}\t$t\n";
    last if $cnt > $top;
    $cnt++;
  }
}
