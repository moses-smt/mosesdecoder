#!/usr/bin/perl -w

# $Id: train-recaser.perl 1326 2007-03-26 05:44:27Z bojar $
use strict;
use Getopt::Long "GetOptions";

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

# apply switches
my ($MODEL, $UNBUFFERED);
die("truecase.perl --model MODEL [-b] < in > out")
    unless &GetOptions('model=s' => \$MODEL,'b|unbuffered' => \$UNBUFFERED)
    && defined($MODEL);
if (defined($UNBUFFERED) && $UNBUFFERED) { $|=1; }

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
      $MARKUP[$i] .= $1." ";
      $line = $2;
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
