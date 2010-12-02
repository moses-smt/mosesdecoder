#!/usr/bin/env perl

use strict;

my $in = $ARGV[0];

open IN, "<$in" or die "Unable to open \"$in\" for reading";

open OUT1, ">precision1.txt";
open OUT2, ">precision2.txt";
open OUT3, ">precision3.txt";
open OUT4, ">precision4.txt";
open OUT5, ">oracleToReferenceRatio.txt";
open OUT6, ">oracleToReferenceRatioInHistory.txt";

open OUT7, ">sourceReferenceRatio.txt";
open OUT8, ">hypothesisReferenceRatio.txt";
open OUT9, ">BP.txt";

open OUT10, ">trackWeightChanges.txt";
open OUT11, ">sourceOracleRatio.txt";

my $averagePrecision1 = 0;
my $averagePrecision2 = 0;
my $averagePrecision3 = 0;
my $averagePrecision4 = 0;

my $examples = 0;
my $oracleToReferenceAverage = 0;
my $oracleToReferenceHistoryAverage = 0;

my $examples2 = 0;
my $examples3 = 0;
my $sourceReferenceRatio = 0;
my $hypoReferenceRatio = 0;
my $BP = 0;
my $sourceOracleRatio = 0;

while(<IN>){
#    if ($_ =~ "precisionHistory 1: (.*) .*\n"){
#	print OUT1 "$1\n";
#	$averagePrecision1 += $1;
#    }
#    elsif ($_ =~ "precisionHistory 2: (.*) .*\n"){
#	print OUT2 "$1\n";
#	$averagePrecision2 += $1;
#    }
#    elsif ($_ =~ "precisionHistory 3: (.*) .*\n"){
#	print OUT3 "$1\n";
#	$averagePrecision3 += $1;
#    }
#    elsif ($_ =~ "precisionHistory 4: (.*) .*\n"){
#	print OUT4 "$1\n";
#	$averagePrecision4 += $1;
#    }
#    elsif ($_ =~ "ratio  length: (.*)\n"){
#	# oracle to reference ratio
#	print OUT5 "$1\n";
#	$oracleToReferenceAverage += $1;	
#    }
#    elsif ($_ =~ "ratio  history: (.*)\n"){
#	# oracle to reference ratio as in the history
#	print OUT6 "$1\n";
#	$oracleToReferenceHistoryAverage += $1;
#    }


    if ($_ =~ "source/reference ratio: (.*)\n"){
	print OUT7 "$1\n";
	#$examples += 1;
	$sourceReferenceRatio += $1;
    }
    elsif ($_ =~ "hypo length/reference length: (.*)\n") {
	print OUT8 "$1\n";
	$examples2 += 1;
	$hypoReferenceRatio += $1;
    }
    elsif ($_ =~ "BP: (.*)\n") {
	print OUT9 "$1\n";
	$examples3 += 1;
	$BP += $1;
    }


    #elsif ($_ =~ "New weights: <<\{((-?\d*\.\d*), ){1,}\}>>\n") {
    elsif ($_ =~ "New weights: <<\{((?:.*?, ){1,})0, \}>>\n") {
	my @weights = split(/,/, $1);
	print OUT10 "@weights\n";
    }
    elsif ($_ =~ "source/oracle ratio: (.*)\n") {
	print OUT11 "$1\n";
	$sourceOracleRatio += $1;
	$examples += 1;
    }
}

#print OUT1 "average: ", $averagePrecision1/$examples ,"\n";
#print OUT2 "average: ", $averagePrecision2/$examples ,"\n";
#print OUT3 "average: ", $averagePrecision3/$examples ,"\n";
#print OUT4 "average: ", $averagePrecision4/$examples ,"\n";

#print OUT5 "average: ", $oracleToReferenceAverage/$examples, "\n";
#print OUT6 "average: ", $oracleToReferenceHistoryAverage/$examples, "\n";

#print OUT7 "average: ", $sourceReferenceRatio/$examples, "\n";
#print OUT8 "average: ", $hypoReferenceRatio/$examples2, "\n";
#print OUT9 "average: ", $BP/$examples3, "\n";

if ($examples > 0) {
    print OUT11 "average: ", $sourceOracleRatio/$examples, "\n";
}


