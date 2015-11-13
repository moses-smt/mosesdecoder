#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;

my $lineNum = 1;
my $countUnparseable = 0;

#open(STDIN);
while(<STDIN>)
{
    my $line = $_;
    $line = trim($line);
    my $len = length($line);
    #print $len." ";

    if ($len == 0) {
	$countUnparseable++;
	print $lineNum."\n";
    }
    else {
	#print $lineNum."\n";
    }

    $lineNum++;
}
#close(STDIN);

print STDERR "num of lines=".($lineNum-1)."\n";
print STDERR "num of unparseable=$countUnparseable\n";

sub trim($)
{
    my $string = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}
