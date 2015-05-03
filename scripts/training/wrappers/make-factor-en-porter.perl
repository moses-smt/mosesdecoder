#!/usr/bin/perl -w

use strict;
use FindBin qw($RealBin);

my ($in,$out,$tmpdir) = @ARGV;

my $porter_in = "$tmpdir/porter-in.$$";
`$RealBin/../../tokenizer/deescape-special-chars.perl < $in > $porter_in`;
`/home/pkoehn/statmt/bin/porter-stemmer $porter_in | $RealBin/../../tokenizer/escape-special-chars.perl > $out`;
