#!/usr/bin/perl -W 
# script for preprocessing language data prior to tokenization
# Start by Ulrich Germann, after noticing systematic preprocessing errors
# in some of the English Europarl data.

use strict;
use Getopt::Std;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

sub usage
{
  print "Script for preprocessing of raw language data prior to tokenization\n";
  print "Usage: $0 -l <language tag>\n";
}

my %args;
getopt('l=s h',\%args);
usage() && exit(0) if $args{'h'};

if ($args{'l'} eq "en")
  {
      while (<>)
      {
	  s/([[:alpha:]]\') s\b/$1s/g;
	  print;
      }
      
  }
else
{
    print while <>;
}
