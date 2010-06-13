#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";

my $MAX_LENGTH = 4;

my ($system,$segmentation,$reference,$dir,$input,$corpus,$ttable);
if (!&GetOptions('system=s' => \$system, # raw output from decoder
                 'reference=s' => \$reference, # tokenized reference
                 'dir=s' => \$dir, # directory for storing results
                 'input=s' => \$input, # tokenized input (as for decoder)
                 'segmentation=s' => \$segmentation, # system output with segmentation markup
                 'input-corpus=s' => \$corpus, # input side of parallel training corpus
                 'ttable=s' => \$ttable) || # phrase translation table used for decoding
    !defined($dir)) {
	die("ERROR: syntax: analysis.perl -system FILE -reference FILE -dir DIR [-input FILE] [-input-corpus FILE] [-ttable FILE] [-segmentation FILE]");	
}

`mkdir -p $dir`;

# compare system output against reference translation
my(@SYSTEM,@REFERENCE);
my (%PRECISION_CORRECT,%PRECISION_TOTAL,
    %RECALL_CORRECT,%RECALL_TOTAL);
if (defined($system) || defined($reference)) {
  die("you need to you specify both system and reference, not just either")
    unless defined($system) && defined($reference);

  die("can't open system file $system") if ! -e $system;
  @SYSTEM = `cat $system`;
  chop(@SYSTEM);

  if (! -e $reference && -e $reference.".ref0") {
      for(my $i=0;-e $reference.".ref".$i;$i++) {
	  my @REF = `cat $reference.ref$i`;
	  chop(@REF);
	  for(my $j=0;$j<scalar(@REF);$j++) {
	      push @{$REFERENCE[$j]}, $REF[$j];
	  }
      }
  }
  else {
      die("can't open system file $reference") if ! -e $reference;
      @REFERENCE = `cat $reference`;
      chop(@REFERENCE);
  }

  for(my $i=0;$i<scalar @SYSTEM;$i++) {
    &add_match($SYSTEM[$i],$REFERENCE[$i],
	       \%PRECISION_CORRECT,\%PRECISION_TOTAL);
    &add_match($REFERENCE[$i],$SYSTEM[$i],
	       \%RECALL_CORRECT,\%RECALL_TOTAL);
  }

  open(SUMMARY,">$dir/summary");
  &best_matches(\%PRECISION_CORRECT,\%PRECISION_TOTAL,"$dir/n-gram-precision");
  &best_matches(\%RECALL_CORRECT,\%RECALL_TOTAL,"$dir/n-gram-recall");
  &bleu_annotation();
  close(SUMMARY);
}

# segmentation
if (defined($segmentation)) {
  &segmentation();
}

# coverage analysis
my (%INPUT_PHRASE,%CORPUS_COVERED,%TTABLE_COVERED,%TTABLE_ENTROPY);
if (defined($ttable) || defined($corpus)) {	
  if (!defined($input)) {
    die("ERROR: when specifying either ttable or input-corpus, please also specify input\n");
  }
  $MAX_LENGTH = 7;
  &input_phrases();
  &ttable_coverage() if defined($ttable);
  &corpus_coverage() if defined($corpus);
  &input_annotation();
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

sub input_phrases {
  open(INPUT,$input) or die "Can't read input $input";
  while(my $line = <INPUT>) {
    $line =~ s/\|\S+//g;
    &extract_n_grams($line,\%INPUT_PHRASE);
  }
  close(INPUT);  
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
	&extract_n_grams_arrayopt( $REFERENCE[$i], \%REF_NGRAM, "max" );

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

	my $ref_length = 9999;
	if (ref($REFERENCE[$i]) eq 'ARRAY') {
	    foreach my $ref (@{$REFERENCE[$i]}) {
		my @RW = split(/ /,$ref);
		$ref_length = scalar(@RW) if scalar(@RW) < $ref_length;
	    }
	}
	else {
	    my @RW = split(/ /,$REFERENCE[$i]);
	    $ref_length = scalar(@RW);
	}

	if (scalar(@WORD) < $ref_length && scalar(@WORD)>0) {
	    $bleu *= exp(1-$ref_length/scalar(@WORD));
	}

	printf OUT "%5.4f\t%d\t",$bleu,$i;
        for(my $i=0;$i<scalar @WORD;$i++) {
	    print OUT " " if $i;
	    print OUT "$WORD[$i]|$MATCH[$i]";
	}
	if (ref($REFERENCE[$i]) eq 'ARRAY') {
	    foreach my $ref (@{$REFERENCE[$i]}) {
		print OUT "\t".$ref;	
	    }
	}
	else {
	  print OUT "\t".$REFERENCE[$i]  
	}
	print OUT "\n";
    }
    close(OUT);
}

