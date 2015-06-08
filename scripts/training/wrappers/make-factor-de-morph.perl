#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use Encode;
use FindBin qw($RealBin);
my ($in,$out,$tmpdir) = @ARGV;

`mkdir -p $tmpdir`;
`$RealBin/../../tokenizer/deescape-special-chars.perl < $in | /home/pkoehn/statmt/bin/unicode2latin1.perl > $tmpdir/tok.$$`;
`/home/pkoehn/statmt/bin/run-lopar-tagger.perl $tmpdir/tok.$$ $tmpdir/lopar.$$`;

open(LOPAR,"$tmpdir/lopar.$$");
open(OUT,"|$RealBin/../../tokenizer/escape-special-chars.perl >$out");
while(<LOPAR>) {
    chomp;
    s/ +/ /g;
    s/^ //;
    s/ $//;
    my $first = 1;
    foreach (split) {
        die("ERROR: choked on token '$_'") unless /^(.+)_([^_]+)_(.+)$/;
        my ($word,$morph,$lemma) = ($1,$2,$3);
	print OUT " " unless $first;
	$first = 0;
	print OUT encode('utf8', decode('iso-8859-1', $morph));
    }
    print OUT "\n";
}
close(LOPAR);
close(OUT);
