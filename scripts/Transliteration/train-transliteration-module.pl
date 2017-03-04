#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use utf8;
use strict;
use Getopt::Long "GetOptions";
use FindBin qw($RealBin);
binmode(STDIN,  ':utf8');
binmode(STDOUT, ':utf8');
binmode(STDERR, ':utf8');

print STDERR "Training Transliteration Module - Start\n".`date`;

my $ORDER = 5;
my $OUT_DIR = "/tmp/Transliteration-Model.$$";
my $___FACTOR_DELIMITER = "|";
my ($MOSES_SRC_DIR,$CORPUS_F,$CORPUS_E,$ALIGNMENT,$SRILM_DIR,$FACTOR,$EXTERNAL_BIN_DIR,$INPUT_EXTENSION, $OUTPUT_EXTENSION, $SOURCE_SYNTAX, $TARGET_SYNTAX,$DECODER);

# utilities
my $ZCAT = "gzip -cd";
my $BZCAT = "bzcat";

die("ERROR: wrong syntax when invoking train-transliteration-module.perl")
    unless &GetOptions('moses-src-dir=s' => \$MOSES_SRC_DIR,
			'external-bin-dir=s' => \$EXTERNAL_BIN_DIR,
			'input-extension=s' => \$INPUT_EXTENSION,
			'output-extension=s' => \$OUTPUT_EXTENSION,
		       'corpus-f=s' => \$CORPUS_F,
		       'corpus-e=s' => \$CORPUS_E,
		       'alignment=s' => \$ALIGNMENT,
		       'order=i' => \$ORDER,
		       'factor=s' => \$FACTOR,
		       'srilm-dir=s' => \$SRILM_DIR,
		       'out-dir=s' => \$OUT_DIR,
		       'decoder=s' => \$DECODER,
		       'source-syntax' => \$SOURCE_SYNTAX,
		       'target-syntax' => \$TARGET_SYNTAX);

# check if the files are in place
die("ERROR: you need to define --corpus-e, --corpus-f, --alignment, --srilm-dir, --moses-src-dir --external-bin-dir, --input-extension and --output-extension")
    unless (defined($MOSES_SRC_DIR) &&
            defined($CORPUS_F) &&
            defined($CORPUS_E) &&
            defined($ALIGNMENT)&&
	     defined($INPUT_EXTENSION)&&
	     defined($OUTPUT_EXTENSION)&&
	     defined($EXTERNAL_BIN_DIR)&&
            defined($SRILM_DIR));
die("ERROR: could not find input corpus file '$CORPUS_F'")
    unless -e $CORPUS_F;
die("ERROR: could not find output corpus file '$CORPUS_E'")
    unless -e $CORPUS_E;
die("ERROR: could not find alignment file '$ALIGNMENT'")
    unless -e $ALIGNMENT;
$DECODER = "$MOSES_SRC_DIR/bin/moses" unless defined($DECODER);

`mkdir $OUT_DIR`;

# strip XML, if present
my $stripped_corpus_f = $CORPUS_F;
if (defined($SOURCE_SYNTAX)) {
    $stripped_corpus_f = "$OUT_DIR/stripped.$INPUT_EXTENSION";
    &strip_xml($CORPUS_F, $stripped_corpus_f);
}
my $stripped_corpus_e = $CORPUS_E;
if (defined($TARGET_SYNTAX)) {
    $stripped_corpus_e = "$OUT_DIR/stripped.$OUTPUT_EXTENSION";
    &strip_xml($CORPUS_E, $stripped_corpus_e);
}

