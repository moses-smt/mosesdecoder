#!/usr/bin/perl -w

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

use FindBin qw($RealBin);
use strict;

while(my $line = <STDIN>) 
{
	chomp($line);

    #escape special chars
    $line =~ s/\&/\&amp;/g;   # escape escape
    $line =~ s/\|/\&#124;/g;  # factor separator
    $line =~ s/\</\&lt;/g;    # xml
    $line =~ s/\>/\&gt;/g;    # xml
    $line =~ s/\'/\&apos;/g;  # xml
    $line =~ s/\"/\&quot;/g;  # xml
    $line =~ s/\[/\&#91;/g;   # syntax non-terminal
    $line =~ s/\]/\&#93;/g;   # syntax non-terminal

	print "$line\n";
}

