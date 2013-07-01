#!/usr/bin/perl -w

# Script to recognize and replace numbers in Moses training corpora
# and decoder input
#
# (c) 2013 TAUS

use strict;

use Getopt::Std;

my $debug = $ENV{DEBUG} || 0;

my %opts;
if(!getopts('s:t:cm:hl',\%opts) || $opts{h}) {
    print "Usage: perl $0 [-s source_locale][-t target_locale][-c][-h][-l][-m symbol] < in > out\n";
    exit;
}
my $sourceLocale = $opts{s} || "";
my $targetLocale = $opts{t} || "";
my $numberSymbol = $opts{m} || '@NUM@';

while(<>) {
    # [-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?
    # while(/\G(.*?)\s*([+-]?\p{Digit}+[+-\p{Digit}\.,eE])/) {
    chomp;
    my $output = "";
    my $remainder = "";
    while(/\G(.*?)(\s*)([+-]?\p{Digit}*[\.,]?\p{Digit}+[\p{Digit}\.,+-eE]*)/g) {
	print STDERR "Between: x$1x\n" if $debug;
	print STDERR "Number: x$3x\n" if $debug;
	$output .= $1;
	if($opts{c}) {
	    $output .= $2.$numberSymbol;
	}
	else {
	    if($opts{l}) {
		$output .= $2."<ne translation=\"$3\">$numberSymbol</ne>";
	    }
	    else {
		$output .= $2."<ne translation=\"$numberSymbol\" entity=\"$3\">$numberSymbol</ne>";
	    }
	}
	$remainder = $';
    }
    print STDERR "Remainder: x".$remainder."x\n" if $debug;
    print STDERR "\n" if $debug;
    $output .= $remainder if $remainder;
    $output .= "\n";
    print $output;
}