# create factors
if (defined($FACTOR)) {

   my @factor_values = split(',', $FACTOR);

    foreach my $factor_val (@factor_values) {

  my ($factor_f,$factor_e) = split(/\-/,$factor_val);

    $stripped_corpus_f =~ /^(.+)\.([^\.]+)/;
    my ($corpus_stem_f,$ext_f) = ($1,$2);
    $stripped_corpus_e =~ /^(.+)\.([^\.]+)/;
    my ($corpus_stem_e,$ext_e) = ($1,$2);
    &reduce_factors($stripped_corpus_f,"$corpus_stem_f.$factor_val.$ext_f",$factor_f);
    &reduce_factors($stripped_corpus_e,"$corpus_stem_e.$factor_val.$ext_e",$factor_e);

    `ln -s $corpus_stem_f.$factor_val.$ext_f $OUT_DIR/f`;
    `ln -s $corpus_stem_e.$factor_val.$ext_e $OUT_DIR/e`;
    `ln -s $ALIGNMENT $OUT_DIR/a`;


  }
}
else {
    `ln -s $stripped_corpus_f $OUT_DIR/f`;
    `ln -s $stripped_corpus_e $OUT_DIR/e`;
    `ln -s $ALIGNMENT $OUT_DIR/a`;

     }

     mine_transliterations($INPUT_EXTENSION, $OUTPUT_EXTENSION);
     train_transliteration_module();
     retrain_transliteration_module();


# create model

print STDERR "Training Transliteration Module - End ".`date`;

