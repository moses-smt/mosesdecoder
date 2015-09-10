#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# $Id$
# given a list of files, combines them to a single corpus (sent to stdout)

use strict;
use warnings;
use Getopt::Long;
use IO::File;
use File::Basename;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

my @addfactors = @ARGV;
die "usage: combine_factors.pl corpusfile1 corpusfile2 ..."
  if 0 == scalar @addfactors;

my @streams = map {
  my $fn = $_;
  my $opn = ($fn =~ /\.gz$/ ? "zcat $fn |" : "$fn");
  my $stream = new IO::File;
  $stream->open($opn) or die "Can't open '$opn'";
  binmode($stream, ":utf8");
  $stream;
} @addfactors;

my $nr=0;
my $firststream = shift @streams;
shift @addfactors; # just to keep the lengths sync'ed
$_ = readline($firststream);
while (defined $_) {
  $nr++;
  print STDERR "." if $nr % 10000 == 0;
  print STDERR "($nr)" if $nr % 100000 == 0;
  my ($intokens,$MARKUP) = split_xml($_);
  # load lines of corresponding streams and ensure equal number of words
  my @lines_of_extratoks;
  foreach my $factor (0..$#streams) {
    my $line = readline($streams[$factor]);
    die "Additional factor file $addfactors[$factor] contains too few sentences!"
      if !defined $line;
    chomp($line);
    $line =~ s/\s+/ /g; $line =~ s/^ //; $line =~ s/ $//;
    my @toks = split / /, $line;
    die "Incompatible number of words in factor $factor on line $nr. ($#toks != $#$intokens)"
      if $#toks != $#$intokens;
    $lines_of_extratoks[$factor] = \@toks;
  }

  # for every token, print the factors in the order as user wished
  for(my $i=0; $i<=$#$intokens; $i++) {
    print "" if $i && $$MARKUP[$i] eq '';
    print $$MARKUP[$i];

    my $token = $$intokens[$i];
    my @outtoken = ();
    push @outtoken, $token; # add the first one
    # print STDERR "Token: $token\n";
    foreach my $factor (0..$#streams) {
      my $f = $lines_of_extratoks[$factor]->[$i];
      die "Missed factor value for word $i+1 on line $nr in $addfactors[$factor]"
        if !defined $f || $f eq "";
      push @outtoken, $f;
    }
    print " " if $i != 0;
    print join("|", @outtoken);
  }
  print $$MARKUP[$#$MARKUP];
  print "\n";
  $_ = readline($firststream);
}
close $firststream;
print STDERR "Done.\n";

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



