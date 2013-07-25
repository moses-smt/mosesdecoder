#!/usr/bin/perl

# $Id$
# convert a phrase-table with alignment in Moses' dead-end format
#    a . ||| A . ||| (0) (0,1) ||| (0,1) (1) ||| 1 0.0626124 1 0.032119 2.718
# to
#    a . ||| A . ||| 1 0.0626124 1 0.032119 2.718 ||| 0-0 1-0 1-1


use strict;
use warnings;
use Getopt::Long;
use IO::File;
use File::Basename;

sub ConvertAlignment($);

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");


my $lineNum = 0;
while (my $line = <STDIN>) {
  chomp($line);
	++$lineNum;

  my @toks = split(/\|/, $line);
  my $source = $toks[0];
  my $target = $toks[3];
  my $scores = $toks[12];

  my $alignS = $toks[6];
  my $align = ConvertAlignment($alignS);

  print "$source|||$target|||$scores ||| $align\n";

}

sub ConvertAlignment($ $)
{
  my $ret = "";
  my $alignS = shift;
  $alignS =~ s/^\s+//;
  $alignS =~ s/\s+$//;

  #print "alignS=$alignS\n";

  my @toks = split(/ /, $alignS);
  for (my $posS = 0; $posS < scalar @toks; ++$posS) {
    my $tok = $toks[$posS];
    $tok = substr($tok, 1, length($tok) - 2);
    #print "tok=$tok\n";

    my @posTvec = split(/,/, $tok);
    for (my $j = 0; $j < scalar @posTvec; ++$j) {
      my $posT = $posTvec[$j];
      $ret .= "$posS-$posT ";
    }
  }

  #print "ret=$ret \n";
  return $ret;
}


