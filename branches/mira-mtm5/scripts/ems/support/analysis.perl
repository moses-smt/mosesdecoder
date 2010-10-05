#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";

my $MAX_LENGTH = 4;

my ($system,$segmentation,$reference,$dir,$input,$corpus,$ttable,$hierarchical);
if (!&GetOptions('system=s' => \$system, # raw output from decoder
                 'reference=s' => \$reference, # tokenized reference
                 'dir=s' => \$dir, # directory for storing results
                 'input=s' => \$input, # tokenized input (as for decoder)
                 'segmentation=s' => \$segmentation, # system output with segmentation markup
                 'input-corpus=s' => \$corpus, # input side of parallel training corpus
                 'ttable=s' => \$ttable, # phrase translation table used for decoding
		 'hierarchical' => \$hierarchical) || # hierarchical model?
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
    if (defined($hierarchical)) {
	&hierarchical_segmentation();
    }
    else {
	&segmentation();
    }
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
    my @COLUMN = split(/ \|\|\| /);
    my ($in,$out,$scores) = @COLUMN;
    # handling hierarchical
    $in =~ s/\[[^ \]]+\]$//; # remove lhs nt
    next if $in =~ /\[[^ \]]+\]\[[^ \]]+\]/; # only consider flat rules
    $scores = $COLUMN[4] if scalar @COLUMN == 5;
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
    s/\|\S+//g; # remove additional factors
    s/<[^>]+>//g; # remove xml markup
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

# analyze the trace file to collect statistics over the
# hierarchical derivations and also create segmentation annotation
sub hierarchical_segmentation {
    my $last_sentence = -1;
    my @DERIVATION;
    my %STATS;
    open(TRACE,$segmentation.".trace");
    open(INPUT_TREE,">$dir/input-tree");
    open(OUTPUT_TREE,">$dir/output-tree");
    open(NODE,">$dir/node");
    while(<TRACE>) {
	/^Trans Opt (\d+) \[(\d+)\.\.(\d+)\]: (.+)  : (\S+) \-\>(.+) :([\d\- ]*): pC=[\d\.\-e]+, c=/ || die("cannot scan line $_");
	my ($sentence,$start,$end,$spans,$rule_lhs,$rule_rhs,$alignment) = ($1,$2,$3,$4,$5,$6,$7);
	if ($last_sentence >= 0 && $sentence != $last_sentence) {
	    &hs_process($last_sentence,\@DERIVATION,\%STATS);
	    @DERIVATION = ();
	}
	my %ITEM;
	$ITEM{'start'} = $start;
	$ITEM{'end'} = $end;
	$ITEM{'rule_lhs'} = $rule_lhs;
	
	$rule_rhs =~ s/</&lt;/g;
	$rule_rhs =~ s/>/&gt;/g;
	@{$ITEM{'rule_rhs'}} = split(/ /,$rule_rhs);
	
	foreach (split(/ /,$alignment)) {
		/(\d+)\-(\d+)/ || die("funny alignment: $_\n");
		$ITEM{'alignment'}{$2} = $1; # target non-terminal to source span
		$ITEM{'alignedSpan'}{$1} = 1;
	}

	@{$ITEM{'spans'}} = ();
	foreach my $span (reverse split(/\s+/,$spans)) {
		$span =~ /\[(\d+)\.\.(\d+)\]=(\S+)$/ || die("funny span: $span\n");
		my %SPAN = ( 'from' => $1, 'to' => $2, 'word' => $3 );
		push @{$ITEM{'spans'}}, \%SPAN;
	}

	push @DERIVATION,\%ITEM;
	$last_sentence = $sentence;
    }
    &hs_process($last_sentence,\@DERIVATION,\%STATS);
    close(TRACE);
    close(NODE);
    close(INPUT_TREE);
    close(OUTPUT_TREE);

    open(SUMMARY,">$dir/rule");
    print SUMMARY "sentence-count\t".(++$last_sentence)."\n";
    print SUMMARY "glue-rule\t".$STATS{'glue-rule'}."\n";
    print SUMMARY "depth\t".$STATS{'depth'}."\n";
    foreach (keys %{$STATS{'rule-type'}}) {
	print SUMMARY "rule\t$_\t".$STATS{'rule-type'}{$_}."\n";
    }
    close(SUMMARY);
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
	if ($$RULE{'rule_lhs'} eq "S" && 
	    scalar(@{$$RULE{'rule_rhs'}}) == 2 &&
	    $$RULE{'rule_rhs'}[0] eq "S" &&
	    $$RULE{'rule_rhs'}[1] eq "X") {
	    unshift @{$GLUE_RULE{'spans'}},$$RULE{'spans'}[1];
	    push @{$GLUE_RULE{'rule_rhs'}}, "X";
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
    my $RULE = $$CHART{$start}{$end};
    $$RULE{'depth'} = $depth;
    
    for(my $i=0;$i<scalar @{$$RULE{'rule_rhs'}};$i++) {
	# non-terminals
	if (defined($$RULE{'alignment'}{$i})) {
	    my $SUBSPAN = $$RULE{'spans'}[$$RULE{'alignment'}{$i}];
	    &hs_compute_depth($$SUBSPAN{'from'},$$SUBSPAN{'to'},$depth+1,$CHART);
	}
    }
}

# re-assign depth to as deep as possible
sub hs_recompute_depth {
    my ($start,$end,$CHART,$max_depth)  = @_;
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
    my $RULE = $$CHART{$start}{$end};
    
    my @CHILDREN = ();
    $$RULE{'children'} = \@CHILDREN;
    
    for(my $i=0;$i<scalar @{$$RULE{'rule_rhs'}};$i++) {
	# non-terminals
	if (defined($$RULE{'alignment'}{$i})) {
	    my $SUBSPAN = $$RULE{'spans'}[$$RULE{'alignment'}{$i}];
	    my $child = &hs_get_children($$SUBSPAN{'from'},$$SUBSPAN{'to'},$CHART);
	    push @CHILDREN, $child;
	}
    }
    return $$RULE{'id'};	
}

# create the span annotation for an output sentence
sub hs_create_out_span {
    my ($start,$end,$CHART,$MATRIX) = @_;
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
