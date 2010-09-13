#!/usr/bin/perl -w 

use strict;
use Getopt::Long "GetOptions";

my ($CORPUS,$MODEL,$TRAIN,$HELP,$VERBOSE);
my $FILLER = ":s:es";
my $MIN_SIZE = 3;
my $MIN_COUNT = 5;
my $MAX_COUNT = 5;
$HELP = 1
    unless &GetOptions('corpus=s' => \$CORPUS,
		       'model=s' => \$MODEL,
		       'filler=s' => \$FILLER,
		       'min-size=i' => \$MIN_SIZE,
		       'min-count=i' => \$MIN_COUNT,
		       'max-count=i' => \$MAX_COUNT,
		       'help' => \$HELP,
		       'verbose' => \$VERBOSE,
		       'train' => \$TRAIN);

if ($HELP ||
    ( $TRAIN && !$CORPUS) ||
    (!$TRAIN && !$MODEL)) {
    print "Compound splitter\n";
    print "-----------------\n\n";
    print "train:   compound-splitter -train -corpus txt-file -model new-model\n";
    print "apply:   compound-splitter -model trained-model < in > out\n";
    print "options: -min-size: minimum word size (default $MIN_SIZE)\n";
    print "         -min-count: minimum word count (default $MIN_COUNT)\n";
    print "         -filler: filler letters between words (default $FILLER)\n";
    exit;
}

if ($TRAIN) {
    &train;
}
else {
    &apply;
}

sub train {
    my %WORD;
    open(CORPUS,$CORPUS) || die("ERROR: could not open corpus '$CORPUS'");
    while(<CORPUS>) {
	chop; s/\s+/ /g; s/^ //; s/ $//;
	foreach (split) {
	    $WORD{$_}++;
	}
    }
    close($CORPUS);
    my $id = 0;
    open(MODEL,">".$MODEL);
    foreach my $word (keys %WORD) {
	print MODEL "".(++$id)."\t".$word."\t".$WORD{$word}."\n";
    }
    close(MODEL);
    print STDERR "written model file with ".(scalar keys %WORD)." words.\n";
}

sub apply {
    my (%WORD,%TRUECASE);
    open(MODEL,$MODEL) || die("ERROR: could not open model '$MODEL'");
    while(<MODEL>) {
	chomp;
	my ($id,$word,$count) = split(/\t/);
        my $lc = lc($word);
	# if word exists with multipe casings, only record most frequent
        next if defined($WORD{$lc}) && $WORD{$lc} > $count;
	$WORD{$lc} = $count;
	$TRUECASE{$lc} = $word;
    }
    close(MODEL);

    while(<STDIN>) {
	my $first = 1;
	chop; s/\s+/ /g; s/^ //; s/ $//;
	foreach my $word (split) {
	    print " " unless $first;	    
	    $first = 0;

	    # don't split frequent words
	    if (defined($WORD{$word}) && $WORD{$word}>=$MAX_COUNT) {
		print $word;
		next;
	    }

	    # consider possible splits
	    my $final = length($word)-1;
	    my %REACHABLE;
	    for(my $i=0;$i<=$final;$i++) { $REACHABLE{$i} = (); }
	    
	    print STDERR "splitting $word:\n" if $VERBOSE;
	    for(my $end=$MIN_SIZE;$end<length($word);$end++) {
		for(my $start=0;$start<=$end-$MIN_SIZE;$start++) {
		    next unless $start == 0 || defined($REACHABLE{$start-1});
		    foreach my $filler (split(/:/,$FILLER)) {
			next if $start == 0 && $filler ne "";
			next if lc(substr($word,$start,length($filler))) ne $filler;
			my $subword = lc(substr($word,
					        $start+length($filler),
					        $end-$start+1-length($filler)));
			next unless defined($WORD{$subword});			
			next unless $WORD{$subword} >= $MIN_COUNT;
			print STDERR "\tmatching word $start .. $end ($filler)$subword $WORD{$subword}\n" if $VERBOSE;
			push @{$REACHABLE{$end}},"$start $TRUECASE{$subword} $WORD{$subword}";	
		    }
		}
	    }

	    # no matches at all?
	    if (!defined($REACHABLE{$final})) {
		print $word;
		next;
	    }

	    my ($best_split,$best_score) = ("",0);

	    my %ITERATOR;
	    for(my $i=0;$i<=$final;$i++) { $ITERATOR{$i}=0; }
	    my $done = 0;
	    while(1) {
		# read off word
		my ($pos,$decomp,$score,$num,@INDEX) = ($final,"",1,0);
		while($pos>0) {
		    last unless scalar @{$REACHABLE{$pos}} > $ITERATOR{$pos}; # dead end?
		    my ($nextpos,$subword,$count) 
			= split(/ /,$REACHABLE{$pos}[ $ITERATOR{$pos} ]);
		    $decomp = $subword." ".$decomp;
		    $score *= $count;
		    $num++;
		    push @INDEX,$pos;
#		    print STDERR "($nextpos-$pos,$decomp,$score,$num)\n";
		    $pos = $nextpos-1;
		}

		chop($decomp);
		print STDERR "\tsplit: $decomp ($score ** 1/$num) = ".($score ** (1/$num))."\n" if $VERBOSE;
		$score **= 1/$num;
		if ($score>$best_score) { 
		    $best_score = $score;
		    $best_split = $decomp;
		}

		# increase iterator
		my $increase = -1;
		while($increase<$final) {
		    $increase = pop @INDEX;
		    $ITERATOR{$increase}++;
		    last if scalar @{$REACHABLE{$increase}} > $ITERATOR{$increase};
		}
		last unless scalar @{$REACHABLE{$final}} > $ITERATOR{$final};
		for(my $i=0;$i<$increase;$i++) { $ITERATOR{$i}=0; }		    
	    }
	    $best_split = $word unless $best_split =~ / /; # do not change case for unsplit words
	    print $best_split;
	}
	print "\n";
    }
}
