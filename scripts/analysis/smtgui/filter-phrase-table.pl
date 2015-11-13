#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# $Id$
#by Philipp Koehn, de-augmented by Evan Herbst
#filter a phrase table for a specific input corpus
#arguments: phrasetable_filename input_filename factor_index (0...)
#outputs to phrasetable_filename.short

#similar function to filter-model-given-input.pl, but only operates
#on the phrase table and doesn't require that any subdirectories exist

use warnings;
use strict;

my $MAX_LENGTH = 10;

my ($file, $input, $source_factor) = @ARGV;
my $dir = ".";

    # get tables to be filtered (and modify config file)
    my (@TABLE,@TABLE_FACTORS,@TABLE_NEW_NAME,%CONSIDER_FACTORS);
		push @TABLE,$file;

		my $new_name = "$file.short";#"$dir/phrase-table.$source_factor";
		push @TABLE_NEW_NAME,$new_name;

		$CONSIDER_FACTORS{$source_factor} = 1;
		push @TABLE_FACTORS,$source_factor;

    # get the phrase pairs appearing in the input text
    my %PHRASE_USED;
    die("could not find input file $input") unless -e $input;
    open(INPUT,$input);
    while(my $line = <INPUT>) {
	chop($line);
	my @WORD = split(/ +/,$line);
	for(my $i=0;$i<=$#WORD;$i++) {
	    for(my $j=0;$j<$MAX_LENGTH && $j+$i<=$#WORD;$j++) {
		foreach (keys %CONSIDER_FACTORS) {
		    my @FACTOR = split(/,/);
		    my $phrase = "";
		    for(my $k=$i;$k<=$i+$j;$k++) {
			my @WORD_FACTOR = split(/\|/,$WORD[$k]);
			for(my $f=0;$f<=$#FACTOR;$f++) {
			    $phrase .= $WORD_FACTOR[$FACTOR[$f]]."|";
			}
			chop($phrase);
			$phrase .= " ";
		    }
		    chop($phrase);
		    $PHRASE_USED{$_}{$phrase}++;
		}
	    }
	}
    }
    close(INPUT);

    # filter files
    for(my $i=0;$i<=$#TABLE;$i++) {
	my ($used,$total) = (0,0);
	my $file = $TABLE[$i];
	my $factors = $TABLE_FACTORS[$i];
	my $new_file = $TABLE_NEW_NAME[$i];
	print STDERR "filtering $file -> $new_file...\n";

        if (-e $file && $file =~ /\.gz$/) { open(FILE,"zcat $file |"); }
        elsif (! -e $file && -e "$file.gz") { open(FILE,"zcat $file.gz|"); }
        elsif (-e $file) { open(FILE,$file); }
	else { die("could not find model file $file");  }

	open(FILE_OUT,">$new_file");

	while(my $entry = <FILE>) {
	    my ($foreign,$rest) = split(/ \|\|\| /,$entry,2);
	    $foreign =~ s/ $//;
	    if (defined($PHRASE_USED{$factors}{$foreign})) {
		print FILE_OUT $entry;
		$used++;
	    }
	    $total++;
	}
	close(FILE);
	close(FILE_OUT);
	printf STDERR "$used of $total phrases pairs used (%.2f%s) - note: max length $MAX_LENGTH\n",(100*$used/$total),'%';
    }
