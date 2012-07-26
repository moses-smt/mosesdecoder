#!/usr/bin/perl -w -d 

use strict;
use FindBin qw($RealBin);
use File::Basename;

my $DEBUG = 1;
my $OUTPUT_RULES = 1;

#my $data_root = "/Users/hieuhoang/workspace/experiment/data/tm-mt-integration/";
my $in_file 		= $ARGV[0]; #"$data_root/in/ac-test.input.tc.4";
my $source_file 	= $ARGV[1]; #"$data_root/in/acquis.truecased.4.en.uniq";
my $target_file 	= $ARGV[2]; #"$data_root/in/acquis.truecased.4.fr.uniq";
my $alignment_file	= $ARGV[3]; #"$data_root/in/acquis.truecased.4.align.uniq";
my $lex_file		= $ARGV[4]; #$data_root/in/lex.4;
my $pt_file			= $ARGV[5]; #"$data_root/out/pt";

my $cmd;

my $TMPDIR= "/tmp/tmp.$$";
$cmd = "mkdir -p $TMPDIR";
`$cmd`;
$TMPDIR = "/Users/hieuhoang/workspace/experiment/data/tm-mt-integration/out/tmp.3196";

my $match_file  = "$TMPDIR/match";

# suffix array creation and extraction
$cmd = "$RealBin/fuzzy-match --multiple $in_file  $source_file > $match_file";
`$cmd`;

# make into xml and pt
my $out_file = "$TMPDIR/ac-test.input.xml.4.uniq.multi.tuning";

open(MATCH,$match_file);
open(FRAME,">$out_file");
open(RULE,">$out_file.extract") if $OUTPUT_RULES;
open(RULE_INV,">$out_file.extract.inv") if $OUTPUT_RULES;
open(INFO,">$out_file.info");
while( my $match = <MATCH> ) {
    chop($match);
    my ($score,$sentence,$path) = split(/ \|\|\| /,$match);

    $score =~ /^(\d+) (.+)/ || die;
    my ($i,$match_score) = ($1,$2);

    # construct frame
    if ($sentence < 1e9 && $sentence >= 0) {
		my $SOURCE = $ALL_SOURCE[$sentence];
		my @ALIGNMENT = split(/ \|\|\| /,$ALL_ALIGNMENT[$sentence]);
		my @TARGET = split(/ \|\|\| /,$ALL_TARGET[$sentence]);
		
		for(my $j=0;$j<scalar(@TARGET);$j++) {
			$TARGET[$j] =~ /^(\d+) (.+)$/ || die;
			my ($target_count,$target) = ($1,$2);
			my ($frame,$rule_s,$rule_t,$rule_alignment,$rule_alignment_inv) = 
			&create_xml($SOURCE,
					$INPUT[$i],
					$target,
					$ALIGNMENT[$j],
					$path);
			print FRAME $frame."\n";
			print RULE "$rule_s [X] ||| $rule_t [X] ||| $rule_alignment ||| $target_count\n" if $OUTPUT_RULES;
			print RULE_INV "$rule_t [X] ||| $rule_s [X] ||| $rule_alignment_inv ||| $target_count\n" if $OUTPUT_RULES;
			print INFO "$i ||| $match_score ||| $target_count\n";
		}
    }
}
close(FRAME);
close(MATCH);
close(RULE) if $OUTPUT_RULES;
close(RULE_INV) if $OUTPUT_RULES;

`LC_ALL=C sort $out_file.extract | gzip -c > $out_file.extract.sorted.gz`;
`LC_ALL=C sort $out_file.extract.inv | gzip -c > $out_file.extract.inv.sorted.gz`;

if ($OUTPUT_RULES)
{
  $cmd = "$RealBin/../../scripts/training/train-model.perl -dont-zip -first-step 6 -last-step 6 -f en -e fr -hierarchical -extract-file $out_file.extract -lexical-file $lex_file -phrase-translation-table $pt_file";
  print STDERR "Executing: $cmd \n";
  `$cmd`;
}

#$cmd = "rm -rf $TMPDIR";
#`$cmd`;