sub learn_transliteration_model{

  my ($t) = @_;

   `cp $OUT_DIR/training/corpus$t.$OUTPUT_EXTENSION $OUT_DIR/lm/target`;

   print STDERR "Align Corpus\n";

  `$MOSES_SRC_DIR/scripts/training/train-model.perl \\
      -mgiza -mgiza-cpus 10 -dont-zip -last-step 1 \\
      -external-bin-dir $EXTERNAL_BIN_DIR -f $INPUT_EXTENSION \\
      -e $OUTPUT_EXTENSION -alignment grow-diag-final-and -parts 5 \\
      -score-options '--KneserNey' -corpus $OUT_DIR/training/corpus$t \\
      -corpus-dir $OUT_DIR/training/prepared`;

  `$MOSES_SRC_DIR/scripts/training/train-model.perl -mgiza -mgiza-cpus 10 \\
      -dont-zip -first-step 2 -last-step 2 \\
      -external-bin-dir $EXTERNAL_BIN_DIR -f $INPUT_EXTENSION \\
      -e $OUTPUT_EXTENSION -alignment grow-diag-final-and -parts 5 \\
      -score-options '--KneserNey' -corpus-dir $OUT_DIR/training/prepared \\
      -giza-e2f $OUT_DIR/training/giza -direction 2`;

  `$MOSES_SRC_DIR/scripts/training/train-model.perl \\
      -mgiza -mgiza-cpus 10 -dont-zip -first-step 2 -last-step 2 \\
      -external-bin-dir $EXTERNAL_BIN_DIR -f $INPUT_EXTENSION \\
      -e $OUTPUT_EXTENSION -alignment grow-diag-final-and -parts 5 \\
      -score-options '--KneserNey' -corpus-dir $OUT_DIR/training/prepared \\
      -giza-f2e $OUT_DIR/training/giza-inverse -direction 1`;

  `$MOSES_SRC_DIR/scripts/training/train-model.perl \\
      -mgiza -mgiza-cpus 10 -dont-zip -first-step 3 -last-step 3 \\
      -external-bin-dir $EXTERNAL_BIN_DIR -f $INPUT_EXTENSION \\
      -e $OUTPUT_EXTENSION -alignment grow-diag-final-and -parts 5 \\
      -score-options '--KneserNey' -giza-e2f $OUT_DIR/training/giza \\
      -giza-f2e $OUT_DIR/training/giza-inverse \\
      -alignment-file $OUT_DIR/model/aligned \\
      -alignment-stem $OUT_DIR/model/aligned -alignment grow-diag-final-and`;

  print STDERR "Train Translation Models\n";

 `$MOSES_SRC_DIR/scripts/training/train-model.perl \\
     -mgiza -mgiza-cpus 10 -dont-zip -first-step 4 -last-step 4 \\
     -external-bin-dir $EXTERNAL_BIN_DIR -f $INPUT_EXTENSION \\
     -e $OUTPUT_EXTENSION -alignment grow-diag-final-and -parts 5 \\
     -score-options '--KneserNey' -lexical-file $OUT_DIR/model/lex \\
     -alignment-file $OUT_DIR/model/aligned \\
     -alignment-stem $OUT_DIR/model/aligned \\
     -corpus $OUT_DIR/training/corpus$t`;

  `$MOSES_SRC_DIR/scripts/training/train-model.perl \\
      -mgiza -mgiza-cpus 10 -dont-zip -first-step 5 -last-step 5 \\
      -external-bin-dir $EXTERNAL_BIN_DIR -f $INPUT_EXTENSION \\
      -e $OUTPUT_EXTENSION -alignment grow-diag-final-and -parts 5 \\
      -score-options '--KneserNey' -alignment-file $OUT_DIR/model/aligned \\
      -alignment-stem $OUT_DIR/model/aligned -extract-file \\
      $OUT_DIR/model/extract -corpus $OUT_DIR/training/corpus$t`;

  `$MOSES_SRC_DIR/scripts/training/train-model.perl \\
      -mgiza -mgiza-cpus 10 -dont-zip -first-step 6 -last-step 6 \\
      -external-bin-dir $EXTERNAL_BIN_DIR -f $INPUT_EXTENSION \\
      -e $OUTPUT_EXTENSION -alignment grow-diag-final-and -parts 5 \\
      -score-options '--KneserNey' -extract-file $OUT_DIR/model/extract \\
      -lexical-file $OUT_DIR/model/lex -phrase-translation-table \\
      $OUT_DIR/model/phrase-table`;

  print STDERR "Train Language Models\n";

  `$SRILM_DIR/ngram-count \\
      -order 5 -interpolate -kndiscount -addsmooth1 0.0 -unk \\
      -text $OUT_DIR/lm/target -lm $OUT_DIR/lm/targetLM`;

  `$MOSES_SRC_DIR/bin/build_binary \\
      $OUT_DIR/lm/targetLM $OUT_DIR/lm/targetLM.bin`;

  print STDERR "Create Config File\n";

  `$MOSES_SRC_DIR/scripts/training/train-model.perl \\
      -mgiza -mgiza-cpus 10 -dont-zip -first-step 9 \\
      -external-bin-dir $EXTERNAL_BIN_DIR -f $INPUT_EXTENSION \\
      -e $OUTPUT_EXTENSION -alignment grow-diag-final-and -parts 5 \\
      -score-options '--KneserNey' \\
      -phrase-translation-table $OUT_DIR/model/phrase-table \\
      -config $OUT_DIR/model/moses.ini -lm 0:5:$OUT_DIR/lm/targetLM.bin:8`;

}

sub retrain_transliteration_module{

   if (-e "$OUT_DIR/training/corpusA.$OUTPUT_EXTENSION")
   {
     `rm -r $OUT_DIR/model`;
     `rm -r $OUT_DIR/lm`;
     `rm -r $OUT_DIR/training/giza`;
     `rm -r $OUT_DIR/training/giza-inverse`;
     `rm -r $OUT_DIR/training/prepared`;
      `mkdir $OUT_DIR/model`;
      `mkdir $OUT_DIR/lm`;

      learn_transliteration_model("");
   }
}

