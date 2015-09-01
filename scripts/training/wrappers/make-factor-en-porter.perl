#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use FindBin qw($RealBin);

my ($in,$out,$tmpdir) = @ARGV;

my $porter_in = "$tmpdir/porter-in.$$";
`$RealBin/../../tokenizer/deescape-special-chars.perl < $in > $porter_in`;
`/home/pkoehn/statmt/bin/porter-stemmer $porter_in | $RealBin/../../tokenizer/escape-special-chars.perl > $out`;
