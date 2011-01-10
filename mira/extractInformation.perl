#!/usr/bin/perl

use strict;

my $in = $ARGV[0];
my $out_dir = "extractedInformation";
if (not -d $out_dir) {
    mkdir($out_dir);
}

open IN, "<$in" or die "Unable to open \"$in\" for reading";

open OUT1, ">$out_dir/newWeights.txt";
open OUT2, ">$out_dir/oracleReferenceRatio.txt";
open OUT3, ">$out_dir/errorAfterMixing.txt";
open OUT4, ">$out_dir/errorAfterDumping.txt";

open OUT5, ">$out_dir/rank0_mixing_before.txt";
open OUT6, ">$out_dir/rank0_mixing_after.txt";
open OUT7, ">$out_dir/rank0_dumping_cumulativeWeights.txt";
open OUT8, ">$out_dir/rank0_dumping_totalWeights.txt";
open OUT9, ">$out_dir/rank0_dumping_averageTotalWeights.txt";


while(<IN>){
    if ($_ =~ m/New weights: <<\{((?:\-?\d*?\.?\d*?, ){1,})0, \}>>/) {
	my @weights = split(/,/, $1);
	print OUT1 "@weights\n";	
    }
    elsif ($_ =~ m/oracle length: (\d*) reference length: (\d*)/) {
	my $ratio = $1/$2;
	print OUT2 "$ratio\n";
    }
    elsif ($_ =~ m/summed error after mixing weights: (\d*?\.?\d*)/) {
	print OUT3 "$1\n";
    }
    elsif ($_ =~ m/summed error after dumping weights: (\d*?\.\d*)/) {
	print OUT4 "$1\n";
    }
    elsif ($_ =~ m/summed error after dumping weights: (\d*?\.?\d*)/) {
	print OUT4 "$1\n";
    }
    elsif ($_ =~ m/Rank 0, before mixing: <<\{((?:.*?, ){1,})0, \}>>/) {
	my @weights = split(/,/, $1);
	print OUT5 "@weights\n";
    }
    elsif ($_ =~ m/Rank 0, after mixing: <<\{((?:.*?, ){1,})0, \}>>/) {
	my @weights = split(/,/, $1);
	print OUT6 "@weights\n";
    }
    elsif ($_ =~ m/Rank 0, cumulative weights: <<\{((?:.*?, ){1,})0, \}>>/) {	
	my @weights = split(/,/, $1);
	print OUT7 "@weights\n";
    }
    elsif ($_ =~ m/Rank 0, total weights: <<\{((?:.*?, ){1,})0, \}>>/) {
	my @weights = split(/,/, $1);
	print OUT8 "@weights\n";
    }
    elsif ($_ =~ m/Rank 0, average total weights: <<\{((?:.*?, ){1,})0, \}>>/) {
	my @weights = split(/,/, $1);
	print OUT9 "@weights\n";
    }
}




