#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use strict;

my $minChars = $ARGV[0];
my $maxChars = $ARGV[1];
my $inputStem = $ARGV[2];
my $source = $ARGV[3];
my $target = $ARGV[4];
my $outputStem = $ARGV[5];
my $linesRetained = $ARGV[6];

open(IN_SOURCE, "<:encoding(UTF-8)", "$inputStem.$source") or die "cannot open $inputStem.$source";
open(IN_TARGET, "<:encoding(UTF-8)", "$inputStem.$target") or die "cannot open $inputStem.$target";

open(OUT_SOURCE, ">:encoding(UTF-8)", "$outputStem.$source") or die "cannot open $outputStem.$source";
open(OUT_TARGET, ">:encoding(UTF-8)", "$outputStem.$target") or die "cannot open $outputStem.$target";

open(LINE_RETAINED, ">:encoding(UTF-8)", "$linesRetained");

my $lineNum = 0;
while (my $lineSource = <IN_SOURCE>) {
    ++$lineNum;
    #print STDERR "$lineNum ";

    chomp($lineSource);
    my $lineTarget = <IN_TARGET>;
    chomp($lineTarget);

    my $lenSource = length($lineSource);
    my $lenTarget = length($lineTarget);

    if ($lenSource < $minChars || $lenSource > $maxChars
	|| $lenTarget < $minChars || $lenTarget > $maxChars) {
	# do nothing
    }
    else {
	print OUT_SOURCE "$lineSource\n";
        print OUT_TARGET "$lineTarget\n";
	print LINE_RETAINED "$lineNum\n";
    }
}

close(OUT_SOURCE);
close(OUT_SOURCE);
close(LINE_RETAINED);
