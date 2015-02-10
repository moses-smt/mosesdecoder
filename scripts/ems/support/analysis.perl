#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";

my $MAX_LENGTH = 4;

my ($system,$system_alignment,$segmentation,$reference,$dir,$input,$corpus,$ttable,@FACTORED_TTABLE,$score_options,$hierarchical,$output_corpus,$alignment,$biconcor,$input_factors,$input_factor_names,$output_factor_names,$precision_by_coverage,$precision_by_coverage_factor,$coverage_dir,$search_graph);
if (!&GetOptions('system=s' => \$system, # raw output from decoder
		 'system-alignment=s' => \$system_alignment, # word alignment of system output
                 'reference=s' => \$reference, # tokenized reference
                 'dir=s' => \$dir, # directory for storing results
		 'input-factors=i' => \$input_factors, # list of input factors
		 'input-factor-names=s' => \$input_factor_names,
		 'output-factor-names=s' => \$output_factor_names,
		 'precision-by-coverage' => \$precision_by_coverage, # added report for input words
		 'precision-by-coverage-factor=i' => \$precision_by_coverage_factor, # sub-reports
                 'input=s' => \$input, # tokenized input (as for decoder)
                 'segmentation=s' => \$segmentation, # system output with segmentation markup
                 'input-corpus=s' => \$corpus, # input side of parallel training corpus
                 'ttable=s' => \$ttable, # phrase translation table used for decoding
		 'factored-ttable=s' => \@FACTORED_TTABLE, # factored phrase translation table
		 'score-options=s' => \$score_options, # score options to detect p(e|f) score
                 'output-corpus=s' => \$output_corpus, # output side of parallel training corpus
                 'alignment-file=s' => \$alignment, # alignment of parallel corpus
		 'coverage=s' => \$coverage_dir, # already computed coverage, stored in this dir
                 'biconcor=s' => \$biconcor, # binary for bilingual concordancer
                 'search-graph=s' => \$search_graph, # visualization of search graph
		 'hierarchical' => \$hierarchical) || # hierarchical model?
    !defined($dir)) {
	die("ERROR: syntax: analysis.perl -system FILE -reference FILE -dir DIR [-input FILE] [-input-corpus FILE] [-ttable FILE] [-score-options SETTINGS] [-segmentation FILE] [-output-corpus FILE] [-alignment-file FILE] [-biconcor BIN]");	
}

`mkdir -p $dir`;

# factor names
if (defined($input_factor_names) && defined($output_factor_names)) {
  open(FACTOR,">$dir/factor-names") or die "Cannot open: $!";
  print FACTOR $input_factor_names."\n";
  print FACTOR $output_factor_names."\n";
  close(FACTOR);
}

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

  open(SUMMARY,">$dir/summary") or die "Cannot open: $!";
  &best_matches(\%PRECISION_CORRECT,\%PRECISION_TOTAL,"$dir/n-gram-precision");
  &best_matches(\%RECALL_CORRECT,\%RECALL_TOTAL,"$dir/n-gram-recall");
  &bleu_annotation();
  close(SUMMARY);
}

# segmentation
if (defined($segmentation)) {
    if (defined($hierarchical)) {
	&hierarchical_segmentation();
    }
    else {
	&segmentation();
    }
}

# coverage analysis
my (%INPUT_PHRASE,%CORPUS_COVERED,%TTABLE_COVERED,%TTABLE_ENTROPY);
if (!defined($coverage_dir) && (defined($ttable) || defined($corpus))) {	
  if (!defined($input)) {
    die("ERROR: when specifying either ttable or input-corpus, please also specify input\n");
  }
  $MAX_LENGTH = 7;
  &input_phrases();
  &ttable_coverage("0",$ttable) if defined($ttable);
  &corpus_coverage() if defined($corpus);
  &input_annotation();

  # corpus coverage for non-surface factors
  if (defined($input_factors)) {
    for(my $factor=1;$factor<$input_factors;$factor++) {
      &input_phrases($factor);
      &corpus_coverage($factor);
    }
  }

  # factored ttable coverage
  foreach my $factored_ttable (@FACTORED_TTABLE) {
      die("factored ttable must be specified as factor:file -- $ttable")
	unless $factored_ttable =~ /^([\d,]+)\:(.+)/; #  factor:ttable
      my ($factor,$file) = ($1,$2);
      next if defined($ttable) && $file eq $ttable; # no need to do this twice
      &input_phrases($factor);
      &ttable_coverage($factor,$file);
  }
}

if (defined($precision_by_coverage)) {
  &precision_by_coverage("ttable");
  &precision_by_coverage("corpus");
}

# bilingual concordance -- not used by experiment.perl
if (defined($corpus) && defined($output_corpus) && defined($alignment) && defined($biconcor)) {
  `$biconcor -s $dir/biconcor -c $corpus -t $output_corpus -a $alignment`;
}

# process search graph for visualization
if (defined($search_graph)) {
  &process_search_graph($search_graph);
}