sub add_match {
    my ($system,$reference,$CORRECT,$TOTAL) = @_;
    my (%SYS_NGRAM,%REF_NGRAM);
    &extract_n_grams_arrayopt( $system, \%SYS_NGRAM, "min" );
    &extract_n_grams_arrayopt( $reference, \%REF_NGRAM, "max" );
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

sub ttable_coverage {
  if (! -e $ttable && -e $ttable.".gz") {
    open(TTABLE,"gzip -cd $ttable.gz|");
  }
  else {
    open(TTABLE,$ttable) or die "Can't read ttable $ttable";
  }
  open(REPORT,">$dir/ttable-coverage-by-phrase");
  my ($last_in,$last_size,$size) = ("",0);
  my @DISTRIBUTION = ();
  while(<TTABLE>) {
    chop;
    my ($in,$out,$scores) = split(/ \|\|\| /);	
    my @IN = split(/ /,$in);
    $size = scalar @IN;
    next unless defined($INPUT_PHRASE{$size}{$in});
    $TTABLE_COVERED{$size}{$in}++;
    my @SCORE = split(/ /,$scores);
    if ($in ne $last_in) {
      if ($last_in ne "") {
        my $entropy = &compute_entropy(@DISTRIBUTION);
        printf REPORT "%s\t%d\t%.5f\n",$last_in,$TTABLE_COVERED{$last_size}{$last_in},$entropy;
	$TTABLE_ENTROPY{$last_size}{$last_in} = $entropy;
        @DISTRIBUTION = ();
      }
      $last_in = $in;
      $last_size = $size;
    }
    push @DISTRIBUTION, $SCORE[2]; # forward probability
  }
  my $entropy = &compute_entropy(@DISTRIBUTION);
  print REPORT "%s\t%d\t%.5f\n",$last_in,$TTABLE_COVERED{$last_size}{$last_in},$entropy;
  $TTABLE_ENTROPY{$last_size}{$last_in} = $entropy;
  close(REPORT);
  close(TTABLE);

  &additional_coverage_reports("ttable",\%TTABLE_COVERED);
}

sub compute_entropy {
  my $z = 0; # normalization
  foreach my $p (@_) {
    $z += $p;
  }
  my $entropy = 0;
  foreach my $p (@_) {
    $entropy -= ($p/$z)*log($p/$z)/log(2);
  }
  return $entropy;
}

sub corpus_coverage {
  # compute how often input phrases occur in the corpus
  open(CORPUS,$corpus) or die "Can't read corpus $corpus";
  while(<CORPUS>) {
    s/\|\S+//g;
    my @WORD = split;
    my $sentence_length = scalar @WORD;
    for(my $start=0;$start < $sentence_length;$start++) {
      my $phrase = "";
      for(my $length=1;$length<$MAX_LENGTH && $start+$length<=$sentence_length;$length++) {
	$phrase .= " " if $length > 1;
	$phrase .= $WORD[$start+$length-1];
	last if !defined($INPUT_PHRASE{$length}{$phrase});
	$CORPUS_COVERED{$length}{$phrase}++;
      }
    }
  }
  close(CORPUS);

  # report occurrence counts for all known input phrases
  open(REPORT,">$dir/corpus-coverage-by-phrase");
  foreach my $size (sort {$a <=> $b} keys %INPUT_PHRASE) {
    foreach my $phrase (keys %{$INPUT_PHRASE{$size}}) {
      next unless defined $CORPUS_COVERED{$size}{$phrase};
      printf REPORT "%s\t%d\n", $phrase, $CORPUS_COVERED{$size}{$phrase};
    }
  }
  close(REPORT);

  &additional_coverage_reports("corpus",\%CORPUS_COVERED);
}

sub additional_coverage_reports {
  my ($name,$COVERED) = @_;

  # unknown word report ---- TODO: extend to rare words?
  open(REPORT,">$dir/$name-unknown");
  foreach my $phrase (keys %{$INPUT_PHRASE{1}}) {
    next if defined($$COVERED{1}{$phrase});
    printf REPORT "%s\t%d\n",$phrase,$INPUT_PHRASE{1}{$phrase};
  }
  close(REPORT);

  # summary report
  open(REPORT,">$dir/$name-coverage-summary");
  foreach my $size (sort {$a <=> $b} keys %INPUT_PHRASE) {
    my (%COUNT_TYPE,%COUNT_TOKEN);
    foreach my $phrase (keys %{$INPUT_PHRASE{$size}}) {
      my $covered = $$COVERED{$size}{$phrase};
      $covered = 0 unless defined($covered);
      $COUNT_TYPE{$covered}++;
      $COUNT_TOKEN{$covered} += $INPUT_PHRASE{$size}{$phrase};
    }
    foreach my $count (sort {$a <=> $b} keys %COUNT_TYPE) {
      printf REPORT "%d\t%d\t%d\t%d\n",$size,$count,$COUNT_TYPE{$count},$COUNT_TOKEN{$count};
    }
  }
  close(REPORT);
}

sub input_annotation {
  open(OUT,">$dir/input-annotation");
  open(INPUT,$input) or die "Can't read input $input";
  while(<INPUT>) {
    chop;
    s/\|\S+//g;
    print OUT $_."\t";
    my @WORD = split;
    my $sentence_length = scalar @WORD;
    for(my $start=0;$start < $sentence_length;$start++) {
      my $phrase = "";
      for(my $length=1;$length<$MAX_LENGTH && $start+$length<=$sentence_length;$length++) {
	$phrase .= " " if $length > 1;
	$phrase .= $WORD[$start+$length-1];

	my $ttable_covered = $TTABLE_COVERED{$length}{$phrase};
	my $corpus_covered = $CORPUS_COVERED{$length}{$phrase};
	next unless defined($ttable_covered) || defined($corpus_covered);
	my $ttable_entropy = $TTABLE_ENTROPY{$length}{$phrase} || 0;
	#$ttable_entropy = 0 unless defined($ttable_entropy);
	$ttable_covered = 0 unless defined($ttable_covered);
	$corpus_covered = 0 unless defined($corpus_covered);
	
	if (defined($TTABLE_COVERED{$length}{$phrase})) {
	  printf OUT "%d-%d:%d:%d:%.5f ",$start,$start+$length-1,$corpus_covered,$ttable_covered,$ttable_entropy;
	}
      }
    }
    print OUT "\n";
  }
  close(INPUT);    
  close(OUT);
}

sub extract_n_grams_arrayopt {
    my ($sentence,$NGRAM,$minmax) = @_;
    if (ref($sentence) eq 'ARRAY') {
	my %MINMAX_NGRAM;
	&extract_n_grams($$sentence[0],\%MINMAX_NGRAM);
	for(my $i=1;$i<scalar(@{$sentence});$i++) {
	    my %SET_NGRAM;
	    &extract_n_grams($$sentence[$i],\%SET_NGRAM);
	    for(my $length=1;$length<=$MAX_LENGTH;$length++) {
		if ($minmax eq "min") {
		    foreach my $ngram (keys %{$MINMAX_NGRAM{$length}}) {
			if (!defined($SET_NGRAM{$length}{$ngram})) {
			    delete( $MINMAX_NGRAM{$length}{$ngram} );
			}
			elsif($MINMAX_NGRAM{$length}{$ngram} > $SET_NGRAM{$length}{$ngram}) {
			    $MINMAX_NGRAM{$length}{$ngram} = $SET_NGRAM{$length}{$ngram};
			}
		    }
		}
		else {
		    foreach my $ngram (keys %{$SET_NGRAM{$length}}) {
			if (!defined($MINMAX_NGRAM{$length}{$ngram}) ||
			    $SET_NGRAM{$length}{$ngram} > $MINMAX_NGRAM{$length}{$ngram}) {
			    $MINMAX_NGRAM{$length}{$ngram} = $SET_NGRAM{$length}{$ngram};
			}
		    }
		}
	    }
	}
	for(my $length=1;$length<=$MAX_LENGTH;$length++) {
	    foreach my $ngram (keys %{$MINMAX_NGRAM{$length}}) {
		$$NGRAM{$length}{$ngram} += $MINMAX_NGRAM{$length}{$ngram};
	    }
	}
    }
    else {
	&extract_n_grams($sentence,$NGRAM);
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

sub segmentation {
  my %SEGMENTATION;

  open(FILE,$segmentation) || die("ERROR: could not open segmentation file $segmentation");
  open(OUT,">$dir/segmentation-annotation");
  while(<FILE>) {
    chop;
    my $count=0;
    my $out = -1;
    foreach (split) {
      if (/^\|(\d+)\-(\d+)\|$/) {
	print OUT " " unless $out-($count-1) == 0;
	printf OUT "%d:%d:%d:%d",$1,$2,$out-($count-1),$out;
	my $in_count = $2-$1+1;
	$SEGMENTATION{$in_count}{$count}++;
        $count = 0;
      }
      else {
        $out++;
        $count++;
      }
    }
    print OUT "\n";
  }
  close(OUT);
  close(FILE);

  open(SUMMARY,">$dir/segmentation");
  foreach my $in (sort { $a <=> $b } keys %SEGMENTATION) {
    foreach my $out (sort { $a <=> $b } keys %{$SEGMENTATION{$in}}) {
      printf SUMMARY "%d\t%d\t%d\n", $in, $out, $SEGMENTATION{$in}{$out};
    }
  }
  close(SUMMARY);

  # TODO: error by segmentation
}
