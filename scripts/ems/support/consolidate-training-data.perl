#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# $Id: consolidate-training-data.perl 928 2009-09-02 02:58:01Z philipp $

use warnings;
use strict;

my ($in,$out,$consolidated,@PART) = @ARGV;

`rm $consolidated.$in`  if -e "$consolidated.$in";
`rm $consolidated.$out` if -e "$consolidated.$out";
if (scalar @PART == 1) {
    my $part = $PART[0];
    `ln -s $part.$in $consolidated.$in`;
    `ln -s $part.$out $consolidated.$out`;
    exit;
}

foreach my $part (@PART) {
    die("ERROR: no part $part.$in or $part.$out")
	if (! -e "$part.$in" || ! -e "$part.$out");
    my $in_size = `cat $part.$in | wc -l`;
    my $out_size = `cat $part.$out | wc -l`;
    die("number of lines don't match: '$part.$in' ($in_size) != '$part.$out' ($out_size)")
	if $in_size != $out_size;
    `cat $part.$in >> $consolidated.$in`;
    `cat $part.$out >> $consolidated.$out`;
}