sub train_transliteration_module{

   `mkdir $OUT_DIR/model`;
   `mkdir $OUT_DIR/lm`;
   print STDERR "Preparing Corpus\n";
   `$MOSES_SRC_DIR/scripts/Transliteration/corpusCreator.pl $OUT_DIR 1-1.$INPUT_EXTENSION-$OUTPUT_EXTENSION.mined-pairs $INPUT_EXTENSION $OUTPUT_EXTENSION`;

   if (-e "$OUT_DIR/training/corpusA.$OUTPUT_EXTENSION")
   {
     learn_transliteration_model("A");
   }
   else
   {
    learn_transliteration_model("");
   }

   print STDERR "Running Tuning for Transliteration Module\n";

    `touch $OUT_DIR/tuning/moses.table.ini`;

    `$MOSES_SRC_DIR/scripts/training/train-model.perl -mgiza -mgiza-cpus 10 -dont-zip -first-step 9 -external-bin-dir $EXTERNAL_BIN_DIR -f $INPUT_EXTENSION -e $OUTPUT_EXTENSION -alignment grow-diag-final-and -parts 5 -score-options '--KneserNey' -phrase-translation-table $OUT_DIR/model/phrase-table -config $OUT_DIR/tuning/moses.table.ini -lm 0:3:$OUT_DIR/tuning/moses.table.ini:8`;

    `$MOSES_SRC_DIR/scripts/training/filter-model-given-input.pl $OUT_DIR/tuning/filtered $OUT_DIR/tuning/moses.table.ini $OUT_DIR/tuning/input  -Binarizer "$MOSES_SRC_DIR/bin/CreateOnDiskPt 1 1 4 100 2"`;

    `rm $OUT_DIR/tuning/moses.table.ini`;

    `$MOSES_SRC_DIR/scripts/ems/support/substitute-filtered-tables.perl $OUT_DIR/tuning/filtered/moses.ini < $OUT_DIR/model/moses.ini > $OUT_DIR/tuning/moses.filtered.ini`;

    `$MOSES_SRC_DIR/scripts/training/mert-moses.pl $OUT_DIR/tuning/input $OUT_DIR/tuning/reference $DECODER $OUT_DIR/tuning/moses.filtered.ini --nbest 100 --working-dir $OUT_DIR/tuning/tmp  --decoder-flags "-threads 16 -drop-unknown -v 0 -distortion-limit 0" --rootdir $MOSES_SRC_DIR/scripts -mertdir $MOSES_SRC_DIR/bin -threads=16 --no-filter-phrase-table`;

    `cp $OUT_DIR/tuning/tmp/moses.ini $OUT_DIR/tuning/moses.ini`;

    `$MOSES_SRC_DIR/scripts/ems/support/substitute-weights.perl $OUT_DIR/model/moses.ini $OUT_DIR/tuning/moses.ini $OUT_DIR/tuning/moses.tuned.ini`;
}



sub mine_transliterations{

my @list = @_;
my $inp_ext = $list[0];
my $op_ext = $list[1];
my $count = 0;
my $l1 = 1;
my $l2 = 1;

print STDERR "Creating Model\n";

print STDERR "Extracting 1-1 Alignments\n";
`$MOSES_SRC_DIR/bin/1-1-Extraction $OUT_DIR/f $OUT_DIR/e $OUT_DIR/a > $OUT_DIR/1-1.$inp_ext-$op_ext`;

print STDERR "Cleaning the list for Miner\n";

`$MOSES_SRC_DIR/scripts/Transliteration/clean.pl $OUT_DIR/1-1.$inp_ext-$op_ext > $OUT_DIR/1-1.$inp_ext-$op_ext.cleaned`;


	if (-e "$OUT_DIR/1-1.$inp_ext-$op_ext.pair-probs")
	{
		print STDERR "1-1.$inp_ext-$op_ext.pair-probs in place, reusing\n";
	}
	else
	{
	print STDERR "Extracting Transliteration Pairs \n";
	 `$MOSES_SRC_DIR/bin/TMining $OUT_DIR/1-1.$inp_ext-$op_ext.cleaned > $OUT_DIR/1-1.$inp_ext-$op_ext.pair-probs`;
	}

print STDERR "Selecting Transliteration Pairs with threshold 0.5 \n";
`echo 0.5 | $MOSES_SRC_DIR/scripts/Transliteration/threshold.pl $OUT_DIR/1-1.$inp_ext-$op_ext.pair-probs > $OUT_DIR/1-1.$inp_ext-$op_ext.mined-pairs`;

}

