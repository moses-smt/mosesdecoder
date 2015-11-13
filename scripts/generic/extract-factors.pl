#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# $Id$
#extract-factors.pl: extract only the desired factors from a factored corpus
#usage: extract-factors corpusfile factor-index factor-index ... > outfile
#factor indices start at 0
#factor indices too large ought to be ignored

use warnings;
use strict;

my ($filename, @factors) = @ARGV;
my %indices = map {$_ => 1} @factors;

open(INFILE, "<$filename") or die "couldn't open '$filename' for read: $!\n";
while(my $line = <INFILE>)
{
	chop $line;
	print join(' ', map {my $i = 0; join('|', grep($indices{$i++}, split(/\|/, $_)))} split(/\s+/, $line)) . "\n";
}
close(INFILE);
