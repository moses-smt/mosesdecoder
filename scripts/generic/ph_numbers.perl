#!/usr/bin/perl -w
package ph_numbers;

# Script to recognize and replace numbers in Moses training corpora
# and decoder input
#
# (c) 2013 TAUS

use strict;

run() unless caller();
use Getopt::Std;

my $debug = $ENV{DEBUG} || 0;

sub run {
    my %opts;
    if(!getopts('s:t:cm:hl',\%opts) || $opts{h}) {
	print "Usage: perl $0 [-s source_locale][-t target_locale][-c][-h][-l][-m symbol] < in > out\n";
	exit;
    }
    my $sourceLocale = $opts{s} || "";
    my $targetLocale = $opts{t} || "";
    my $numberSymbol = $opts{m} || '@NUM@';
    while(<>) {
	chomp;
	print recognize($_,$opts{c},$opts{l},$numberSymbol,$_),"\n";
    }
}

sub recognize {
    my $line = shift;
    my $corpusMode = shift;
    my $legacyMode = shift;
    my $numberSymbol = shift || '@NUM@';

    # [-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?
    # while(/\G(.*?)\s*([+-]?\p{Digit}+[+-\p{Digit}\.,eE])/) {
    my $output = "";
    my $remainder = "";
    while($line =~ /\G(.*?)(\s*)([+-]?\p{Digit}*[\.,]?\p{Digit}+[\p{Digit}\.,+-eE]*)/g) {
	my $between = $1;
	my $number = $3;
	print STDERR "Between: x${between}x\n" if $debug;
	print STDERR "Number: x${number}x\n" if $debug;
	# If there are more numbers separated by whitespace, add these
	my $numberContinuation = "";
	while($line = /\G(\s+)([\p{Digit}\.,+-eE]*)/g) {
	    $numberContinuation .= $1.$2;
	}
	$number .= $numberContinuation;
	$output .= $between;
	if($corpusMode) {
	    $output .= $2.$numberSymbol;
	}
	else {
	    if($legacyMode) {
		$output .= $2."<ne translation=\"$number\">$numberSymbol</ne>";
	    }
	    else {
		$output .= $2."<ne translation=\"$numberSymbol\" entity=\"$number\">$numberSymbol</ne>";
	    }
	}
	$remainder = $';
    }
    print STDERR "Remainder: x".$remainder."x\n" if $debug; 
    print STDERR "\n" if $debug; 
    $output .= $remainder if $remainder; 
    return $output; 
}

1;
