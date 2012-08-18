#!/usr/bin/perl -w

use strict;

my $DEBUG = 1;

my $match_file  = "tm/BEST.acquis-xml-escaped.4.uniq";
my $source_file = "data/acquis.truecased.4.en.uniq";
my $target_file = "data/acquis.truecased.4.fr.uniq.most-frequent";
my $alignment_file = "data/acquis.truecased.4.align.uniq.most-frequent";
my $out_file = "data/ac-test.input.xml.4.uniq";
my $in_file = "evaluation/ac-test.input.tc.4";

#my $match_file  = "tm/BEST.acquis-xml-escaped.4";
#my $source_file = "corpus/acquis.truecased.4.en";
#my $target_file = "corpus/acquis.truecased.4.fr";
#my $alignment_file = "model/aligned.4.grow-diag-final-and";
#my $out_file = "data/ac-test.input.xml.4";
#my $in_file = "evaluation/ac-test.input.tc.4";

#my $match_file  = "tm/BEST.acquis.with";
#my $source_file = "../acquis-truecase/corpus/acquis.truecased.190.en";
#my $target_file = "../acquis-truecase/corpus/acquis.truecased.190.fr";
#my $alignment_file = "../acquis-truecase/model/aligned.190.grow-diag-final-and";
#my $out_file = "data/ac-test.input.xml";
#my $in_file = "evaluation/ac-test.input.tc.1";

my @INPUT = `cat $in_file`; chop(@INPUT);
my @SOURCE = `cat $source_file`; chop(@SOURCE);
my @TARGET = `cat $target_file`; chop(@TARGET);
my @ALIGNMENT = `cat $alignment_file`; chop(@ALIGNMENT);

open(MATCH,$match_file);
open(FRAME,">$out_file");
for(my $i=0;$i<4107;$i++) {

    # get match data
    my $match = <MATCH>;
    chop($match);
    my ($score,$sentence,$path) = split(/ \|\|\| /,$match);

    # construct frame
    if ($sentence < 1e9 && $sentence >= 0) {
	my $frame = &create_xml($SOURCE[$sentence],
				$INPUT[$i],
				$TARGET[$sentence],
				$ALIGNMENT[$sentence],
				$path);
	print FRAME $frame."\n";
    }

    # no frame -> output source
    else {
	print FRAME $INPUT[$i]."\n";
    }
}
close(FRAME);
close(MATCH);

sub create_xml {
    my ($source,$input,$target,$alignment,$path) = @_;
    
    my @INPUT = split(/ /,$input);
    my @SOURCE = split(/ /,$source);
    my @TARGET = split(/ /,$target);
    my %ALIGN = &create_alignment($alignment);
    
    my %FRAME_INPUT;
    my @TARGET_BITMAP; 
    foreach (@TARGET) { push @TARGET_BITMAP,1 }
    
    ### STEP 1: FIND MISMATCHES

    my ($s,$i) = (0,0);
    my $currently_matching = 0;
    my ($start_s,$start_i) = (0,0);

    $path .= "X"; # indicate end
    print "$input\n$source\n$target\n$path\n";
    for(my $p=0;$p<length($path);$p++) {
	my $action = substr($path,$p,1);
	
	# beginning of a mismatch
	if ($currently_matching && $action ne "M" && $action ne "X") {
	    $start_i = $i;
	    $start_s = $s;
	    $currently_matching = 0;
	}
	
	# end of a mismatch
	elsif (!$currently_matching && 
	       ($action eq "M" || $action eq "X")) {
	    
	    # remove use of affected target words
	    for(my $ss = $start_s; $ss<$s; $ss++) {
		foreach my $tt (keys %{${$ALIGN{'s'}}[$ss]}) {
		    $TARGET_BITMAP[$tt] = 0;
		}
		
		# also remove enclosed unaligned words?
	    }
	    
	    # are there input words that need to be inserted ?
	    print "($start_i<$i)?\n";
	    if ($start_i<$i) {
		
		# take note of input words to be inserted
		my $insertion = "";
		for(my $ii = $start_i; $ii<$i; $ii++) {
		    $insertion .= $INPUT[$ii]." ";
		}
		
		# find position for inserted input words
		
		# find first removed target word
		my $start_t = 1000;
		for(my $ss = $start_s; $ss<$s; $ss++) {
		    foreach my $tt (keys %{${$ALIGN{'s'}}[$ss]}) {
			$start_t = $tt if $tt < $start_t;
		    }
		}

		# end of sentence? add to end
		if ($start_t == 1000 && $i > $#INPUT) {
		    $start_t = $#TARGET;
		}
		
		# backtrack to previous words if unaligned
		if ($start_t == 1000) {
		    $start_t = -1;
		    for(my $ss = $s-1; $start_t==-1 && $ss>=0; $ss--) {
			foreach my $tt (keys %{${$ALIGN{'s'}}[$ss]}) {
			    $start_t = $tt if $tt > $start_t;
			}
		    }
		}
		$FRAME_INPUT{$start_t} .= $insertion;
	    }
	    
	    $currently_matching = 1;
	}
	
	print "$action $s $i ($start_s $start_i) $currently_matching";
	if ($action ne "I") {
	    print " ->";
	    foreach my $tt (keys %{${$ALIGN{'s'}}[$s]}) {
		print " ".$tt;
	    }
	}
	print "\n";
	$s++ unless $action eq "I";
	$i++ unless $action eq "D";
    }
    

    print $target."\n";
    foreach (@TARGET_BITMAP) { print $_; } print "\n";
    foreach (sort keys %FRAME_INPUT) { 
	print "$_: $FRAME_INPUT{$_}\n";
    }

    ### STEP 2: BUILD FRAME

    # modify frame
    my $frame = "";
    $frame = $FRAME_INPUT{-1} if defined $FRAME_INPUT{-1};
    
    my $currently_included = 0;
    my $start_t = -1;
    push @TARGET_BITMAP,0; # indicate end

    for(my $t=0;$t<=scalar(@TARGET);$t++) {	    
	
	# beginning of tm target inclusion
	if (!$currently_included && $TARGET_BITMAP[$t]) {
	    $start_t = $t;
	    $currently_included = 1;
	}
	
	# end of tm target inclusion (not included word or inserted input)
	elsif ($currently_included && 
	       (!$TARGET_BITMAP[$t] || defined($FRAME_INPUT{$t}))) {
	    # add xml (unless change is at the beginning of the sentence
	    if ($start_t >= 0) {
		my $target = "";
		print "for(tt=$start_t;tt<$t+$TARGET_BITMAP[$t]);\n";
		for(my $tt=$start_t;$tt<$t+$TARGET_BITMAP[$t];$tt++) {
		    $target .= $TARGET[$tt] . " ";
		}
		chop($target);
		$frame .= "<xml translation=\"$target\"> x </xml> ";
	    }
	    $currently_included = 0;
	}
	
	$frame .= $FRAME_INPUT{$t} if defined $FRAME_INPUT{$t};
	print "$TARGET_BITMAP[$t] $t ($start_t) $currently_included\n";
    }

    print $frame."\n-------------------------------------\n";
    return $frame;
}

sub create_alignment {
	my ($line) = @_;
	my (@ALIGNED_TO_S,@ALIGNED_TO_T);
	foreach my $point (split(/ /,$line)) {
		my ($s,$t) = split(/\-/,$point);
		$ALIGNED_TO_S[$s]{$t}++;
		$ALIGNED_TO_T[$t]{$s}++;
	}
	my %ALIGNMENT = ( 's' => \@ALIGNED_TO_S, 't' => \@ALIGNED_TO_T );
	return %ALIGNMENT;
}
