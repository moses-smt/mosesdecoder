#! /usr/bin/perl

#******************************************************************************
# Arianna Bisazza @ FBK-irst. March 2012
#******************************************************************************
# combine-ptables.pl : Combine Moses-style phrase tables, using different approaches


use strict;
use open ':utf8';
binmode STDIN, ':utf8';
binmode STDOUT, ':utf8';

use Getopt::Long "GetOptions";

sub main {
my $usage = "
USAGE
-----
combine-ptables.pl --mode=(interp|union|fillup|backoff|intersect1|stats) ptable1 ptable2 ... ptableN > combined-ptable
combine-ptables.pl --mode=intersect1 reotable-unpruned ptable-pruned > reotable-pruned
-----
#
# This scripts reads two or more *sorted* phrase tables and combines them in different modes.
#
# (Note: if present, word alignments are ignored).
#
# ----------------
# OPTIONS
# ----------------
#
# Required:
# --mode			fillup:	    Each entry is taken only from the first table that contains it.
#		                            A binary feature is added from each table except the first.
# 				backoff:    Each entry is taken only from the first table that contains it.
#		                            NO binary feature is added.
#				interp:     Linear interpolation.
#				union:	    Union of entries, feature vectors are concatenated.
#                               intersect1: Intersection of entries, feature vectors taken from the first table.
#                               stats:      Only compute some statistics about tables overlap. No table is produced.
#
#                               NOTE: if present, additional fields such as word alignment, phrase counts etc. are always
#                                     taken from the first table.
#
# Generic options:
# --phpenalty=FLOAT             Constant value for phrase penalty. Default is exp(1)=2.718
# --phpenalty-at=N              The (N+1)th score of each table is considered as phrase penalty with a constant value.
#                               In 'interp' mode, the corresponding feature is not interpolated but simply set to the constant.
#                               In 'union' mode, the ph.penalty (constant) is output only once, after all the other scores.
#                               By default, no score is considered as phrase penalty.
#
#
# Options for 'fillup':
# --newSourceMaxLength=INT      Don't include \"new\" source phrases if longer than INT words.
#
# Options for 'backoff':
# --newSourceMaxLength=INT      Don't include \"new\" source phrases if longer than INT words.
#
# Options for 'interp':
# --weights=W1,W2,...WN		Weights for interpolation. By default, uniform weights are applied.
# --epsilon=X			Score to assume when a phrase pair is not contained in a table (in 'interp' and 'union' modes).
#                               Default epsilon is 1e-06.
#
# Options for 'union':
#
#
";

my $combination_mode = '';
my $debug = '';
my $weights_str = '';
my $epsilon = 0.000001;
my $phPenalty = 2.718;	# exp(1)
my $phPenalty_idx = -1; 
my $delim= " ||| ";
my $delim_RE= ' \|\|\| ';
my $exp_one = 2.718;
my $exp_zero = 1;
my $newSourceMaxLength = -1;
my $help = '';

GetOptions ('debug' => \$debug, 
	    'mode=s' => \$combination_mode,
	    'weights=s' => \$weights_str,
            'epsilon=f' => \$epsilon,
	    'phpenalty=f' => \$phPenalty,
            'phpenalty-at=i' => \$phPenalty_idx,
	    'newSourceMaxLength=i' => \$newSourceMaxLength,
            'help' => \$help);

if($help) { die "$usage\n\n"; }

if($combination_mode!~/(interp|union|fillup|backoff|intersect1|stats)/) {die "$usage\nUnknown combination mode!\n"}; 

if(@ARGV < 2) {die "$usage\n\n Please provide at least 2 tables to combine \n\n";}

print STDERR "
WARNING: Your phrase tables must be sorted (with LC_ALL=C) !!
******************************
Combination mode is [$combination_mode]
******************************
";

my @tables = @ARGV;
my $nbtables = scalar(@tables);

###########################################

# The newSourceMaxLength option requires reading all the first PT before starting the combination
my %sourcePhrasesPT1; 
if((($combination_mode eq "fillup") || ($combination_mode eq "backoff")) && $newSourceMaxLength>-1) {
    my $table1=$tables[0];
    $table1 =~ s/(.*\.gz)\s*$/gzip -dc < $1|/;
    open(TABLE1, "$table1") or die "Cannot open $table1: ($!)\n";
    while(my $line=<TABLE1>) {
	$line=~m/^(.*?)$delim_RE/;
	$sourcePhrasesPT1{$1}++;
    }
    close(TABLE1);
}

my @table_files=();
foreach my $table (@tables) {
    $table =~ s/(.*\.gz)\s*$/gzip -dc < $1|/;
    #localize the file glob, so FILE is unique to the inner loop.
    local *FILE;
    open(FILE, "$table") or die "Cannot open $table: ($!)\n";
    push(@table_files, *FILE);
}


# Read first line from all tables to find number of weights (and sanity checks)
my @read_ppairs=();
my $nbscores = &read_line_from_tables(\@table_files, \@read_ppairs);
print STDERR "Each phrase table contains $nbscores features.\n";

###########################################

if($phPenalty_idx!=-1) {
    if($phPenalty_idx<0 || $phPenalty_idx>=$nbscores) {
	die "Invalid value for option phpenalty-at! Should be in the range [0,($nbscores-1)]\n\n";
    }
    else { print STDERR "Phrase penalty at index $phPenalty_idx\n"; }
}

#if($weights_str ne "") { die "Weights option NOT supported yet. Can only use uniform (1/nbscores)\n\n"; }
#my $unifw = 1/$nbtables;

my @weights=(); # Array of arrays each containing the feature weights for a phrase table
if($combination_mode eq "interp") {
    my @table_level_weights=();
    if($weights_str eq "") {
	@table_level_weights= ((1/$nbtables) x $nbtables);   # assuming uniform weights
    }
    else {
	@table_level_weights= split(/,/, $weights_str);
	if(scalar(@table_level_weights) != $nbtables) {
	    die "$usage\n Invalid string for option --weights! Must be a comma-separated list of floats, one per ph.table.\n";
	}
    }

    for(my $i=0; $i<$nbtables; $i++) {
	my @weights_pt = (($table_level_weights[$i]) x $nbscores);
	if($phPenalty_idx!=-1) {
	    $weights_pt[$phPenalty_idx]=0;
	}
	print STDERR "WEIGHTS-PT_$i: ", join(" -- ", @weights_pt), "\n";
	$weights[$i] = \@weights_pt;
    }
    print STDERR "EPSILON: $epsilon \n";
}


###########################################

my @empty_ppair=("");
my @epsilons = (($epsilon) x $nbscores);
if($phPenalty_idx>-1) {
    pop @epsilons;
}

my $nbPpairs_inAll=0;
my @nbPairs_found_only_in=((0) x $nbtables);
my $MINSCORE=1;

print STDERR "Working...\n\n";
while(1) {  
    my $min_ppair="";
    my $reached_end_of_tables=1;
    my @tablesContainingPpair=((0) x $nbtables);
    for(my $i=0; $i<$nbtables; $i++) {
	my $ppair=$read_ppairs[$i]->[0];
	if($ppair ne "") {
	    $reached_end_of_tables=0;
	    if($min_ppair eq "" || $ppair lt $min_ppair) {
		$min_ppair=$ppair;
		@tablesContainingPpair=((0) x $nbtables);
		$tablesContainingPpair[$i]=1;
	    }
	    elsif($ppair eq $min_ppair) {
                $tablesContainingPpair[$i]=1;
	    }
	}
    }
    last if($reached_end_of_tables);

    ## Actual combination is performed here:
    &combine_ppair(\@read_ppairs, \@tablesContainingPpair);

    &read_line_from_tables(\@table_files, \@read_ppairs, \@tablesContainingPpair);
    
}

print STDERR "...done!\n";

print STDERR "The minimum score in all tables is $MINSCORE\n";

if($combination_mode eq "stats") {
my $tot_ppairs=0;
print "
# entries
found in all tables:   $nbPpairs_inAll\n";

for(my $i=0; $i<$nbtables; $i++) {
    print "found only in PT_$i:    $nbPairs_found_only_in[$i]\n";
}

}

####################################
sub combine_ppair(PPAIRS_REFARRAY, TABLE_INDICES_REFARRAY) {
    my $ra_ppairs=shift;   # 1st item: phrase-pair key (string); 
                           # 2nd item: ref.array of scores;
                           # 3rd item: additional info (string, may be empty)

    my $ra_toRead=shift;   # Important: this says which phrase tables contain the ph.pair currently processed

    my $ppair="";
    my @scores=();
    my $additional_info="";

    my $to_print=1;

    if($debug) {
	print STDERR "combine_ppair:\n";
	for(my $i=0; $i<$nbtables; $i++) {
	    if($ra_toRead->[$i]) {
		print STDERR "ppair_$i= ", join (" // ", @{$ra_ppairs->[$i]}), "\n";
	    }
	}
    }

    if($combination_mode eq "stats") {
	$to_print=0;
	my $found_in=-1;
	my $nb_found=0;
	for(my $i=0; $i<$nbtables; $i++) {
	    if($ra_toRead->[$i]) {
		$found_in=$i;
		$nb_found++;
	    }
	}
	if($nb_found==1) { $nbPairs_found_only_in[$found_in]++; }
	elsif($nb_found==$nbtables) { $nbPpairs_inAll++; }
    }
    ### Fill-up + additional binary feature
    elsif($combination_mode eq "fillup") {
	my @bin_feats=(($exp_zero) x ($nbtables-1));
	for(my $i=0; $i<$nbtables; $i++) {
	    if($ra_toRead->[$i]) {
		$ppair= shift(@{$ra_ppairs->[$i]});
		# pruning criteria are applied here:
		if($i>0 && $newSourceMaxLength>-1) {
		    $ppair=~m/^(.*?)$delim_RE/;
		    if(scalar(split(/ +/, $1)) > $newSourceMaxLength &&
			!defined($sourcePhrasesPT1{$1})) 
		       { $to_print=0; }
		}
#		@scores= @{$ra_ppairs->[$i]};
		@scores = @{shift(@{$ra_ppairs->[$i]})};
                # binary feature for ph.pair provenance fires here
		if($i>0) { $bin_feats[$i-1]=$exp_one; } 
		$additional_info=shift(@{$ra_ppairs->[$i]});
		last;
	    }
	}
	push(@scores, @bin_feats);
    }
    ### Backoff
    elsif($combination_mode eq "backoff") {
        #my @bin_feats=(($exp_zero) x ($nbtables-1));
        for(my $i=0; $i<$nbtables; $i++) {
            if($ra_toRead->[$i]) {
                $ppair= shift(@{$ra_ppairs->[$i]});
                # pruning criteria are applied here:
                if($i>0 && $newSourceMaxLength>-1) {
                    $ppair=~m/^(.*?)$delim_RE/;
                    if(scalar(split(/ +/, $1)) > $newSourceMaxLength &&
                        !defined($sourcePhrasesPT1{$1}))
                       { $to_print=0; }
                }
		@scores = @{shift(@{$ra_ppairs->[$i]})};
 		$additional_info=shift(@{$ra_ppairs->[$i]});
                last;
            }
	}
    }
    ### Linear interpolation
    elsif($combination_mode eq "interp") {
	my $firstPpair=-1;
	@scores=((0) x $nbscores);
	for(my $i=0; $i<$nbtables; $i++) {
	    if($ra_toRead->[$i]) {
		if($firstPpair==-1) { $firstPpair=$i; }
		$ppair= shift(@{$ra_ppairs->[$i]});
		my @scoresPT = @{shift(@{$ra_ppairs->[$i]})};
		for(my $j=0; $j<$nbscores; $j++) {
#		    $scores[$j]+= $weights[$i]->[$j]* $ra_ppairs->[$i][$j];
		    $scores[$j]+= $weights[$i]->[$j]* $scoresPT[$j];
		}
	    }
	    else {
		for(my $j=0; $j<$nbscores; $j++) {
                    $scores[$j]+= $weights[$i]->[$j]* $epsilon;
                }
	    }
	    if($phPenalty_idx!=-1) {
		$scores[$phPenalty_idx]= $phPenalty;
	    }
	}
	if($debug) { print STDERR "..taking info from ptable_$firstPpair\n"; }
	$additional_info= shift(@{$ra_ppairs->[$firstPpair]});
    }
    ### Union + feature concatenation
    elsif($combination_mode eq "union") {
        my $firstPpair=-1;
	for(my $i=0; $i<$nbtables; $i++) {
	    if($ra_toRead->[$i]) { 
		if($firstPpair==-1) { $firstPpair=$i; }
		$ppair= shift(@{$ra_ppairs->[$i]});
		my @scoresPT= @{shift(@{$ra_ppairs->[$i]})};
		if($phPenalty_idx!=-1) {
#		    splice(@{$ra_ppairs->[$i]}, $phPenalty_idx, 1);
		    splice(@scoresPT, $phPenalty_idx, 1);
		}
#		push(@scores, @{$ra_ppairs->[$i]});
	        push(@scores, @scoresPT);
	    } 
	    else { 
		push(@scores, @epsilons);
	    }
	}
	if($phPenalty_idx!=-1) { 
	    push(@scores, $phPenalty);
	}
	if($debug) { print STDERR "..taking info from ptable_$firstPpair\n"; }
        $additional_info= shift(@{$ra_ppairs->[$firstPpair]});
    }
    ### Intersect + features from first table
    elsif($combination_mode eq "intersect1") {
        $to_print=0;
        my $found_in_all=1;
        for(my $i=0; $i<$nbtables; $i++) {
            if(!$ra_toRead->[$i]) {
		$found_in_all=0;
		last;
            }
        }
        if($found_in_all) { 
	    $to_print=1;
	    $ppair= shift(@{$ra_ppairs->[0]});
#	    @scores= @{$ra_ppairs->[0]};
	    @scores= @{shift(@{$ra_ppairs->[0]})};
	    $additional_info= shift(@{$ra_ppairs->[0]});
	}
    }
    else {
	die "$usage\nUnknown combination mode!\n";
    }


    if($to_print) {
	if($additional_info eq "") {
	    print $ppair, join(" ", @scores), "\n";
	}else {
	    print $ppair, join(" ", @scores), $delim, $additional_info, "\n";
	}
    }
}

####################################
# Read lines from all filehandles given in FILES_REFARRAY, 
# or from the files whose indices are assigned 1 in the array TABLE_INDICES_REFARRAY
# Parse each of them as a phrase pair entry and stores it to the corresponding position of PPAIRS_REFARRAY
sub read_line_from_tables(FILES_REFARRAY, PPAIRS_REFARRAY, TABLE_INDICES_REFARRAY) {
    my $ra_files=shift;
    my $ra_ppairs=shift;

    my $ra_toRead=shift;
    my @toRead=((1) x $nbtables);   # by default read from all files
    if($ra_toRead ne "") { 
	@toRead=@$ra_toRead;
    }

    my $nbscores=-1;
    my $key=""; my $additional_info="";
    for(my $i=0; $i<$nbtables; $i++) {
	next if($toRead[$i]==0);
	my @ppair=();
	my $file=$ra_files->[$i];
	if(my $line = <$file>) { 
	    chomp $line;
	    my @fields = split(/$delim_RE/, $line);
	    if(scalar(@fields)<3) {
                die "Invalid phrase table entry:\n$line\n";
	    }
	    my @scores = split(/\s+/, $fields[2]);
	    foreach my $score (@scores) {
		if($score<$MINSCORE) { $MINSCORE=$score; }
	    }
	    # Get nb of scores from the 1st table. Check that all tables provide the same nb of scores,
	    # unless mode is 'intersect' (then it doesn't matter as scores are taken only from 1st table)
	    if($nbscores==-1) {
		$nbscores=scalar(@scores);
	    } elsif($nbscores!=scalar(@scores) && $combination_mode ne "intersect1") {
		die "Wrong number of scores in table-$i! Should be $nbscores\n";
	    }
	    # Get additional fields if any (word aligment, phrase counts etc.)
	    if(scalar(@fields)>3) {
		$additional_info=join($delim, splice(@fields,3));
		#print STDOUT "additional_info:__{$additional_info}__\n";
	    }
	    my $key = "$fields[0]$delim$fields[1]$delim";  ## IMPORTANT: the | delimiter at the end of the phrase pair is crucial to preserve sorting!!
	    push(@ppair, $key, \@scores, $additional_info);
	}
	else { 
	    push(@ppair, "");
	}
	$ra_ppairs->[$i]=\@ppair;
    }

    return $nbscores;
}

#########
}


&main;
