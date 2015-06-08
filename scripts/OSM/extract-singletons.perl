#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

#use strict;
use warnings;
use Getopt::Std;
getopts('q');

my $target = shift;
my $source = shift;
my $align = shift or die "
Usage: extract-singletons.perl target source align

";
open(TARGET,$target) or die "Error: unable to open target file \"$target\"!\n";
open(SOURCE,$source) or die "Error: unable to open source file \"$source\"!\n";
open(ALIGN,$align) or die "Error: unable to open alignment file \"$align\"!\n";

while (<TARGET>) {
    unless (defined $opt_q) {
	print STDERR "\r$M" if ++$M%1000 == 0;
    }
    @T = split;
    $_ = <SOURCE>;
    @S = split;
    $_ = <ALIGN>;
    @A = split;

    my(@source_links,@target_links);
    for( $i=0; $i<=$#A; $i+=2 ) {
	$target_links[$A[$i]]++;
	$source_links[$A[$i+1]]++;
    }

    for( $i=0; $i<=$#A; $i+=2 ) {
	if ($target_links[$A[$i]] == 1 && $source_links[$A[$i+1]] == 1 &&
	    $T[$A[$i]] eq $S[$A[$i+1]])
	{
	    $count{$S[$A[$i+1]]}++; # Print this if it only occurs here
	}
	else {
	    $count{$S[$A[$i+1]]}+=2; # Don't print this
	}
    }
}

foreach $w (sort keys %count) {
    print "$w\n" if $count{$w}==1;
}
