#!/usr/bin/perl -w

use strict;

my ($in,$out,$tmpdir) = @ARGV;
use Encode;
use FindBin qw($RealBin);

`mkdir -p $tmpdir`;
`$RealBin/../../tokenizer/deescape-special-chars.perl < $in | /home/pkoehn/statmt/bin/unicode2latin1.perl > $tmpdir/tok.$$`;
`/home/pkoehn/statmt/bin/run-lopar-tagger.perl $tmpdir/tok.$$ $tmpdir/lopar.$$`;

open(LOPAR,"$tmpdir/lopar.$$");
open(OUT,"|$RealBin/../../tokenizer/escape-special-chars.perl > $out");
while(<LOPAR>) {
    chomp;
    s/ +/ /g;
    s/^ //;
    s/ $//;
    my $first = 1;
    foreach (split) {
        die("ERROR: choked on token '$_'") unless /^(.+)_([^_]+)_(.+)$/;
        my ($word,$pos,$lemma) = ($1,$2,$3);
	$pos =~ s/\..+//;
	print OUT " " unless $first;
	$first = 0;
	print OUT encode('utf8', decode('iso-8859-1', $pos));
    }
    print OUT "\n";
}
close(LOPAR);
close(OUT);