sub best_matches {
    my ($CORRECT,$TOTAL,$out) = @_;
    my $type = ($out =~ /precision/) ? "precision" : "recall";
    for(my $length=1;$length<=$MAX_LENGTH;$length++) {
	my ($total,$correct) = (0,0);
	open(OUT,">$out.$length") or die "Cannot open: $!";
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

# get all the n-grams from the input corpus
sub input_phrases {
  my ($factor) = (@_);
  %INPUT_PHRASE = ();

  open(INPUT,$input) or die "Can't read input $input";
  while(my $line = <INPUT>) {
    chop($line);
    $line = &get_factor_phrase($factor,$line);
    &extract_n_grams($line,\%INPUT_PHRASE);
  }
  close(INPUT);  
}

# reduce a factorized phrase into the factors of interest
sub get_factor_phrase {
  my ($factor,$line) = @_;

  # clean line
  $line =~ s/[\r\n]+//g;
  $line =~ s/\s+/ /;
  $line =~ s/^ //;
  $line =~ s/ $//;

  # only surface? delete remaining factors
  if (!defined($factor) || $factor eq "0") {
    $line =~ s/\|\S+//g;
    return $line;
  }
  my $factored_line = "";

  # reduce each word
  foreach (split(/ /,$line)) {
    $factored_line .= &get_factor_word($factor,$_) . " ";
  }

  chop($factored_line);
  return $factored_line;
}

# reduce a factorized word into the factors of interest
sub get_factor_word {
  my ($factor,$word) = @_;

  my @WORD = split(/\|/,$word);
  my $fword = "";
  foreach (split(/,/,$factor)) {
    $fword .= $WORD[$_]."|";
  }
  chop($fword);
  return $fword;
}

sub factor_ext {
  my ($factor) = @_;
  return "" if !defined($factor) || $factor eq "0";
  return ".".$factor;
}

sub bleu_annotation {
    open(OUT,"| sort -r >$dir/bleu-annotation") or die "Cannot open: $!";
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
  my ($factor,$ttable) = @_;

  # open file
  if (! -e $ttable && -e $ttable.".gz") {
    open(TTABLE,"gzip -cd $ttable.gz|") or die "Cannot open: $!";
  }
  elsif ($ttable =~ /.gz$/) {
    open(TTABLE,"gzip -cd $ttable|") or die "Cannot open: $!";
  }
  else {
    open(TTABLE,$ttable) or die "Can't read ttable $ttable: $!";
  }

  # create report file
  open(REPORT,">$dir/ttable-coverage-by-phrase".&factor_ext($factor)) or die "Cannot open: $!";
  my ($last_in,$last_size,$size) = ("",0);

  my $p_e_given_f_score = 2;
  if ($score_options) {
    if ($score_options =~ /OnlyDirect/) {
      $p_e_given_f_score = 0;
    }
    elsif ($score_options =~ /NoLex/) {
      $p_e_given_f_score = 1;
    }
  }

  my @DISTRIBUTION = ();
  while(<TTABLE>) {
    chop;
    my @COLUMN = split(/ +\|\|\| +/);
    my ($in,$out,$scores) = @COLUMN;
    # handling hierarchical
    $in =~ s/ \[[^ \]]+\]$//; # remove lhs nt
    next if $in =~ /\[[^ \]]+\]\[[^ \]]+\]/; # only consider flat rules
    $in = &get_factor_phrase($factor,$in) if defined($factor) && $factor eq "0"; 
    $scores = $COLUMN[4] if defined($hierarchical); #scalar @COLUMN == 5;
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
    push @DISTRIBUTION, $SCORE[$p_e_given_f_score]; # forward probability
  }
  my $entropy = &compute_entropy(@DISTRIBUTION);
  printf REPORT "%s\t%d\t%.5f\n",$last_in,$TTABLE_COVERED{$last_size}{$last_in},$entropy;
  $TTABLE_ENTROPY{$last_size}{$last_in} = $entropy;
  close(REPORT);
  close(TTABLE);

  &additional_coverage_reports($factor,"ttable",\%TTABLE_COVERED);
}

sub compute_entropy {
  my $z = 0; # normalization
  foreach my $p (@_) {
    $z += $p;
  }
  my $entropy = 0;
  foreach my $p (@_) {
    next if $p == 0;
    $entropy -= ($p/$z)*log($p/$z)/log(2);
  }
  return $entropy;
}

