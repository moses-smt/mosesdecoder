#!/usr/bin/env perl

use strict;
#eddie specific
use lib "/exports/informatics/inf_iccs_smt/perl/lib/perl5/site_perl";
use File::Copy;

my ($weight_file, $old_ini_file, $new_ini_file, $input_dir);
my $old_ini_file_copy = "temporary_ini_file";
my %extra_weights;
my $total;

$weight_file = $ARGV[0];
if (-d $weight_file) {
    $input_dir = $ARGV[0];
    $old_ini_file = $ARGV[1];
    print "input dir: $input_dir, old ini file: $old_ini_file\n";
}
else {
    $old_ini_file = $ARGV[1];
    $new_ini_file = $ARGV[2];
    print "weight file: $weight_file, old ini file: $old_ini_file, new ini file: $new_ini_file\n";
}

while (1) {
    print "continue? y/n\n";
    my $continue = <STDIN>;
    if ($continue eq "y\n") {
	last;
    }
    if ($continue eq "n\n") {
	exit;
    } 
}

my ($default_weight,$wordpenalty_weight,$unknownwordpenalty_weight,@phrasemodel_weights,$lm_weight,$distortion_weight,@lexicalreordering_weights);

if ($input_dir) {
    $old_ini_file_copy = $input_dir."/".$old_ini_file_copy;
    my $weights_file_stem = "expt-weights";    
    
    opendir(WEIGHTS, "$input_dir") || die("Cannot open directory"); 

    my @files= readdir(WEIGHTS);
    @files = sort(@files);
    my $processedFiles = 0;
    foreach my $file (@files) {
	if ($file =~ $weights_file_stem) {
	    $weight_file = $input_dir."/".$file;

	    # read in the weights
	    readWeights();

	    # normalise them if necessary
	    normaliseWeights();
	    
	    $new_ini_file = $input_dir."/expt-test.".$processedFiles.".ini";
	    $processedFiles += 1;

	    # write new ini file
	    readOldWriteNewIniFile();	    
	}
    }

    if (-e $old_ini_file_copy ) {
	unlink($old_ini_file_copy);
    }
}
else {
    readWeights();
    normaliseWeights();
    readOldWriteNewIniFile();
}

sub readWeights {
    if (! (open WEIGHTS, "$weight_file")) {
	die "Warning: unable to open weights file $weight_file\n";
    }
    
    while(<WEIGHTS>) {
	chomp;
	my ($name,$value) = split;
	if ($name eq "DEFAULT_") {
	    $default_weight = $value;
	} else {
	    if ($name eq "WordPenalty") {
		$wordpenalty_weight = $value;
	    } elsif ($name eq "!UnknownWordPenalty") {
		$unknownwordpenalty_weight = $value;
	    } elsif ($name =~ /^PhraseModel/) {
		push @phrasemodel_weights,$value;
	    } elsif ($name =~ /^LM/) {
		$lm_weight = $value;
	    } elsif ($name eq "Distortion") {
		$distortion_weight = $value;
	    } elsif ($name =~ /^LexicalReordering/) {
		push @lexicalreordering_weights,$value;
	    } elsif ($name ne "BleuScore") {
		$extra_weights{$name} = $value;
	    }
	}
    }

    close WEIGHTS;
}

sub normaliseWeights {
    # Normalising factor
    $total = abs($wordpenalty_weight+$default_weight) + 
	abs($unknownwordpenalty_weight+$default_weight) +
	abs($lm_weight+$default_weight) +
	abs($distortion_weight+$default_weight);
   
    # if weights are normalized already, do nothing
    if ($total == 0) {
	$total = 1.0;
	$default_weight = 0.0;
    }
    
    foreach my $phrasemodel_weight (@phrasemodel_weights) {
	$total += abs($phrasemodel_weight + $default_weight);
    }
    foreach my $lexicalreordering_weight (@lexicalreordering_weights) {
	$total += abs($lexicalreordering_weight + $default_weight);
    }
}

sub readOldWriteNewIniFile {
    copy($old_ini_file, $old_ini_file_copy) or die "File $old_ini_file cannot be copied.";

    # read old ini file 
    if (! (open OLDINI, "$old_ini_file_copy" )) {
	die "Warning: unable to read old ini file $old_ini_file_copy\n";
    }

    # Create new ini file
    if (! (open NEWINI, ">$new_ini_file" )) {
	die "Warning: unable to create new ini file: specify name!\n";
    }
    
    while(<OLDINI>) {
	if (/weight-l/) {
	    print NEWINI "[weight-l]\n";
	    print NEWINI ($lm_weight+$default_weight) / $total;
	    print NEWINI "\n";
	    readline(OLDINI);
	} elsif (/weight-t/) {
	    print NEWINI "[weight-t]\n";
	    foreach my $phrasemodel_weight (@phrasemodel_weights) {
		print NEWINI ($phrasemodel_weight+$default_weight) / $total;
                print NEWINI "\n";
		readline(OLDINI);
	    }
	} elsif (/weight-d/) {
	    print NEWINI "[weight-d]\n";
	    print NEWINI ($distortion_weight+$default_weight) / $total;
	    print NEWINI "\n";
	    readline(OLDINI);
	    foreach my $lexicalreordering_weight (@lexicalreordering_weights) {
		print NEWINI ($lexicalreordering_weight+$default_weight) / $total;
		print NEWINI "\n";
		readline(OLDINI);
	    }
	} elsif (/weight-w/) {
	    print NEWINI "[weight-w]\n";
	    print NEWINI ($wordpenalty_weight+$default_weight) / $total;
	    print NEWINI "\n";
	    readline(OLDINI);
	} elsif (/weight-u/) {
	    print NEWINI "[weight-u]\n";
	    print NEWINI ($unknownwordpenalty_weight+$default_weight) / $total;
	    print NEWINI "\n";
	    readline(OLDINI);
	} else {
	    print NEWINI;
	}
    }
    
    close NEWINI;
    close OLDINI;
    @phrasemodel_weights = ();
    @lexicalreordering_weights = ();
}





