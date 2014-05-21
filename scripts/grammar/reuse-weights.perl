#!/usr/bin/perl

# $Id: reuse-weights.perl 41 2007-03-14 22:54:18Z hieu $

use strict;

die("ERROR: syntax: reuse-weights.perl weights.ini < moses.ini > weighted.ini")
    unless scalar @ARGV == 1;

my ($weight_file) = @ARGV;
open(WEIGHT,$weight_file) or die("ERROR: could not open weight file: $weight_file");

my %weights;
while(<WEIGHT>) {
    if (/^(\w+\d+)=/) {
        $weights{$1} = $_;
    }
}
close(WEIGHT);

while(<STDIN>) {
    if (/^(\w+\d+)=/) {
        if (exists($weights{$1})) {
            print $weights{$1};
        }
        else {
            print;
        }        
    }
    else {
        print;
    }
}
