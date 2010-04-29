#!/usr/bin/perl

$INFILE_name=$ARGV[0];
$OUTFILE_name=$ARGV[1];

open(INFILE,"<$INFILE_name") or die "No input files!";
open(OUTFILE,">$OUTFILE_name") or die "No output files!";

while (<INFILE>) {
	s/\s+$//;
#	s/\,|\[|\]//g;
	print OUTFILE $_;
	print OUTFILE "\n";
}

close INFILE;


