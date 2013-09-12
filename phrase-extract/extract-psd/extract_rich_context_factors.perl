#!/usr/bin/perl -w

use strict;

my $en_file_parsed = shift;

open(ENparsed, $en_file_parsed) or die "cannot open EN parsed: $en_file_parsed\n";

#my $en_parsed_in = <ENparsed>;

my $line_counter = 1;
$/ = "";
while(<ENparsed>) {
    chomp;
    s/^\n//;
    my @en_parsed = split(/\n/, $_);

    #print join("\n", @en_parsed)."\n\n";

    my $len = @en_parsed;
    my (@dependencyfeatures);
    @dependencyfeatures = &get_dependency_features(\@en_parsed);
    print join(" ", @dependencyfeatures)."\n";
    
#    for (my $i=0; $i<$len; $i++) {
#	print "$dependencyfeatures[$i]\n";
#    }
#    print "\n\n";

    $line_counter ++;
}

#######################################################################################################

# PARSED DATA

#info[0]: position (starting from 1)
#info[1]: word
#info[2]: lemma
#info[3]: POS
#info[4]: --
#info[5]: dependency position
#info[6]: dependency name
#info[7]: semantic label

sub get_dependency_features {
    my @parsed = @{$_[0]};

    ## add dummy line at position zero in array - numbering of the parse starts at 1
    unshift(@parsed, "0\tX\tX\tX\tX\tX\tX\tX");
    my $len = @parsed;

    my %has_leftmostchild;
    my %has_child;
    my %has_parent;
    my %has_siblings;

    my @word;
    my @lemma;
    my @pos;
    my @rel;

    my (@collect_features);

#    print join("\n", @parsed)."\n\n";

    for (my $i=1; $i<$len; $i++) {
	my @info = split(/\t/, $parsed[$i]);
	$has_parent{$i} = $info[5];     
	
	push(@word, $info[1]);
	push(@lemma, $info[2]);
	push(@pos, $info[3]);
	push(@rel, $info[6]);
    }
    
    foreach my $child (keys %has_parent) {
	my $parent = $has_parent{$child};

	push(@{$has_child{$parent}}, $child);  ## collect positions of *all* children
	
	if (exists $has_leftmostchild{$parent}) {
	    if ($has_leftmostchild{$parent} > $child) {
		$has_leftmostchild{$parent} = $child;
	    }
	}
	else {
	    $has_leftmostchild{$parent} = $child;
	}	
    }
    
    for (my $i=1; $i<$len; $i++) {
	
	## find FATHER
	unless (exists $has_parent{$i}) {die "No parent for word $i in sentence $line_counter\n"; }
	my $parent_ind = $has_parent{$i};
	my $parent_word = "undef";
	my $parent_pos = "undef";
	my $parent_lem = "undef";

	if ($parent_ind eq "0") { 
	    $parent_word = "root"; 
	    $parent_lem = "root";
	    $parent_pos = "root";
	}
	else { 
	    $parent_word = $word[$parent_ind-1]; 
	    $parent_pos = $pos[$parent_ind-1];
	    $parent_lem = $lemma[$parent_ind-1];
	}

	## find GRANDFATHER
	my $grandparent_word = "undef";
	my $grandparent_lem = "undef";
	my $grandparent_pos = "undef"; 
	my $grandparent_ind = "0";

	unless ($parent_ind == 0) {
	    $grandparent_ind = $has_parent{$parent_ind};
	}
	if ($grandparent_ind eq "0") { 
	    $grandparent_word = "root"; 
	    $grandparent_lem = "root";
	    $grandparent_pos = "root";
	}
	else { 
	    $grandparent_word = $word[$grandparent_ind-1]; 
	    $grandparent_lem = $lemma[$grandparent_ind-1];
	    $grandparent_pos = $pos[$grandparent_ind-1];
	}
	
	## find SIBLINGS
	my $left_sister_word = "noLeftSister";
	my $left_sister_lem = "noLeftSister";
	my $left_sister_pos = "noLeftSister";
	my $right_sister_word = "noRightSister";
	my $right_sister_pos = "noRightSister";
	my $right_sister_lem = "noRightSister";
	my $left_sister_ind = "undef";
	my $right_sister_ind = "undef";

	my @set = @{$has_child{$parent_ind}};
	@set = sort {$a <=> $b} @set;
	my $len_set = @set;

	#print"I:$i   --->   ".join(" ", @set)."\n";
	
	for (my $j=0; $j<$len_set; $j++) {
	    if ($set[$j] == $i) {
		if (exists $set[$j-1] && $j>0 ) {
		    $left_sister_ind = $set[$j-1] ;
		    $left_sister_word = $word[$left_sister_ind-1];
		    $left_sister_lem = $lemma[$left_sister_ind-1];
		    $left_sister_pos = $pos[$left_sister_ind-1];
		}		
		if (exists $set[$j+1]) {
		    $right_sister_ind = $set[$j+1];
		    $right_sister_word = $word[$right_sister_ind-1];
		    $right_sister_pos = $pos[$right_sister_ind-1];
		    $right_sister_lem = $lemma[$right_sister_ind-1];
		}
	    }
	}

	## find LEFTMOST CHILD
	my $leftmostchild_word = "noLmChild";
	my $leftmostchild_pos = "noLmChild";
	my $leftmostchild_lem = "noLmChild";
	my $leftmostchild_rel = "noLmChild";
	my $leftmostchild_ind = "undef";
	
	if (exists $has_leftmostchild{$i}) {
	    $leftmostchild_ind = $has_leftmostchild{$i};
	    $leftmostchild_word = $word[$leftmostchild_ind-1];
	    $leftmostchild_lem = $lemma[$leftmostchild_ind-1];
	    $leftmostchild_pos = $pos[$leftmostchild_ind-1];
	    $leftmostchild_rel = $rel[$leftmostchild_ind-1];
	}

	my $treefeatures = "$parent_lem|$parent_pos|$grandparent_lem|$grandparent_pos|$leftmostchild_lem|$leftmostchild_pos|$leftmostchild_rel|$left_sister_lem|$left_sister_pos|$right_sister_lem|$right_sister_pos";

	## SIMPLE LOCAL FEATURES + MORPH FEATU:
	my $current_word = $word[$i-1];
	my $current_tag = $pos[$i-1];
	my $current_lem = $lemma[$i-1];
	my $current_rel = $rel[$i-1];

	## get NUMBER information for NOUNS
	my $en_tag = $current_tag;
	my $number = "noNum";
	if ($current_tag =~ /^(NNS|NNPS)$/) {
	    $number = "pl";
	    $en_tag =~ s/P?S$//;
	}
	elsif ($current_tag =~ /^(NN|NNP)$/) {
	    $number = "sg";
	    $en_tag =~ s/P$//;
	}

	if ($en_tag =~ /^PRP$/) {
	    if ($current_lem =~ /^(they|we)$/) {
		$number = "pl";
	    }
	    elsif ($current_lem =~ /^(i|you|he|she|it|one)$/){
		$number = "sg";
	    }
	}

	my $localfeatures = "$current_word|$current_lem|$current_tag|$en_tag|$current_rel|$number|";


	push(@collect_features, $localfeatures.$treefeatures);
    }
    return @collect_features;
}
