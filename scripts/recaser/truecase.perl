#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# $Id: train-recaser.perl 1326 2007-03-26 05:44:27Z bojar $

use warnings;
use strict;
use Getopt::Long "GetOptions";

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

# apply switches
# ASR input has no case, make sure it is lowercase, and make sure known are cased eg. 'i' to be uppercased even if i is known
my ($MODEL, $UNBUFFERED, $ASR);
die("truecase.perl --model MODEL [-b] [-a] < in > out")
    unless &GetOptions('model=s' => \$MODEL,'b|unbuffered' => \$UNBUFFERED, 'a|asr' => \$ASR)
    && defined($MODEL);
if (defined($UNBUFFERED) && $UNBUFFERED) { $|=1; }
my $asr = 0;
if (defined($ASR) && $ASR) { $asr = 1; }

my (%BEST,%KNOWN);
open(MODEL,$MODEL) || die("ERROR: could not open '$MODEL'");
binmode(MODEL, ":utf8");
while(<MODEL>) {
  my ($word,@OPTIONS) = split;
  $BEST{ lc($word) } = $word;
  if ($asr == 0) {
    $KNOWN{ $word } = 1;
    for(my $i=1;$i<$#OPTIONS;$i+=2) {
      $KNOWN{ $OPTIONS[$i] } = 1;
    }
  }
}
close(MODEL);

my %SENTENCE_END = ("."=>1,":"=>1,"?"=>1,"!"=>1);
my %DELAYED_SENTENCE_START = ("("=>1,"["=>1,"\""=>1,"'"=>1,"&apos;"=>1,"&quot;"=>1,"&#91;"=>1,"&#93;"=>1);

while(<STDIN>) {
  chop;
  my ($WORD,$MARKUP) = split_xml($_);
  my $sentence_start = 1;
  for(my $i=0;$i<=$#$WORD;$i++) {
    print " " if $i && $$MARKUP[$i] eq '';
    print $$MARKUP[$i];

    my ($word,$otherfactors);
    if ($$WORD[$i] =~ /^([^\|]+)(.*)/)
    {
	$word = $1;
	$otherfactors = $2;
    }
    else
    {
	$word = $$WORD[$i];
	$otherfactors = "";
    }
    if ($asr){
      $word = lc($word); #make sure ASR output is not uc
    }

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
  print $$MARKUP[$#$MARKUP];
  print "\n";
}

# store away xml markup
sub split_xml {
  my ($line) = @_;
  my (@WORD,@MARKUP);
  my $i = 0;
  $MARKUP[0] = "";
  while($line =~ /\S/) {
    # XML tag
    if ($line =~ /^\s*(<\S[^>]*>)(.*)$/) {
      my $potential_xml = $1;
      my $line_next = $2;
      # exception for factor that is an XML tag
      if ($line =~ /^\S/ && scalar(@WORD)>0 && $WORD[$i-1] =~ /\|$/) {
	$WORD[$i-1] .= $potential_xml;
	if ($line_next =~ /^(\|+)(.*)$/) {
	  $WORD[$i-1] .= $1;
	  $line_next = $2;
	}
      }
      else {
        $MARKUP[$i] .= $potential_xml." ";
      }
      $line = $line_next;
    }
    # non-XML text
    elsif ($line =~ /^\s*([^\s<>]+)(.*)$/) {
      $WORD[$i++] = $1;
      $MARKUP[$i] = "";
      $line = $2;
    }
    # '<' or '>' occurs in word, but it's not an XML tag
    elsif ($line =~ /^\s*(\S+)(.*)$/) {
      $WORD[$i++] = $1;
      $MARKUP[$i] = "";
      $line = $2;
      }
    else {
      die("ERROR: huh? $line\n");
    }
  }
  chop($MARKUP[$#MARKUP]);
  return (\@WORD,\@MARKUP);
}