# from train-model.perl
sub reduce_factors {
    my ($full,$reduced,$factors) = @_;

    my @INCLUDE = sort {$a <=> $b} split(/,/,$factors);

    print STDERR "Reducing factors to produce $reduced  @ ".`date`;
    while(-e $reduced.".lock") {
	sleep(10);
    }
    if (-e $reduced) {
        print STDERR "  $reduced in place, reusing\n";
        return;
    }
    if (-e $reduced.".gz") {
        print STDERR "  $reduced.gz in place, reusing\n";
        return;
    }

    # peek at input, to check if we are asked to produce exactly the
    # available factors
    my $inh = open_or_zcat($full);
    my $firstline = <$inh>;
    die "Corpus file $full is empty" unless $firstline;
    close $inh;
    # pick first word
    $firstline =~ s/^\s*//;
    $firstline =~ s/\s.*//;
    # count factors
    my $maxfactorindex = $firstline =~ tr/|/|/;
    if (join(",", @INCLUDE) eq join(",", 0..$maxfactorindex)) {
	# create just symlink; preserving compression
	my $realfull = $full;
	if (!-e $realfull && -e $realfull.".gz") {
            $realfull .= ".gz";
            $reduced =~ s/(\.gz)?$/.gz/;
	}
	safesystem("ln -s '$realfull' '$reduced'")
            or die "Failed to create symlink $realfull -> $reduced";
	return;
    }

    # The default is to select the needed factors
    `touch $reduced.lock`;
    *IN = open_or_zcat($full);
    open(OUT,">".$reduced) or die "ERROR: Can't write $reduced";
    my $nr = 0;
    while(<IN>) {
        $nr++;
        print STDERR "." if $nr % 10000 == 0;
        print STDERR "($nr)" if $nr % 100000 == 0;
	chomp; s/ +/ /g; s/^ //; s/ $//;
	my $first = 1;
	foreach (split) {
	    my @FACTOR = split /\Q$___FACTOR_DELIMITER/;
              # \Q causes to disable metacharacters in regex
	    print OUT " " unless $first;
	    $first = 0;
	    my $first_factor = 1;
            foreach my $outfactor (@INCLUDE) {
              print OUT "|" unless $first_factor;
              $first_factor = 0;
              my $out = $FACTOR[$outfactor];
              die "ERROR: Couldn't find factor $outfactor in token \"$_\" in $full LINE $nr" if !defined $out;
              print OUT $out;
            }
	}
	print OUT "\n";
    }
    print STDERR "\n";
    close(OUT);
    close(IN);
    `rm -f $reduced.lock`;
}

sub strip_xml {
  my ($source, $dest) = @_;
  my $cmd = "$MOSES_SRC_DIR/scripts/generic/strip-xml.perl < '$source' > '$dest'";
  safesystem($cmd);
}

sub open_or_zcat {
  my $fn = shift;
  my $read = $fn;
  $fn = $fn.".gz" if ! -e $fn && -e $fn.".gz";
  $fn = $fn.".bz2" if ! -e $fn && -e $fn.".bz2";
  if ($fn =~ /\.bz2$/) {
      $read = "$BZCAT $fn|";
  } elsif ($fn =~ /\.gz$/) {
      $read = "$ZCAT $fn|";
  }
  my $hdl;
  open($hdl,$read) or die "Can't read $fn ($read)";
  return $hdl;
}

sub safesystem {
  print STDERR "Executing: @_\n";
  system(@_);
  if ($? == -1) {
      print STDERR "ERROR: Failed to execute: @_\n  $!\n";
      exit(1);
  }
  elsif ($? & 127) {
      printf STDERR "ERROR: Execution of: @_\n  died with signal %d, %s coredump\n",
          ($? & 127),  ($? & 128) ? 'with' : 'without';
      exit(1);
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}

