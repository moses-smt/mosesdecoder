#!/usr/bin/perl -w

use strict;
use FindBin qw($Bin);

my $scoreExe	= $ARGV[0];

my $outPath = "$Bin/phrase-table.4.half.f2e";
my $cmdMain = "$scoreExe $Bin/extract.sorted $Bin/lex.f2e $outPath  --GoodTuring \n";

`$cmdMain`;

my $truthPath = "$Bin/truth/results.txt";
my $cmd = "diff $outPath $truthPath | wc -l";

my $numDiff = 554;
$numDiff = `$cmd`;

if ($numDiff == 0)
{
  print STDERR "SUCCESS\n";
  exit 0;
}
else
{
  print STDERR "FAILURE. Ran $cmdMain\n";
  exit 1;
}
