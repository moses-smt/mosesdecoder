#!/usr/bin/perl -w

use strict;

my ($size,$in,$out) = @ARGV;

open(IN,$in);
open(OUT,">$out");
binmode(IN, ":utf8");
binmode(OUT, ":utf8");

while(<IN>) {
    my $first = 1;
    chomp; s/\s+/ /g; s/^ //; s/ $//;
    foreach my $word (split) {
        if (length($word) > $size) {
	    $word = substr($word,0,$size);
        }
	print OUT " " unless $first; $first = 0;
	print OUT $word;
    }
    print OUT "\n";
}
close(OUT);
close(IN);
