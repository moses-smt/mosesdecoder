#!/usr/bin/perl

#add_empties_to_phrase_table: go through an old-style pharaoh phrase table (no empty target sources) and add one such line for each single-word source phrase in the table,
#complete with factors (note the number and type of factors are hardcoded here);
#also add deletion-cost factors as necessary to all lines
#Evan Herbst 7 / 11 / 06

#usage: aetpt INPUT_PTABLE OUTPUT_PTABLE

my ($inputFile, $outputFile) = @ARGV;
my $DELETIONCOST = 2.718; #weight for an individual deletion

open(INFILE, "<$inputFile") or die "couldn't open '$inputFile' for read\n";
open(OUTFILE, ">$outputFile") or die "couldn't open '$outputFile' for write\n";
my ($lastSrcPhrase, $srcPhrase);
while(my $line = <INFILE>)
{
	chop($line);
	$lastSrcPhrase = $srcPhrase;
	my @tokens = split(/\|\|\|/, $line);
	$srcPhrase = $tokens[0];
	if($srcPhrase ne $lastSrcPhrase && $srcPhrase =~ /^\s*\S+\s*$/) #new source phrase of a single word; add deletion line
	{
		print OUTFILE "$srcPhrase |||  ||| 1 1 1 1 2.718 $DELETIONCOST\n";
	}
	print OUTFILE "$line 1\n";
}
close(INFILE);
close(OUTFILE);
