#!/usr/bin/perl -w

use strict;
use FindBin qw($Bin);

my $scoreExe	= $ARGV[0];

my $cmd = "$scoreExe $Bin/extract.sorted $Bin/lex.f2e $Bin/phrase-table.4.half.f2e  --GoodTuring \n";

`$cmd`;
