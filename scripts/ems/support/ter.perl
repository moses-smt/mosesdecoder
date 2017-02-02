#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use strict;
use FindBin qw($RealBin);

my ($jar, $hyp,$ref,$tmp) = @ARGV;
`mkdir -p $tmp`;
`$RealBin/create-xml.perl test < $hyp > $tmp/hyp`;
`$RealBin/create-xml.perl ref  < $ref > $tmp/ref`;
`java -jar $jar -h $tmp/hyp -r $tmp/ref -o ter -n $tmp/out`;
print `cat $tmp/out.ter`;

