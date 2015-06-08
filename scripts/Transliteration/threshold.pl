#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use utf8;
require Encode;
use IO::Handle;

$input = <STDIN>;
#print $input;

$filename = shift or die "Error: missing hindi urdu file argument!\n";
open(FILE,$filename) or die "Error: unable to open file \"$filename\"!\n";

binmode(STDIN,  ':utf8');
binmode(STDOUT, ':utf8');
binmode(STDERR, ':utf8');
binmode(FILE, ':utf8');
$c=0;
while (<FILE>)
{
        chomp;
        @F=split("\t");
        $hash{$F[0]."\t".$F[1]}=$F[$#F];
        $c++;
        if($F[$#F] < $input)
        {
                print "$F[0]\t$F[1]\n";
        }

}close FILE;

