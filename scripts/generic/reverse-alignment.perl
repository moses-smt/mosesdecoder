#!/usr/bin/perl -w

use strict;

my $line;
while ($line = <STDIN>)
{
  chomp($line);
  my @toks = split(/ /, $line);
  
  foreach (my $i = 0; $i < @toks; ++$i)
  {
    my $tok = $toks[$i];
    my @alignPair = split(/-/, $tok);
    (@alignPair == 2) or die("Something wrong");
    print $alignPair[1]."-".$alignPair[0]." ";
  }
  print "\n";
}