sub corpus_coverage {
  my ($factor) = @_;
  %CORPUS_COVERED = ();

  # compute how often input phrases occur in the corpus
  open(CORPUS,$corpus) or die "Can't read corpus $corpus";
  while(<CORPUS>) {
    my $line = &get_factor_phrase($factor,$_);
    my @WORD = split(/ /,$line);
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
  open(REPORT,">$dir/corpus-coverage-by-phrase".&factor_ext($factor)) or die "Cannot open: $!";
  foreach my $size (sort {$a <=> $b} keys %INPUT_PHRASE) {
    foreach my $phrase (keys %{$INPUT_PHRASE{$size}}) {
      next unless defined $CORPUS_COVERED{$size}{$phrase};
      printf REPORT "%s\t%d\n", $phrase, $CORPUS_COVERED{$size}{$phrase};
    }
  }
  close(REPORT);

  &additional_coverage_reports($factor,"corpus",\%CORPUS_COVERED);
}

sub additional_coverage_reports {
  my ($factor,$name,$COVERED) = @_;

  # unknown word report ---- TODO: extend to rare words?
  open(REPORT,">$dir/$name-unknown".&factor_ext($factor)) or die "Cannot open: $!";
  foreach my $phrase (keys %{$INPUT_PHRASE{1}}) {
    next if defined($$COVERED{1}{$phrase});
    printf REPORT "%s\t%d\n",$phrase,$INPUT_PHRASE{1}{$phrase};
  }
  close(REPORT);

  # summary report
  open(REPORT,">$dir/$name-coverage-summary".&factor_ext($factor)) or die "Cannot open: $!";
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
  open(OUT,">$dir/input-annotation") or die "Cannot open: $!";;
  open(INPUT,$input) or die "Can't read input $input";
  while(<INPUT>) {
    chop;
    s/\|\S+//g; # remove additional factors
    s/<\S[^>]*>//g; # remove xml markup
    s/\s+/ /g; s/^ //; s/ $//; # remove redundant spaces
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

    $sentence =~ s/[\r\n]+//g;
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

sub precision_by_coverage {
  my ($coverage_type) = @_;
  my (%PREC_BY_WORD,%TOTAL_BY_WORD,%LENGTH_BY_WORD,%DELETED_BY_WORD);
  my (%PREC_BY_COVERAGE,%TOTAL_BY_COVERAGE,%LENGTH_BY_COVERAGE,%DELETED_BY_COVERAGE);
  my (%PREC_BY_FACTOR,%TOTAL_BY_FACTOR,%LENGTH_BY_FACTOR,%DELETED_BY_FACTOR);
  my (%PREC_BY_FACTOR_COVERAGE,%TOTAL_BY_FACTOR_COVERAGE,%LENGTH_BY_FACTOR_COVERAGE,%DELETED_BY_FACTOR_COVERAGE);

  # get coverage statistics
  my %COVERAGE;
  print STDERR "".(defined($coverage_dir)?$coverage_dir:$dir)
      ."/$coverage_type-coverage-by-phrase";
  open(COVERAGE,(defined($coverage_dir)?$coverage_dir:$dir)
                ."/$coverage_type-coverage-by-phrase") or die "Cannot open: $!";
  while(<COVERAGE>) {
    chop;
    my ($phrase,$count) = split(/\t/);
    $COVERAGE{$phrase} = $count;
  }
  close(COVERAGE);

  # go through each line...
  open(FILE,$segmentation) || die("ERROR: could not open segmentation file $segmentation");
  open(INPUT,$input) or die "Can't read input $input";
  open(ALIGNMENT,$system_alignment) or die "Can't read output alignment file $system_alignment";

  # get marked up output
  my $line_count = 0;
  while(my $line = <FILE>) {
    chop($line);

    # get corresponding input line
    my $input = <INPUT>;
    my @INPUT = split(/ /,&get_factor_phrase(0,$input)); # surface
    my @FACTOR = split(/ /,&get_factor_phrase($precision_by_coverage_factor,$input));

    # word alignment
    my $alignment = <ALIGNMENT>;
    my %ALIGNED;
    foreach (split(/ /,$alignment)) {
      my ($input_pos,$output_pos) = split(/\-/,$_);
      push @{$ALIGNED{$input_pos}}, $output_pos;
    }

    # output words
    # @SYSTEM is already collected
    my @OUTPUT = split(/ /,$SYSTEM[$line_count]);

    # compute precision of each ngram
    # @REFERENCE (possibly multiple) is already collected
    my (%SYS_NGRAM,%REF_NGRAM,%PREC_NGRAM);
    &extract_n_grams( $SYSTEM[$line_count], \%SYS_NGRAM );
    &extract_n_grams_arrayopt( $REFERENCE[$line_count++], \%REF_NGRAM, "max" );
    foreach my $ngram (keys %{$SYS_NGRAM{1}}) { # note: only interested in unigram precision
      $PREC_NGRAM{1}{$ngram} = 0;
      if (defined($REF_NGRAM{1}) &&
	  defined($REF_NGRAM{1}{$ngram})) {
	my $ref_count = $REF_NGRAM{1}{$ngram};
        my $sys_count = $SYS_NGRAM{1}{$ngram};
	$PREC_NGRAM{1}{$ngram} = 
	  ($ref_count >= $sys_count) ? 1 : $ref_count/$sys_count;	    
      }
    }
    close(REPORT);

    # process one phrase at a time
    my $output_pos = 0;
    while($line =~ /([^|]+) \|(\d+)\-(\d+)\|\s*(.*)$/) {
      my ($output,$from,$to) = ($1,$2,$3);
      $line = $4;
        
      # bug fix: 1-1 unknown word mappings get alignment point
      if ($from == $to &&                         # one
	  scalar(split(/ /,$output)) == 1 &&      # to one 
	  !defined($ALIGNED{$from})) {             # but not aligned
	push @{$ALIGNED{$from}},$output_pos;
      }
      $output_pos += scalar(split(/ /,$output));

      # compute precision for each word
      for(my $i=$from; $i<=$to; $i++) {
	my $coverage = 0;
	$coverage = $COVERAGE{$INPUT[$i]} if defined($COVERAGE{$INPUT[$i]});

	my ($precision,$deleted,$length) = (0,0,0);

	# unaligned? note as deleted	
	if (!defined($ALIGNED{$i})) {
	  $deleted = 1;
	}
	# aligned 
	else {
	  foreach my $o (@{$ALIGNED{$i}}) {
	    $precision += $PREC_NGRAM{1}{$OUTPUT[$o]};
	  }
	  $precision /= scalar(@{$ALIGNED{$i}}); # average, if multi-aligned
	  $length = scalar(@{$ALIGNED{$i}});
	}

	my $word = $INPUT[$i];
	$word .= "\t".$FACTOR[$i] if $precision_by_coverage_factor;
	$DELETED_BY_WORD{$word} += $deleted;
	$PREC_BY_WORD{$word} += $precision;
	$LENGTH_BY_WORD{$word} += $length;
	$TOTAL_BY_WORD{$word}++;	

	$DELETED_BY_COVERAGE{$coverage} += $deleted;
	$PREC_BY_COVERAGE{$coverage} += $precision;
	$LENGTH_BY_COVERAGE{$coverage} += $length;
	$TOTAL_BY_COVERAGE{$coverage}++;	

	if ($precision_by_coverage_factor) {
	  $DELETED_BY_FACTOR{$FACTOR[$i]} += $deleted;
	  $DELETED_BY_FACTOR_COVERAGE{$FACTOR[$i]}{$coverage} += $deleted;
	  $PREC_BY_FACTOR{$FACTOR[$i]} += $precision;
	  $PREC_BY_FACTOR_COVERAGE{$FACTOR[$i]}{$coverage} += $precision;
	  $LENGTH_BY_FACTOR{$FACTOR[$i]} += $length;
	  $LENGTH_BY_FACTOR_COVERAGE{$FACTOR[$i]}{$coverage} += $length;	
	  $TOTAL_BY_FACTOR{$FACTOR[$i]}++;	    
	  $TOTAL_BY_FACTOR_COVERAGE{$FACTOR[$i]}{$coverage}++;	
        }
      }
    }
  }
  close(FILE);

  open(REPORT,">$dir/precision-by-$coverage_type-coverage") or die "Cannot open: $!";
  foreach my $coverage (sort {$a <=> $b} keys %TOTAL_BY_COVERAGE) {
    printf REPORT "%d\t%.3f\t%d\t%d\t%d\n", $coverage, $PREC_BY_COVERAGE{$coverage}, $DELETED_BY_COVERAGE{$coverage}, $LENGTH_BY_COVERAGE{$coverage}, $TOTAL_BY_COVERAGE{$coverage};
  }
  close(REPORT);

  open(REPORT,">$dir/precision-by-input-word") or die "Cannot open: $!";
  foreach my $word (keys %TOTAL_BY_WORD) {
    my ($w,$f) = split(/\t/,$word);
    my $coverage = 0;
    $coverage = $COVERAGE{$w} if defined($COVERAGE{$w});
   printf REPORT "%.3f\t%d\t%d\t%d\t%d\t%s\n", $PREC_BY_WORD{$word}, $DELETED_BY_WORD{$word}, $LENGTH_BY_WORD{$word}, $TOTAL_BY_WORD{$word},$coverage,$word;
  }
  close(REPORT);

  if ($precision_by_coverage_factor) {
    open(REPORT,">$dir/precision-by-$coverage_type-coverage.$precision_by_coverage_factor") or die "Cannot open: $!";
    foreach my $factor (sort keys %TOTAL_BY_FACTOR_COVERAGE) {
      foreach my $coverage (sort {$a <=> $b} keys %{$TOTAL_BY_FACTOR_COVERAGE{$factor}}) {
        printf REPORT "%s\t%d\t%.3f\t%d\t%d\t%d\n", $factor, $coverage, $PREC_BY_FACTOR_COVERAGE{$factor}{$coverage}, $DELETED_BY_FACTOR_COVERAGE{$factor}{$coverage}, $LENGTH_BY_FACTOR_COVERAGE{$factor}{$coverage}, $TOTAL_BY_FACTOR_COVERAGE{$factor}{$coverage};
      }
    }
    close(REPORT);
  }
}

sub segmentation {
  my %SEGMENTATION;

  open(FILE,$segmentation) || die("ERROR: could not open segmentation file $segmentation");
  open(OUT,">$dir/segmentation-annotation") or die "Cannot open: $!";
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

  open(SUMMARY,">$dir/segmentation") or die "Cannot open: $!";
  foreach my $in (sort { $a <=> $b } keys %SEGMENTATION) {
    foreach my $out (sort { $a <=> $b } keys %{$SEGMENTATION{$in}}) {
      printf SUMMARY "%d\t%d\t%d\n", $in, $out, $SEGMENTATION{$in}{$out};
    }
  }
  close(SUMMARY);

  # TODO: error by segmentation
}

# analyze the trace file to collect statistics over the
# hierarchical derivations and also create segmentation annotation
sub hierarchical_segmentation {
    my $last_sentence = -1;
    my @DERIVATION;
    my %STATS;
    open(TRACE,$segmentation.".trace") or die "Cannot open: $!";
    open(INPUT_TREE,">$dir/input-tree") or die "Cannot open: $!";
    open(OUTPUT_TREE,">$dir/output-tree") or die "Cannot open: $!";
    open(NODE,">$dir/node") or die "Cannot open: $!";
    while(<TRACE>) {
        my $sentence;
        my %ITEM;
        &hs_scan_line($_, \$sentence, \%ITEM) || die("cannot scan line $_");
        if ($last_sentence >= 0 && $sentence != $last_sentence) {
            &hs_process($last_sentence,\@DERIVATION,\%STATS);
            @DERIVATION = ();
        }
        push @DERIVATION,\%ITEM;
        $last_sentence = $sentence;
    }
    &hs_process($last_sentence,\@DERIVATION,\%STATS);
    close(TRACE);
    close(NODE);
    close(INPUT_TREE);
    close(OUTPUT_TREE);

    open(SUMMARY,">$dir/rule") or die "Cannot open: $!";
    print SUMMARY "sentence-count\t".(++$last_sentence)."\n";
    print SUMMARY "glue-rule\t".$STATS{'glue-rule'}."\n";
    print SUMMARY "depth\t".$STATS{'depth'}."\n";
    foreach (keys %{$STATS{'rule-type'}}) {
	print SUMMARY "rule\t$_\t".$STATS{'rule-type'}{$_}."\n";
    }
    close(SUMMARY);
}

# scan a single line of the trace file
sub hs_scan_line {
    my ($line,$ref_sentence,$ref_item) = @_;

    if ($line =~ /^Trans Opt/) {
        # Old format
        $line =~ /^Trans Opt (\d+) \[(\d+)\.\.(\d+)\]: (.+)  : (\S+) \-\>(.+) :([\(\),\d\- ]*): pC=[\d\.\-e]+, c=/ ||
        $line =~ /^Trans Opt (\d+) \[(\d+)\.\.(\d+)\]: (.+)  : (\S+) \-\>\S+  \-\> (.+) :([\(\),\d\- ]*): c=/ || return 0;
        my ($sentence,$start,$end,$spans,$rule_lhs,$rule_rhs,$alignment) = ($1,$2,$3,$4,$5,$6,$7);

        ${$ref_sentence} = $sentence;

        $ref_item->{'start'} = $start;
        $ref_item->{'end'} = $end;
        $ref_item->{'rule_lhs'} = $rule_lhs;

        $rule_rhs =~ s/</&lt;/g;
        $rule_rhs =~ s/>/&gt;/g;
        @{$ref_item->{'rule_rhs'}} = split(/ /,$rule_rhs);

        foreach (split(/ /,$alignment)) {
            /(\d+)[\-,](\d+)/ || die("funny alignment: $_\n");
            $ref_item->{'alignment'}{$2} = $1; # target non-terminal to source span
            $ref_item->{'alignedSpan'}{$1} = 1;
        }

        @{$ref_item->{'spans'}} = ();
        foreach my $span (reverse split(/\s+/,$spans)) {
            $span =~ /\[(\d+)\.\.(\d+)\]=(\S+)$/ || die("funny span: $span\n");
            my %SPAN = ( 'from' => $1, 'to' => $2, 'word' => $3 );
            push @{$ref_item->{'spans'}}, \%SPAN;
        }
    } else {
        # New format
        $line =~ /^(\d+) \|\|\| \[\S+\] -> (.+) \|\|\| \[(\S+)\] -> (.+) \|\|\| (.*)\|\|\| (.*)/ || return 0;
        my ($sentence,$source_rhs,$target_lhs,$target_rhs,$alignment,$source_spans) = ($1,$2,$3,$4,$5,$6);

        ${$ref_sentence} = $sentence;

        @{$ref_item->{'spans'}} = ();
        foreach (split(/ /,$source_rhs)) {
            /^\[?([^\]]+)\]?$/;
            my %SPAN = ( 'word' => $1 );
            push @{$ref_item->{'spans'}}, \%SPAN;
        }

        my $i = 0;
        foreach my $span (split(/ /,$source_spans)) {
            $span =~ /(\d+)\.\.(\d+)/ || die("funny span: $span\n");
            $ref_item->{'spans'}[$i]{'from'} = $1;
            $ref_item->{'spans'}[$i]{'to'} = $2;
            if ($i == 0) {
                $ref_item->{'start'} = $1;
            }
            $ref_item->{'end'} = $2;
            $i++;
        }

        $ref_item->{'rule_lhs'} = $target_lhs;

        $target_rhs =~ s/</&lt;/g;
        $target_rhs =~ s/>/&gt;/g;
        @{$ref_item->{'rule_rhs'}} = ();
        foreach (split(/ /,$target_rhs)) {
            /^\[?([^\]]+)\]?$/;
            push @{$ref_item->{'rule_rhs'}}, $1;
        }

        foreach (split(/ /,$alignment)) {
            /(\d+)[\-,](\d+)/ || die("funny alignment: $_\n");
            $ref_item->{'alignment'}{$2} = $1; # target non-terminal to source span
            $ref_item->{'alignedSpan'}{$1} = 1;
        }
    }

    return 1;
}

# process a single sentence for hierarchical segmentation
sub hs_process {
    my ($sentence,$DERIVATION,$STATS) = @_;
    
    my $DROP_RULE = shift @{$DERIVATION}; # get rid of S -> S </s>
    my $max = $$DERIVATION[0]{'end'};
    
    # consolidate glue rules into one rule
    my %GLUE_RULE;
    $GLUE_RULE{'start'} = 1;
    $GLUE_RULE{'end'} = $max;
    $GLUE_RULE{'rule_lhs'} = "S";
    $GLUE_RULE{'depth'} = 0;
    my $x=0;
    while(1) {
	my $RULE = shift @{$DERIVATION};
	if (scalar(@{$$RULE{'rule_rhs'}}) == 2 &&
	     ($$RULE{'rule_lhs'} eq "S" && 
	      $$RULE{'rule_rhs'}[0] eq "S" &&
	      $$RULE{'rule_rhs'}[1] eq "X") ||
	     ($$RULE{'rule_lhs'} eq "Q" && 
	      $$RULE{'rule_rhs'}[0] eq "Q")) {
	    unshift @{$GLUE_RULE{'spans'}},$$RULE{'spans'}[1];
	    push @{$GLUE_RULE{'rule_rhs'}}, $$RULE{'rule_rhs'}[1];
	    $GLUE_RULE{'alignment'}{$x} = $x;
	    $GLUE_RULE{'alignedSpan'}{$x} = 1;
	    $x++;
	}
	else {
	    unshift @{$DERIVATION}, $RULE;
	    last;
	}
    }
    unshift @{$DERIVATION}, \%GLUE_RULE;	
    $$STATS{'glue-rule'} += $x;
    
    # create chart
    my %CHART;
    foreach my $RULE (@{$DERIVATION}) {
	$CHART{$$RULE{'start'}}{$$RULE{'end'}} = $RULE;
    }
    
    # compute depth
    &hs_compute_depth(1,$max,0,\%CHART);	
    my $max_depth = 0;
    foreach my $RULE (@{$DERIVATION}) {
	next unless defined($$RULE{'depth'}); # better: delete offending rule S -> S <s>
	$max_depth = $$RULE{'depth'} if $$RULE{'depth'} > $max_depth;
    }
    &hs_recompute_depth(1,$max,\%CHART,$max_depth);
    $$STATS{'depth'} += $max_depth;
    
    # build matrix of divs
    
    my @MATRIX;
    &hs_create_out_span(1,$max,\%CHART,\@MATRIX);
    print OUTPUT_TREE &hs_output_matrix($sentence,\@MATRIX,$max_depth);
    
    my @MATRIX_IN;
    &hs_create_in_span(1,$max,\%CHART,\@MATRIX_IN);
    print INPUT_TREE &hs_output_matrix($sentence,\@MATRIX_IN,$max_depth);
    
    # number rules and get their children
    my $id = 0;
    foreach my $RULE (@{$DERIVATION}) {
	next unless defined($$RULE{'start_div'}); # better: delete offending rule S -> S <s>
	$$STATS{'rule-type'}{&hs_rule_type($RULE)}++ if $id>0;
	$$RULE{'id'} = $id++;
    }
    &hs_get_children(1,$max,\%CHART);
    
    foreach my $RULE (@{$DERIVATION}) {
	next unless defined($$RULE{'start_div'}); # better: delete offending rule S -> S <s>
	
	print NODE $sentence." ";
	print NODE $$RULE{'depth'}." ";
	print NODE $$RULE{'start_div'}." ".$$RULE{'end_div'}." ";
	print NODE $$RULE{'start_div_in'}." ".$$RULE{'end_div_in'}." ";
	print NODE join(",",@{$$RULE{'children'}})."\n";
    }
}

sub hs_output_matrix {
    my ($sentence,$MATRIX,$max_depth) = @_;
    my @OPEN;
    my $out = "";
    for(my $d=0;$d<=$max_depth;$d++) { push @OPEN, 0; }
    foreach my $SPAN (@$MATRIX) {
	$out .= $sentence."\t";
	for(my $d=0;$d<=$max_depth;$d++) {
	    my $class = " ";
	    my $closing_flag = 0;
	    if (defined($$SPAN{'closing'}) && defined($$SPAN{'closing'}{$d})) {
		$closing_flag = 1;
	    }
	    if ($d == $$SPAN{'depth'}) {
		if (defined($$SPAN{'opening'}) && $closing_flag) {
		    $class = "O";
		}
		elsif(defined($$SPAN{'opening'})) {
		    $class = "[";
		}
		elsif($closing_flag) {
		    $class = "]";
		}
		else {
		    $class = "-";
		}
	    }
	    elsif ($closing_flag) {
		$class = "]";
	    }
	    elsif ($OPEN[$d]) {
		$class = "-";				
	    }
	    $out .= $class;
	}
	$out .= "\t";		
	$out .= $$SPAN{'lhs'} if defined($$SPAN{'lhs'});
	$out .= "\t";
	$out .= $$SPAN{'rhs'} if defined($$SPAN{'rhs'});
	$out .= "\n";
	$OPEN[$$SPAN{'depth'}] = 1 if defined($$SPAN{'opening'});
	if(defined($$SPAN{'closing'})) {
	    for(my $d=$max_depth;$d>=0;$d--) {
		$OPEN[$d] = 0 if defined($$SPAN{'closing'}{$d});
	    }
	}
    }
    return $out;
}

sub hs_rule_type {
    my ($RULE) = @_;
    
    my $type = "";
    
    # output side
    my %NT;
    my $total_word_count = 0;
    my $word_count = 0;
    my $nt_count = 0;
    for(my $i=0;$i<scalar @{$$RULE{'rule_rhs'}};$i++) {
	if (defined($$RULE{'alignment'}{$i})) {
	    $type .= $word_count if $word_count > 0;
	    $word_count = 0;
	    my $nt = chr(97+$nt_count++);
	    $NT{$$RULE{'alignment'}{$i}} = $nt;
	    $type .= $nt;			
	}
	else {
	    $word_count++;
	    $total_word_count++;
	}
    }
    $type .= $word_count if $word_count > 0;
    
    $type .= ":".$total_word_count.":".$nt_count.":";
    
    # input side
    $word_count = 0;
    $total_word_count = 0;
    for(my $i=0;$i<scalar(@{$$RULE{'spans'}});$i++) {
	my $SUBSPAN = ${$$RULE{'spans'}}[$i];
	if (defined($$RULE{'alignedSpan'}{$i})) {
	    $type .= $word_count if $word_count > 0;
	    $word_count = 0;
	    $type .= $NT{$i};
	}
	else {
	    $word_count++;
	    $total_word_count++;
	}
    }
    $type .= $word_count if $word_count > 0;
    $type .= ":".$total_word_count;
    return $type;
}

# compute depth of each node
sub hs_compute_depth {
    my ($start,$end,$depth,$CHART)  = @_;
    if (!defined($$CHART{$start}{$end})) {
      print STDERR "warning: illegal span ($start,$end)\n";
      return;
    }
    my $RULE = $$CHART{$start}{$end};

    $$RULE{'depth'} = $depth;
    
    for(my $i=0;$i<scalar @{$$RULE{'rule_rhs'}};$i++) {
	# non-terminals
	if (defined($$RULE{'alignment'}{$i})) {
	    my $SUBSPAN = $$RULE{'spans'}[$$RULE{'alignment'}{$i}];
	    &hs_compute_depth($$SUBSPAN{'from'},$$SUBSPAN{'to'},$depth+1,$CHART);
	}
    }
}

# re-assign depth to as deep as possible
sub hs_recompute_depth {
    my ($start,$end,$CHART,$max_depth)  = @_;
    if (!defined($$CHART{$start}{$end})) {
      print STDERR "warning: illegal span ($start,$end)\n";
      return 0;
    }
    my $RULE = $$CHART{$start}{$end};
    
    my $min_sub_depth = $max_depth+1;
    for(my $i=0;$i<scalar @{$$RULE{'rule_rhs'}};$i++) {
	# non-terminals
	if (defined($$RULE{'alignment'}{$i})) {
	    my $SUBSPAN = $$RULE{'spans'}[$$RULE{'alignment'}{$i}];
	    my $sub_depth = &hs_recompute_depth($$SUBSPAN{'from'},$$SUBSPAN{'to'},$CHART,$max_depth);
	    $min_sub_depth = $sub_depth if $sub_depth < $min_sub_depth;
	}
    }
    $$RULE{'depth'} = $min_sub_depth-1;
    return $$RULE{'depth'};
}

# get child dependencies for a sentence
sub hs_get_children {
    my ($start,$end,$CHART)  = @_;
    if (!defined($$CHART{$start}{$end})) {
      print STDERR "warning: illegal span ($start,$end)\n";
      return -1;
    }
    my $RULE = $$CHART{$start}{$end};
    
    my @CHILDREN = ();
    $$RULE{'children'} = \@CHILDREN;
    
    for(my $i=0;$i<scalar @{$$RULE{'rule_rhs'}};$i++) {
	# non-terminals
	if (defined($$RULE{'alignment'}{$i})) {
	    my $SUBSPAN = $$RULE{'spans'}[$$RULE{'alignment'}{$i}];
	    my $child = &hs_get_children($$SUBSPAN{'from'},$$SUBSPAN{'to'},$CHART);
	    push @CHILDREN, $child unless $child == -1;
	}
    }
    return $$RULE{'id'};	
}

# create the span annotation for an output sentence
sub hs_create_out_span {
    my ($start,$end,$CHART,$MATRIX) = @_;
    if (!defined($$CHART{$start}{$end})) {
      print STDERR "warning: illegal span ($start,$end)\n";
      return;
    }
    my $RULE = $$CHART{$start}{$end};
    
    my %SPAN;
    $SPAN{'start'} = $start;
    $SPAN{'end'} = $end;
    $SPAN{'depth'} = $$RULE{'depth'};
    $SPAN{'lhs'} = $$RULE{'rule_lhs'};
    $SPAN{'opening'} = 1;
    push @{$MATRIX},\%SPAN;
    $$RULE{'start_div'} = $#{$MATRIX};
    my $THIS_SPAN = \%SPAN;
    # in output order ...
    my $terminal = 1;
    for(my $i=0;$i<scalar @{$$RULE{'rule_rhs'}};$i++) {
	# non-terminals
	if (defined($$RULE{'alignment'}{$i})) {
	    my $SUBSPAN = $$RULE{'spans'}[$$RULE{'alignment'}{$i}];
	    &hs_create_out_span($$SUBSPAN{'from'},$$SUBSPAN{'to'},$CHART,$MATRIX);
	    $terminal = 0;
	}
	# terminals
	else {
	    # new sequence of terminals?
	    if (!$terminal) {
		my %SPAN;
		$SPAN{'start'} = $start;
		$SPAN{'end'} = $end;
		$SPAN{'depth'} = $$RULE{'depth'};
		push @{$MATRIX},\%SPAN;
		$THIS_SPAN = \%SPAN;								
	    }
	    $$THIS_SPAN{'rhs'} .= " " if defined($$THIS_SPAN{'rhs'});
	    $$THIS_SPAN{'rhs'} .= $$RULE{"rule_rhs"}[$i];
	    $terminal = 1;
	}
    }
    $THIS_SPAN = $$MATRIX[scalar(@{$MATRIX})-1];
    $$RULE{'end_div'} = $#{$MATRIX};
    $$THIS_SPAN{'closing'}{$$RULE{'depth'}} = 1;
}

# create the span annotation for an input sentence
sub hs_create_in_span {
    my ($start,$end,$CHART,$MATRIX) = @_;
    if (!defined($$CHART{$start}{$end})) {
      print STDERR "warning: illegal span ($start,$end)\n";
      return;
    }
    my $RULE = $$CHART{$start}{$end};
    
    my %SPAN;
    $SPAN{'start'} = $start;
    $SPAN{'end'} = $end;
    $SPAN{'depth'} = $$RULE{'depth'};
    $SPAN{'lhs'} = $$RULE{'rule_lhs'};
    $SPAN{'opening'} = 1;
    push @{$MATRIX},\%SPAN;
    $$RULE{'start_div_in'} = $#{$MATRIX};
    my $THIS_SPAN = \%SPAN;
    
    my $terminal = 1;
    # in input order ...
    for(my $i=0;$i<scalar(@{$$RULE{'spans'}});$i++) {
	my $SUBSPAN = ${$$RULE{'spans'}}[$i];
	if (defined($$RULE{'alignedSpan'}{$i})) {
	    &hs_create_in_span($$SUBSPAN{'from'},$$SUBSPAN{'to'},$CHART,$MATRIX);
	    $terminal = 0;
	}
	else {
	    # new sequence of terminals?
	    if (!$terminal) {
		my %SPAN;
		$SPAN{'start'} = $start;
		$SPAN{'end'} = $end;
		$SPAN{'depth'} = $$RULE{'depth'};
		push @{$MATRIX},\%SPAN;
		$THIS_SPAN = \%SPAN;								
	    }
	    $$THIS_SPAN{'rhs'} .= " " if defined($$THIS_SPAN{'rhs'});
	    $$THIS_SPAN{'rhs'} .= $$SUBSPAN{'word'};
	    $terminal = 1;
	}
    }
    $THIS_SPAN = $$MATRIX[scalar(@{$MATRIX})-1];
    $$RULE{'end_div_in'} = $#{$MATRIX};
    $$THIS_SPAN{'closing'}{$$RULE{'depth'}} = 1;
}

sub process_search_graph {
  my ($search_graph_file) = @_;
  open(OSG,$search_graph) || die("ERROR: could not open search graph file '$search_graph_file'");
  `mkdir -p $dir/search-graph`;
  my $last_sentence = -1;
  while(<OSG>) {
    my ($sentence,$id,$recomb,$lhs,$output,$alignment,$rule_score,$heuristic_rule_score,$from,$to,$children,$hyp_score);
    if (/^(\d+) (\d+)\-?\>?(\S*) (\S+) =\> (.+) :(.*): pC=([\de\-\.]+), c=([\de\-\.]+) \[(\d+)\.\.(\d+)\] (.*)\[total=([\d\-\.]+)\] \<\</) {
      ($sentence,$id,$recomb,$lhs,$output,$alignment,$rule_score,$heuristic_rule_score,$from,$to,$children,$hyp_score) = ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12);
    }
    elsif (/^(\d+) (\d+)\-?\>?(\S*) (\S+) =\> (.+) :(.*): c=([\de\-\.]+) \[(\d+)\.\.(\d+)\] (.*)\[total=([\d\-\.]+)\] core/) {
      ($sentence,$id,$recomb,$lhs,$output,$alignment,$rule_score,$from,$to,$children,$hyp_score) = ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12);
      $heuristic_rule_score = $rule_score; # hmmmm....
    }
    else {
      die("ERROR: buggy search graph line: $_"); 
    }
    chop($alignment) if $alignment;
    chop($children) if $children;
    $recomb = 0 unless $recomb;
    $children = "" unless defined $children;
    $alignment = "" unless defined $alignment;
    if ($last_sentence != $sentence) {
      close(SENTENCE) if $sentence;
      open(SENTENCE,">$dir/search-graph/graph.$sentence");
      $last_sentence = $sentence;
    }
    print SENTENCE "$id\t$recomb\t$from\t$to\t$output\t$alignment\t$children\t$rule_score\t$heuristic_rule_score\t$hyp_score\t$lhs\n";
  }
  close(OSG);
  close(SENTENCE);
}
