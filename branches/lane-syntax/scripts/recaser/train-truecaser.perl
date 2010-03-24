#!/usr/bin/perl -w

# $Id: train-recaser.perl 1326 2007-03-26 05:44:27Z bojar $
use strict;
use Getopt::Long "GetOptions";

# apply switches
my ($MODEL,$CORPUS);
die("train-truecaser.perl --model truecaser --corpus cased")
    unless &GetOptions('corpus=s' => \$CORPUS,
                       'model=s' => \$MODEL) && defined($CORPUS) && defined($MODEL);
my %CASING;
my %SENTENCE_END = ("."=>1,":"=>1,"?"=>1,"!"=>1);
my %DELAYED_SENTENCE_START = ("("=>1,"["=>1,"\""=>1,"'"=>1);
open(CORPUS,$CORPUS) || die("ERROR: could not open '$CORPUS'");
binmode(CORPUS, ":utf8");
while(<CORPUS>) {
  chop;
  my @WORD = split;
  my $start = 0;
  while($start<=$#WORD && defined($DELAYED_SENTENCE_START{$WORD[$start]})) { $start++; }
  for(my $i=$start+1;$i<=$#WORD;$i++) {
    if (! defined($SENTENCE_END{$WORD[$i-1]})) {
      $CASING{ lc($WORD[$i]) }{ $WORD[$i] }++;
    }
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
