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
    my $numberSymbol = $opts{m} || '@num@';
    while(<>) {
	chomp;
	print mark_numbers($_,$opts{c},$opts{l},$numberSymbol,$_),"\n";
    }
}

sub mark_numbers {
    my $input = shift;
    my $corpusMode = shift;
    my $legacyMode = shift;
    my $numberSymbol = shift || '@num@';

    my $numref = recognize($input);
    my $input_length = length($input);
    my $output = "";
    my $position = 0;
    for(my $i = 0 ; $i < scalar(@{$numref}) ; $i++) {
	my $numstart = $numref->[$i][0];
	my $numend = $numref->[$i][1];
	if($position < $numstart) {
	    $output .= substr($input,$position,$numstart-$position);
	}
	my $number = substr($input,$numstart,$numend-$numstart);
	if($corpusMode) {
      $output .= $numberSymbol;
	}
	else {
	    if($legacyMode) {
		$output .= "<ne translation=\"$number\">$numberSymbol</ne>";
	    }
	    else {
		$output .= "<ne translation=\"$numberSymbol\" entity=\"$number\">$numberSymbol</ne>";
	    }
	}
	$position = $numend;
    }
    $output .= substr($input,$position); 
    return $output; 
}

sub recognize {
    my $input = shift;

    my @recognized = ();
    while($input =~ /\G(.*?)(\s*)([+\-]?\p{Digit}*[\.,]?\p{Digit}+[\p{Digit}\.,+\-eE]*)/g) {
	my $start = $-[3];
	my $end = $+[3];
	while($input =~ /\G(\s+)(\p{Digit}+[\p{Digit}\.,+\-eE]*)/gc) {
	    $end = $+[2];
	}
	push @recognized,[$start,$end];
    }
    return \@recognized;
}

1;