#######################################################
sub create_xml {
    my ($source,$input,$target,$alignment,$path) = @_;
    
    my @INPUT = split(/ /,$input);
    my @SOURCE = split(/ /,$source);
    my @TARGET = split(/ /,$target);
    my %ALIGN = &create_alignment($alignment);
    
    my %FRAME_INPUT;
    my (@NT,@INPUT_BITMAP,@TARGET_BITMAP,%ALIGNMENT_I_TO_S);
    foreach (@TARGET) { push @TARGET_BITMAP,1 }
    
    ### STEP 1: FIND MISMATCHES

    my ($s,$i) = (0,0);
    my $currently_matching = 0;
    my ($start_s,$start_i) = (0,0);

    $path .= "X"; # indicate end
    print STDERR "$input\n$source\n$target\n$path\n";
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
	    print STDERR "($start_i<$i)?\n";
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
		my %NT = ("start_t" => $start_t,
			  "start_i" => $start_i );
		push @NT,\%NT;		
	    }	    
	    $currently_matching = 1;
	}
	
	print STDERR "$action $s $i ($start_s $start_i) $currently_matching";
	if ($action ne "I") {
	    print STDERR " ->";
	    foreach my $tt (keys %{${$ALIGN{'s'}}[$s]}) {
		print STDERR " ".$tt;
	    }
	}
	print STDERR "\n";
	$s++ unless $action eq "I";
	$i++ unless $action eq "D";
	$ALIGNMENT_I_TO_S{$i} = $s unless $action eq "D";
	push @INPUT_BITMAP, 1 if $action eq "M";
	push @INPUT_BITMAP, 0 if $action eq "I" || $action eq "S";
    }
    

    print STDERR $target."\n";
    foreach (@TARGET_BITMAP) { print STDERR $_; } print STDERR "\n";
    foreach (sort keys %FRAME_INPUT) { 
	print STDERR "$_: $FRAME_INPUT{$_}\n";
    }

    ### STEP 2: BUILD RULE AND FRAME
        
    # hierarchical rule
    my $rule_s = "";
    my $rule_pos_s = 0;
    my %RULE_ALIGNMENT_S;
    for(my $i=0;$i<scalar(@INPUT_BITMAP);$i++) {
		if ($INPUT_BITMAP[$i]) {
			$rule_s .= $INPUT[$i]." ";
			$RULE_ALIGNMENT_S{$ALIGNMENT_I_TO_S{$i}} = $rule_pos_s++;
		}
		foreach my $NT (@NT) {
			if ($i == $$NT{"start_i"}) {
				$rule_s .= "[X][X] ";
				$$NT{"rule_pos_s"} = $rule_pos_s++;
			}
		}
    }

    my $rule_t = "";
    my $rule_pos_t = 0;
    my %RULE_ALIGNMENT_T;
    for(my $t=-1;$t<scalar(@TARGET_BITMAP);$t++) {
	if ($t>=0 && $TARGET_BITMAP[$t]) {
	    $rule_t .= $TARGET[$t]." ";
	    $RULE_ALIGNMENT_T{$t} = $rule_pos_t++;
	}
	foreach my $NT (@NT) {
	    if ($t == $$NT{"start_t"}) {
		$rule_t .= "[X][X] ";
		$$NT{"rule_pos_t"} = $rule_pos_t++;
	    }
	}
    }

    my $rule_alignment = "";
    foreach my $s (sort { $a <=> $b} keys %RULE_ALIGNMENT_S) {
	foreach my $t (keys %{$ALIGN{"s"}[$s]}) {
	    next unless defined($RULE_ALIGNMENT_T{$t});
	    $rule_alignment .= $RULE_ALIGNMENT_S{$s}."-".$RULE_ALIGNMENT_T{$t}." ";
	}
    }
    foreach my $NT (@NT) {
	$rule_alignment .= $$NT{"rule_pos_s"}."-".$$NT{"rule_pos_t"}." ";
    }
    
    chop($rule_s);
    chop($rule_t);
    chop($rule_alignment);

    my $rule_alignment_inv = "";
    foreach (split(/ /,$rule_alignment)) {
	/^(\d+)\-(\d+)$/;
	$rule_alignment_inv .= "$2-$1 ";
    }
    chop($rule_alignment_inv);

    # frame
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
		print STDERR "for(tt=$start_t;tt<$t+$TARGET_BITMAP[$t]);\n";
		for(my $tt=$start_t;$tt<$t+$TARGET_BITMAP[$t];$tt++) {
		    $target .= $TARGET[$tt] . " ";
		}
		chop($target);
		$frame .= "<xml translation=\"$target\"> x </xml> ";
	    }
	    $currently_included = 0;
	}
	
	$frame .= $FRAME_INPUT{$t} if defined $FRAME_INPUT{$t};
	print STDERR "$TARGET_BITMAP[$t] $t ($start_t) $currently_included\n";
    }

    print STDERR $frame."\n-------------------------------------\n";
    return ($frame,$rule_s,$rule_t,$rule_alignment,$rule_alignment_inv);
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
