#!/usr/bin/perl -w

# $Id: lopar2pos.pl 1307 2007-03-14 22:22:36Z hieuhoang1972 $
#lopar2pos: extract POSs from LOPAR output
#usage: lopar2pos.pl CORPUS.lopar > CORPUS.pos

my $infilename = shift @ARGV;
open(INFILE, "<$infilename") or die "couldn't open '$infilename' for read: $!\n";
while(my $line = <INFILE>)
{
	my @words = split(/\s+/, $line);
	my @tags = map {$_ =~ /^[^_]*_([A-Z]+)/; $1} @words;
	print join(' ', @tags) . "\n";
}
close(INFILE);
