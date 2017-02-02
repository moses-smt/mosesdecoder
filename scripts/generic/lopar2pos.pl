#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# $Id$
#lopar2pos: extract POSs from LOPAR output
#usage: lopar2pos.pl CORPUS.lopar > CORPUS.pos

use warnings;

my $infilename = shift @ARGV;
open(INFILE, "<$infilename") or die "couldn't open '$infilename' for read: $!\n";
while(my $line = <INFILE>)
{
	my @words = split(/\s+/, $line);
	my @tags = map {$_ =~ /^[^_]*_([A-Z]+)/; $1} @words;
	print join(' ', @tags) . "\n";
}
close(INFILE);
