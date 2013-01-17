#!/usr/bin/perl -w

# $Id: train-recaser.perl 1326 2007-03-26 05:44:27Z bojar $

#
# Options:
#
# --possiblyUseFirstToken : boolean option; the default behaviour (when this option is not provided) is that the first token of a sentence is ignored, on the basis that the first word of a sentence is always capitalized; if this option is provided then: a) if a sentence-initial token is *not* capitalized, then it is counted, and b) if a capitalized sentence-initial token is the only token of the segment, then it is counted, but with only 10% of the weight of a normal token.
#

use strict;
use Getopt::Long "GetOptions";

# apply switches
my ($MODEL,$CORPUS);
die("train-truecaser.perl --model truecaser --corpus cased [--possiblyUseFirstToken]")
    unless &GetOptions('corpus=s' => \$CORPUS,
                       'model=s' => \$MODEL,
                       'possiblyUseFirstToken' => \(my $possiblyUseFirstToken = 0))
    && defined($CORPUS) && defined($MODEL);
my %CASING;
my %SENTENCE_END = ("."=>1,":"=>1,"?"=>1,"!"=>1);
my %DELAYED_SENTENCE_START = ("("=>1,"["=>1,"\""=>1,"'"=>1,"&apos;"=>1,"&quot;"=>1,"&#91;"=>1,"&#93;"=>1);
open(CORPUS,$CORPUS) || die("ERROR: could not open '$CORPUS'");
binmode(CORPUS, ":utf8");
while(<CORPUS>) {
  chop;
  my @WORD = split;
  my $start = 0;
  while($start<=$#WORD && defined($DELAYED_SENTENCE_START{$WORD[$start]})) { $start++; }
  my $firstWordOfSentence = 1;
  for(my $i=$start;$i<=$#WORD;$i++) {
    my $currentWord = $WORD[$i];
    if (! $firstWordOfSentence && defined($SENTENCE_END{$WORD[$i-1]})) {
      $firstWordOfSentence = 1;
    }

    my $currentWordWeight = 0;
    if (! $firstWordOfSentence) {
      $currentWordWeight = 1;
    } elsif ($possiblyUseFirstToken) {
      # gated special handling of first word of sentence
      my $firstChar = substr($currentWord, 0, 1);
      if (lc($firstChar) eq $firstChar) {
        # if the first character is not upper case, count the token as full evidence (because if it's not capitalized, then there's no reason to be wary that the given casing is only due to being sentence-initial)
	$currentWordWeight = 1;
      } elsif (scalar(@WORD) == 1) {
	# if the first character is upper case, but the current token is the only token of the segment, then count the token as partial evidence (because the segment is presumably not a sentence and the token is therefore not the first word of a sentence and is possibly in its natural case)
	$currentWordWeight = 0.1;
      }
    }
    if ($currentWordWeight > 0) {
      $CASING{ lc($currentWord) }{ $currentWord } += $currentWordWeight;
    }

    $firstWordOfSentence = 0;
  }
}
close(CORPUS);

open(MODEL,">$MODEL") || die("ERROR: could not create '$MODEL'");
binmode(MODEL, ":utf8");
foreach my $type (keys %CASING) {
  my ($score,$total,$best) = (-1,0,"");
  foreach my $word (keys %{$CASING{$type}}) {
    my $count = $CASING{$type}{$word};
    $total += $count;
    if ($count > $score) {
      $best = $word;
      $score = $count;
    }
  }
  print MODEL "$best ($score/$total)";
  foreach my $word (keys %{$CASING{$type}}) {
    print MODEL " $word ($CASING{$type}{$word})" unless $word eq $best;
  }
  print MODEL "\n";
}
close(MODEL);
