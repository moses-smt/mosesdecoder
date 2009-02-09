#!/usr/bin/perl -w

# $Id: train-recaser.perl 1326 2007-03-26 05:44:27Z bojar $
use strict;
use Getopt::Long "GetOptions";

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

# apply switches
my $MODEL;
die("truecase.perl --model truecaser < in > out")
    unless &GetOptions('model=s' => \$MODEL);

my (%BEST,%KNOWN);
open(MODEL,$MODEL) || die("ERROR: could not open '$MODEL'");
binmode(MODEL, ":utf8");
while(<MODEL>) {
  my ($word,@OPTIONS) = split;
  $BEST{ lc($word) } = $word;
  $KNOWN{ $word } = 1;
  for(my $i=1;$i<$#OPTIONS;$i+=2) {
    $KNOWN{ $OPTIONS[$i] } = 1;
  }
}
close(MODEL);

my %SENTENCE_END = ("."=>1,":"=>1,"?"=>1,"!"=>1);
my %DELAYED_SENTENCE_START = ("("=>1,"["=>1,"\""=>1,"'"=>1);

while(<STDIN>) {
  chop;
  my @WORD = split;
  my $sentence_start = 1;
  for(my $i=0;$i<=$#WORD;$i++) {
    print " " if $i;

    $WORD[$i] =~ /^([^\|]+)(.*)/;
    my $word = $1;
    my $otherfactors = $2;

    if ($sentence_start && defined($BEST{lc($word)})) {
      print $BEST{lc($word)}; # truecase sentence start
    }
    elsif (defined($KNOWN{$word})) {
      print $word; # don't change known words
    }
    elsif (defined($BEST{lc($word)})) {
      print $BEST{lc($word)}; # truecase otherwise unknown words
    }
    else {
      print $word; # unknown, nothing to do
    }
    print $otherfactors;

    if    ( defined($SENTENCE_END{ $word }))           { $sentence_start = 1; }
    elsif (!defined($DELAYED_SENTENCE_START{ $word })) { $sentence_start = 0; }
  }
  print "\n";
}
