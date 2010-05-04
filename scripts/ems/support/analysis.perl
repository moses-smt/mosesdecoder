#!/usr/bin/perl -w

use strict;

my $MAX_LENGTH = 4;

die("ERROR: syntax: analysis.perl system reference directory") 
    unless scalar @ARGV == 3;
my ($system,$reference,$dir) = @ARGV;

`mkdir -p $dir`;

my @SYSTEM = `cat $system`; chop(@SYSTEM);
my @REFERENCE = `cat $reference`; chop(@REFERENCE);

my (%PRECISION_CORRECT,%PRECISION_TOTAL,
    %RECALL_CORRECT,%RECALL_TOTAL);
open(SUMMARY,">$dir/summary");
&create_n_gram_stats();
&best_matches(\%PRECISION_CORRECT,\%PRECISION_TOTAL,"$dir/n-gram-precision");
&best_matches(\%RECALL_CORRECT,\%RECALL_TOTAL,"$dir/n-gram-recall");
&bleu_annotation();
close(SUMMARY);

sub create_n_gram_stats {
    for(my $i=0;$i<scalar @SYSTEM;$i++) {
	&add_match($SYSTEM[$i],$REFERENCE[$i],
		   \%PRECISION_CORRECT,\%PRECISION_TOTAL);
	&add_match($REFERENCE[$i],$SYSTEM[$i],
		   \%RECALL_CORRECT,\%RECALL_TOTAL);
    }
}

sub best_matches {
    my ($CORRECT,$TOTAL,$out) = @_;
    my $type = ($out =~ /precision/) ? "precision" : "recall";
    for(my $length=1;$length<=$MAX_LENGTH;$length++) {
	my ($total,$correct) = (0,0);
	open(OUT,">$out.$length");
	foreach my $ngram (keys %{$$TOTAL{$length}}) {
	    printf OUT "%d\t%d\t%s\n",
	    $$TOTAL{$length}{$ngram},
	    $$CORRECT{$length}{$ngram},
	    $ngram;
	    $total += $$TOTAL{$length}{$ngram};
	    $correct += $$CORRECT{$length}{$ngram};
	}
	close(OUT);
	print SUMMARY "$type-$length-total: $total\n";
	print SUMMARY "$type-$length-correct: $correct\n";
    }
}

sub bleu_annotation {
    open(OUT,"| sort -r >$dir/bleu-annotation");
    for(my $i=0;$i<scalar @SYSTEM;$i++) {
	my $system = $SYSTEM[$i];
	$system =~ s/\s+/ /g;
	$system =~ s/^ //;
	$system =~ s/ $//;
	my (%SYS_NGRAM,%REF_NGRAM);
	&extract_n_grams( $system, \%SYS_NGRAM );
	&extract_n_grams( $REFERENCE[$i], \%REF_NGRAM );

 	my @WORD = split(/ /,$system);
	my @MATCH;
	for(my $i=0;$i<scalar @WORD;$i++) {
	    $MATCH[$i] = 0;
	}

	my $bleu = 1;
	for(my $length=1;$length<=$MAX_LENGTH && $length <= scalar @WORD;$length++) {
	    my $ngram_correct = 1;
	    for(my $i=0;$i<=scalar @WORD-$length;$i++) {
		my $ngram = "";
		for(my $n=0;$n<$length;$n++) {
		    $ngram .= " " if $n>0;
		    $ngram .= $WORD[$i+$n];
		}
		$REF_NGRAM{$length}{$ngram}--;
		if ($REF_NGRAM{$length}{$ngram} >= 0) {
		    $ngram_correct++;
		    for(my $n=0;$n<$length;$n++) {
			$MATCH[$i+$n] = $length;
		    }
		}
	    }
	    $bleu *= ($ngram_correct/(scalar(@WORD)-$length+2));
	}
	$bleu = $bleu ** (1/4);
	my @RW = split(/ /,$REFERENCE[$i]);
	my $ref_length = scalar(@RW);
	if (scalar(@WORD) < $ref_length) {
	    $bleu *= exp(1-$ref_length/scalar(@WORD));
	}

	my @COLOR = ("#FFC0C0","#FFC0FF","#C0C0FF","#C0FFFF","#C0FFC0");
	printf OUT "%5.4f\t%d\t",$bleu,$i;
        for(my $i=0;$i<scalar @WORD;$i++) {
	    print OUT " " if $i;
	    print OUT "$WORD[$i]|$MATCH[$i]";
	}
	print OUT "\t".$REFERENCE[$i]."\n";
    }
    close(OUT);
}

sub add_match {
    my ($system,$reference,$CORRECT,$TOTAL) = @_;
    my (%SYS_NGRAM,%REF_NGRAM);
    &extract_n_grams( $system, \%SYS_NGRAM );
    &extract_n_grams( $reference, \%REF_NGRAM );
    foreach my $length (keys %SYS_NGRAM) {
	foreach my $ngram (keys %{$SYS_NGRAM{$length}}) {
	    my $sys_count = $SYS_NGRAM{$length}{$ngram};
	    my $ref_count = 0;
	    $ref_count = $REF_NGRAM{$length}{$ngram} if defined($REF_NGRAM{$length}{$ngram});
	    my $match_count = ($sys_count > $ref_count) ? $ref_count : $sys_count;
	    
	    $$CORRECT{$length}{$ngram} += $match_count;
	    $$TOTAL{$length}{$ngram} += $sys_count;
	    #print "$length:$ngram $sys_count $ref_count\n";
	}
    }
}

sub extract_n_grams {
    my ($sentence,$NGRAM) = @_;
    $sentence =~ s/\s+/ /g;
    $sentence =~ s/^ //;
    $sentence =~ s/ $//;
    
    my @WORD = split(/ /,$sentence);
    for(my $length=1;$length<=$MAX_LENGTH;$length++) {
	for(my $i=0;$i<=scalar(@WORD)-$length;$i++) {
	    my $ngram = "";
	    for(my $n=0;$n<$length;$n++) {
		$ngram .= " " if $n>0;
		$ngram .= $WORD[$i+$n];
	    }
	    $$NGRAM{$length}{$ngram}++;
	}
    }
}
