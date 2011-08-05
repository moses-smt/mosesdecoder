#!/usr/bin/perl -w

use strict;

die("ERROR syntax: input-from-sgm.perl < in.sgm > in.txt") 
    unless scalar @ARGV == 0;

while(<STDIN>) {
    if (/<seg[^>]+>\s*(.*)\s*<\/seg>/i) {
	my $input = $1;
	$input =~ s/\s+/ /g;
	$input =~ s/^ //g;
	$input =~ s/ $//g;
	print $input."\n";
    }
}
