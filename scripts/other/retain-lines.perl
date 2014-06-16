#!/usr/bin/perl

#retain lines in clean.lines-retained.1
use strict;
use warnings;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

my $retainPath = $ARGV[0];

open(LINE_RETAINED, $retainPath);
my $retainLine = <LINE_RETAINED>;

my $lineNum = 0;
while (my $line = <STDIN>) {
    chomp($line);
    ++$lineNum;

    if ($retainLine == $lineNum) {
        print "$line\n";
	if ($retainLine = <LINE_RETAINED>) {
	    # do nothing
	}
        else {
            # retained lines is finished.
            $retainLine = 0;
        }
    }
}
