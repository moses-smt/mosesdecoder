#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";
use FindBin qw($Bin);
use File::Spec::Functions;
use File::Basename;

# Train Factored Phrase Model
# (c) 2006-2009 Philipp Koehn
# with contributions from other JHU WS participants
# Train a model from a parallel corpus
# -----------------------------------------------------
$ENV{"LC_ALL"} = "C";
my $SCRIPTS_ROOTDIR = $Bin;
if ($SCRIPTS_ROOTDIR eq '') {
    $SCRIPTS_ROOTDIR = dirname(__FILE__);
}
$SCRIPTS_ROOTDIR =~ s/\/training$//;
$SCRIPTS_ROOTDIR = $ENV{"SCRIPTS_ROOTDIR"} if defined($ENV{"SCRIPTS_ROOTDIR"});

my($_ROOT_DIR, $_CORPUS_DIR, $_GIZA_E2F, $_GIZA_F2E, $_MODEL_DIR, $_TEMP_DIR, $_SORT_BUFFER_SIZE, $_CORPUS,
   $_CORPUS_COMPRESSION, $_FIRST_STEP, $_LAST_STEP, $_F, $_E, $_MAX_PHRASE_LENGTH,
   $_LEXICAL_FILE, $_NO_LEXICAL_WEIGHTING, $_VERBOSE, $_ALIGNMENT,
   $_ALIGNMENT_FILE, $_ALIGNMENT_STEM, @_LM, $_EXTRACT_FILE, $_GIZA_OPTION, $_HELP, $_PARTS,
   $_DIRECTION, $_ONLY_PRINT_GIZA, $_GIZA_EXTENSION, $_REORDERING,
   $_REORDERING_SMOOTH, $_INPUT_FACTOR_MAX, $_ALIGNMENT_FACTORS,
   $_TRANSLATION_FACTORS, $_REORDERING_FACTORS, $_GENERATION_FACTORS,
   $_DECODING_GRAPH_BACKOFF,
   $_DECODING_STEPS, $_PARALLEL, $_FACTOR_DELIMITER, @_PHRASE_TABLE,
   @_REORDERING_TABLE, @_GENERATION_TABLE, @_GENERATION_TYPE, $_GENERATION_CORPUS,
   $_DONT_ZIP,  $_MGIZA, $_MGIZA_CPUS,  $_HMM_ALIGN, $_CONFIG,
   $_HIERARCHICAL,$_XML,$_SOURCE_SYNTAX,$_TARGET_SYNTAX,$_GLUE_GRAMMAR,$_GLUE_GRAMMAR_FILE,$_UNKNOWN_WORD_LABEL_FILE,$_GHKM,$_EXTRACT_OPTIONS,$_SCORE_OPTIONS,
   $_PHRASE_WORD_ALIGNMENT,$_FORCE_FACTORED_FILENAMES,
   $_MEMSCORE, $_FINAL_ALIGNMENT_MODEL,
   $_CONTINUE,$_MAX_LEXICAL_REORDERING,$_DO_STEPS,
   $_ADDITIONAL_INI,
   $_DICTIONARY, $_EPPEX);

my $debug = 0; # debug this script, do not delete any files in debug mode

# the following line is set installation time by 'make release'.  BEWARE!
my $BINDIR="/home/pkoehn/statmt/bin";

$_HELP = 1
    unless &GetOptions('root-dir=s' => \$_ROOT_DIR,
		       'bin-dir=s' => \$BINDIR, # allow to override default bindir path
		       'corpus-dir=s' => \$_CORPUS_DIR,
		       'corpus=s' => \$_CORPUS,
		       'f=s' => \$_F,
		       'e=s' => \$_E,
		       'giza-e2f=s' => \$_GIZA_E2F,
		       'giza-f2e=s' => \$_GIZA_F2E,
		       'max-phrase-length=s' => \$_MAX_PHRASE_LENGTH,
		       'lexical-file=s' => \$_LEXICAL_FILE,
		       'no-lexical-weighting' => \$_NO_LEXICAL_WEIGHTING,
		       'model-dir=s' => \$_MODEL_DIR,
		       'temp-dir=s' => \$_TEMP_DIR,
           'sort-buffer-size=s' => \$_SORT_BUFFER_SIZE,
		       'extract-file=s' => \$_EXTRACT_FILE,
		       'alignment=s' => \$_ALIGNMENT,
		       'alignment-file=s' => \$_ALIGNMENT_FILE,
		       'alignment-stem=s' => \$_ALIGNMENT_STEM,
		       'verbose' => \$_VERBOSE,
		       'first-step=i' => \$_FIRST_STEP,
		       'last-step=i' => \$_LAST_STEP,
		       'giza-option=s' => \$_GIZA_OPTION,
		       'giza-extension=s' => \$_GIZA_EXTENSION,
		       'parallel' => \$_PARALLEL,
		       'lm=s' => \@_LM,
		       'help' => \$_HELP,
		       'mgiza' => \$_MGIZA, # multi-thread 
		       'mgiza-cpus=i' => \$_MGIZA_CPUS, # multi-thread 
		       'hmm-align' => \$_HMM_ALIGN,
		       'final-alignment-model=s' => \$_FINAL_ALIGNMENT_MODEL, # use word alignment model 1/2/hmm/3/4/5 as final (default is 4); value 'hmm' equivalent to the --hmm-align switch
		       'debug' => \$debug,
		       'dont-zip' => \$_DONT_ZIP,
		       'parts=i' => \$_PARTS,
		       'direction=i' => \$_DIRECTION,
		       'only-print-giza' => \$_ONLY_PRINT_GIZA,
		       'reordering=s' => \$_REORDERING,
		       'reordering-smooth=s' => \$_REORDERING_SMOOTH,
		       'input-factor-max=i' => \$_INPUT_FACTOR_MAX,
		       'alignment-factors=s' => \$_ALIGNMENT_FACTORS,
		       'translation-factors=s' => \$_TRANSLATION_FACTORS,
		       'reordering-factors=s' => \$_REORDERING_FACTORS,
		       'generation-factors=s' => \$_GENERATION_FACTORS,
		       'decoding-steps=s' => \$_DECODING_STEPS,
		       'decoding-graph-backoff=s' => \$_DECODING_GRAPH_BACKOFF,
		       'scripts-root-dir=s' => \$SCRIPTS_ROOTDIR,
		       'factor-delimiter=s' => \$_FACTOR_DELIMITER,
		       'phrase-translation-table=s' => \@_PHRASE_TABLE,
		       'generation-corpus=s' => \$_GENERATION_CORPUS,
		       'generation-table=s' => \@_GENERATION_TABLE,
		       'reordering-table=s' => \@_REORDERING_TABLE,
		       'generation-type=s' => \@_GENERATION_TYPE,
		       'continue' => \$_CONTINUE,
		       'hierarchical' => \$_HIERARCHICAL,
		       'glue-grammar' => \$_GLUE_GRAMMAR,
		       'glue-grammar-file=s' => \$_GLUE_GRAMMAR_FILE,
		       'unknown-word-label-file=s' => \$_UNKNOWN_WORD_LABEL_FILE,
		       'ghkm' => \$_GHKM,
		       'extract-options=s' => \$_EXTRACT_OPTIONS,
		       'score-options=s' => \$_SCORE_OPTIONS,
		       'source-syntax' => \$_SOURCE_SYNTAX,
		       'target-syntax' => \$_TARGET_SYNTAX,
		       'xml' => \$_XML,
		       'phrase-word-alignment' => \$_PHRASE_WORD_ALIGNMENT,
		       'config=s' => \$_CONFIG,
		       'max-lexical-reordering' => \$_MAX_LEXICAL_REORDERING,
		       'do-steps=s' => \$_DO_STEPS,
		       'memscore:s' => \$_MEMSCORE,
		       'force-factored-filenames' => \$_FORCE_FACTORED_FILENAMES,
		       'dictionary=s' => \$_DICTIONARY,
		       'eppex:s' => \$_EPPEX,
           'additional-ini=s' => \$_ADDITIONAL_INI
               );

if ($_HELP) {
    print "Train Phrase Model

Steps: (--first-step to --last-step)
(1) prepare corpus
(2) run GIZA
(3) align words
(4) learn lexical translation
(5) extract phrases
(6) score phrases
(7) learn reordering model
(8) learn generation model
(9) create decoder config file

For more, please check manual or contact koehn\@inf.ed.ac.uk\n";
  exit(1);
}

$_HIERARCHICAL = 1 if $_SOURCE_SYNTAX || $_TARGET_SYNTAX;
$_XML = 1 if $_SOURCE_SYNTAX || $_TARGET_SYNTAX;
my $___FACTOR_DELIMITER = $_FACTOR_DELIMITER;
$___FACTOR_DELIMITER = '|' unless ($_FACTOR_DELIMITER);

print STDERR "Using SCRIPTS_ROOTDIR: $SCRIPTS_ROOTDIR\n";

# Setting the steps to perform
my $___VERBOSE = 0;
my $___FIRST_STEP = 1;
my $___LAST_STEP = 9;
$___VERBOSE = $_VERBOSE if $_VERBOSE;
$___FIRST_STEP = $_FIRST_STEP if $_FIRST_STEP;
$___LAST_STEP =  $_LAST_STEP  if $_LAST_STEP;
my $___DO_STEPS = $___FIRST_STEP."-".$___LAST_STEP;
$___DO_STEPS = $_DO_STEPS if $_DO_STEPS;
my @STEPS = (0,0,0,0,0,0,0,0,0);

my @step_conf = split(',',$___DO_STEPS);
my ($f,$l);
foreach my $step (@step_conf) {
  if ($step =~ /^(\d)$/) {
    $f = $1;
    $l = $1;
  }
  elsif ($step =~ /^(\d)-(\d)$/) {
    $f = $1;
    $l = $2;
  }
  else {
    die ("Malformed argument to --do-steps");
  }
  die("Only steps between 1 and 9 can be used") if ($f < 1 || $l > 9);
  die("The first step must be smaller than the last step") if ($f > $l);
	
  for (my $i=$f; $i<=$l; $i++) {
    $STEPS[$i] = 1;
  }
}



# supporting binaries from other packages
my $MGIZA_MERGE_ALIGN = "$BINDIR/merge_alignment.py";
my $GIZA;
if(!defined $_MGIZA ){
	$GIZA = "$BINDIR/GIZA++";
	print STDERR "Using single-thread GIZA\n";
}
else {
    $GIZA = "$BINDIR/mgizapp";
	print STDERR "Using multi-thread GIZA\n";	
    if (!defined($_MGIZA_CPUS)) {
        $_MGIZA_CPUS=4;
    }
    die("ERROR: Cannot find $MGIZA_MERGE_ALIGN") unless (-x $MGIZA_MERGE_ALIGN);
}

my $SNT2COOC = "$BINDIR/snt2cooc.out"; 
my $MKCLS = "$BINDIR/mkcls";

# supporting scripts/binaries from this package
my $PHRASE_EXTRACT = "$SCRIPTS_ROOTDIR/training/phrase-extract/extract";
my $RULE_EXTRACT;
if (defined($_GHKM)) {
  $RULE_EXTRACT = "$SCRIPTS_ROOTDIR/training/phrase-extract/extract-ghkm/tools/extract-ghkm";
}
else {
  $RULE_EXTRACT = "$SCRIPTS_ROOTDIR/training/phrase-extract/extract-rules";
}
my $LEXICAL_REO_SCORER = "$SCRIPTS_ROOTDIR/training/lexical-reordering/score";
my $MEMSCORE = "$SCRIPTS_ROOTDIR/training/memscore/memscore";
my $EPPEX = "$SCRIPTS_ROOTDIR/training/eppex/eppex";
my $SYMAL = "$SCRIPTS_ROOTDIR/training/symal/symal";
my $GIZA2BAL = "$SCRIPTS_ROOTDIR/training/symal/giza2bal.pl";
my $PHRASE_SCORE = "$SCRIPTS_ROOTDIR/training/phrase-extract/score";
my $PHRASE_CONSOLIDATE = "$SCRIPTS_ROOTDIR/training/phrase-extract/consolidate";

# utilities
my $ZCAT = "gzip -cd";
my $BZCAT = "bzcat";

# do a sanity check to make sure we can find the necessary binaries since
# these are not installed by default
# not needed if we start after step 2
die("ERROR: Cannot find mkcls, GIZA++, & snt2cooc.out in $BINDIR.\nDid you install this script using 'make release'?") unless ((!$STEPS[2]) ||
                                       (-x $GIZA && -x $SNT2COOC && -x $MKCLS));

# set varibles to defaults or from options
my $___ROOT_DIR = ".";
$___ROOT_DIR = $_ROOT_DIR if $_ROOT_DIR;
my $___CORPUS_DIR  = $___ROOT_DIR."/corpus";
$___CORPUS_DIR = $_CORPUS_DIR if $_CORPUS_DIR;
die("ERROR: use --corpus to specify corpus") unless $_CORPUS || !($STEPS[1] || $STEPS[4] || $STEPS[5] || $STEPS[8]);
my $___CORPUS      = $_CORPUS;

# check the final-alignment-model switch
my $___FINAL_ALIGNMENT_MODEL = undef;
$___FINAL_ALIGNMENT_MODEL = 'hmm' if $_HMM_ALIGN;
$___FINAL_ALIGNMENT_MODEL = $_FINAL_ALIGNMENT_MODEL if $_FINAL_ALIGNMENT_MODEL;

die("ERROR: --final-alignment-model can be set to '1', '2', 'hmm', '3', '4' or '5'")
	unless (!defined($___FINAL_ALIGNMENT_MODEL) or $___FINAL_ALIGNMENT_MODEL =~ /^(1|2|hmm|3|4|5)$/);

my $___GIZA_EXTENSION = 'A3.final';
if(defined $___FINAL_ALIGNMENT_MODEL) {
    $___GIZA_EXTENSION = 'A1.5' if $___FINAL_ALIGNMENT_MODEL eq '1';
    $___GIZA_EXTENSION = 'A2.5' if $___FINAL_ALIGNMENT_MODEL eq '2';
    $___GIZA_EXTENSION = 'Ahmm.5' if $___FINAL_ALIGNMENT_MODEL eq 'hmm';
}
$___GIZA_EXTENSION = $_GIZA_EXTENSION if $_GIZA_EXTENSION;

my $___CORPUS_COMPRESSION = '';
if ($_CORPUS_COMPRESSION) {
  $___CORPUS_COMPRESSION = ".$_CORPUS_COMPRESSION";
}

# foreign/English language extension
die("ERROR: use --f to specify foreign language") unless $_F;
die("ERROR: use --e to specify English language") unless $_E;
my $___F = $_F;
my $___E = $_E;

# vocabulary files in corpus dir
my $___VCB_E = $___CORPUS_DIR."/".$___E.".vcb";
my $___VCB_F = $___CORPUS_DIR."/".$___F.".vcb";

# GIZA generated files
my $___GIZA = $___ROOT_DIR."/giza";
my $___GIZA_E2F = $___GIZA.".".$___E."-".$___F;
my $___GIZA_F2E = $___GIZA.".".$___F."-".$___E;
$___GIZA_E2F = $_GIZA_E2F if $_GIZA_E2F;
$___GIZA_F2E = $_GIZA_F2E if $_GIZA_F2E;
my $___GIZA_OPTION = "";
$___GIZA_OPTION = $_GIZA_OPTION if $_GIZA_OPTION;

# alignment heuristic
my $___ALIGNMENT = "grow-diag-final";
$___ALIGNMENT = $_ALIGNMENT if $_ALIGNMENT;
my $___NOTE_ALIGNMENT_DROPS = 1;


# model dir and alignment/extract file
my $___MODEL_DIR = $___ROOT_DIR."/model";
$___MODEL_DIR = $_MODEL_DIR if $_MODEL_DIR;
my $___ALIGNMENT_FILE = "$___MODEL_DIR/aligned";
$___ALIGNMENT_FILE = $_ALIGNMENT_FILE if $_ALIGNMENT_FILE;
my $___ALIGNMENT_STEM = $___ALIGNMENT_FILE;
$___ALIGNMENT_STEM = $_ALIGNMENT_STEM if $_ALIGNMENT_STEM;
my $___EXTRACT_FILE = $___MODEL_DIR."/extract";
$___EXTRACT_FILE = $_EXTRACT_FILE if $_EXTRACT_FILE;
my $___GLUE_GRAMMAR_FILE = $___MODEL_DIR."/glue-grammar";
$___GLUE_GRAMMAR_FILE = $_GLUE_GRAMMAR_FILE if $_GLUE_GRAMMAR_FILE;

my $___CONFIG = $___MODEL_DIR."/moses.ini";
$___CONFIG = $_CONFIG if $_CONFIG;

my $___DONT_ZIP = 0; 
$_DONT_ZIP = $___DONT_ZIP unless $___DONT_ZIP;

my $___TEMP_DIR = $___MODEL_DIR;
$___TEMP_DIR = $_TEMP_DIR if $_TEMP_DIR;

my $__SORT_BUFFER_SIZE = "";
$__SORT_BUFFER_SIZE = "-S $_SORT_BUFFER_SIZE" if $_SORT_BUFFER_SIZE;

my $___CONTINUE = 0; 
$___CONTINUE = $_CONTINUE if $_CONTINUE;

my $___MAX_PHRASE_LENGTH = "7";
my $___LEXICAL_WEIGHTING = 1;
my $___LEXICAL_FILE = $___MODEL_DIR."/lex";
$___MAX_PHRASE_LENGTH = $_MAX_PHRASE_LENGTH if $_MAX_PHRASE_LENGTH;
$___LEXICAL_WEIGHTING = 0 if $_NO_LEXICAL_WEIGHTING;
$___LEXICAL_FILE = $_LEXICAL_FILE if $_LEXICAL_FILE;

my $___PHRASE_SCORER = "phrase-extract";
$___PHRASE_SCORER = "memscore" if defined $_MEMSCORE;
my $___MEMSCORE_OPTIONS = "-s ml -s lexweights \$LEX_E2F -r ml -r lexweights \$LEX_F2E -s const 2.718";
$___MEMSCORE_OPTIONS = $_MEMSCORE if $_MEMSCORE;


my @___LM = ();
if ($STEPS[9]) {
  die "ERROR: use --lm factor:order:filename to specify at least one language model"
    if scalar @_LM == 0;
  foreach my $lm (@_LM) {
    my $type = 0; # default to srilm
    my ($f, $order, $filename);
    ($f, $order, $filename, $type) = split /:/, $lm, 4;
    die "ERROR: Wrong format of --lm. Expected: --lm factor:order:filename"
      if $f !~ /^[0-9,]+$/ || $order !~ /^[0-9]+$/ || !defined $filename;
    die "ERROR: Filename is not absolute: $filename"
      unless file_name_is_absolute $filename;
    die "ERROR: Language model file not found or empty: $filename"
      if ! -s $filename;
    push @___LM, [ $f, $order, $filename, $type ];
  }
}

my $___PARTS = 1;
$___PARTS = $_PARTS if $_PARTS;

my $___DIRECTION = 0;
$___DIRECTION = $_DIRECTION if $_DIRECTION;

# don't fork
my $___NOFORK = !defined $_PARALLEL;

my $___ONLY_PRINT_GIZA = 0;
$___ONLY_PRINT_GIZA = 1 if $_ONLY_PRINT_GIZA;

# Reordering model (esp. lexicalized)
my $___REORDERING = "distance";
$___REORDERING = $_REORDERING if $_REORDERING;
my $___REORDERING_SMOOTH = 0.5;
$___REORDERING_SMOOTH = $_REORDERING_SMOOTH if $_REORDERING_SMOOTH;
my @REORDERING_MODELS;
my $REORDERING_LEXICAL = 0; # flag for building lexicalized reordering models
my %REORDERING_MODEL_TYPES = ();

my $___MAX_LEXICAL_REORDERING = 0;
$___MAX_LEXICAL_REORDERING = 1 if $_MAX_LEXICAL_REORDERING;

my $model_num = 0;

foreach my $r (split(/\,/,$___REORDERING)) {

   #Don't do anything for distance models
   next if ($r eq "distance");

   #change some config string options, to be backward compatible
   $r =~ s/orientation/msd/;
   $r =~ s/unidirectional/backward/;
   #set default values
   push @REORDERING_MODELS, {};
   $REORDERING_MODELS[$model_num]{"dir"} = "backward";   
   $REORDERING_MODELS[$model_num]{"type"} = "wbe";
   $REORDERING_MODELS[$model_num]{"collapse"} = "allff";

   #handle the options set in the config string
   foreach my $reoconf (split(/\-/,$r)) {
      if ($reoconf =~ /^((msd)|(mslr)|(monotonicity)|(leftright))/) { 
        $REORDERING_MODELS[$model_num]{"orient"} = $reoconf;
        $REORDERING_LEXICAL = 1;
      }
      elsif ($reoconf =~ /^((bidirectional)|(backward)|(forward))/) {
        $REORDERING_MODELS[$model_num]{"dir"} = $reoconf;
      }
      elsif ($reoconf =~ /^((fe)|(f))/) {
        $REORDERING_MODELS[$model_num]{"lang"} = $reoconf;
      }
      elsif ($reoconf =~ /^((hier)|(phrase)|(wbe))/) {
        $REORDERING_MODELS[$model_num]{"type"} = $reoconf;
      }
      elsif ($reoconf =~ /^((collapseff)|(allff))/) {
        $REORDERING_MODELS[$model_num]{"collapse"} = $reoconf;
      }
      else {
        print STDERR "unknown type in reordering model config string: \"$reoconf\" in $r\n";
        exit(1);
      }
  }


  #check that the required attributes are given
  if (!defined($REORDERING_MODELS[$model_num]{"type"})) {
     print STDERR "you have to give the type of the reordering models (mslr, msd, monotonicity or leftright); it is not done in $r\n";
     exit(1);
  }

  if (!defined($REORDERING_MODELS[$model_num]{"lang"})) {
     print STDERR "you have specify which languages to condition on for lexical reordering (f or fe); it is not done in $r\n";
     exit(1);
  }

  #fix the all-string
  $REORDERING_MODELS[$model_num]{"filename"} = $REORDERING_MODELS[$model_num]{"type"}."-".$REORDERING_MODELS[$model_num]{"orient"}.'-'.
                                               $REORDERING_MODELS[$model_num]{"dir"}."-".$REORDERING_MODELS[$model_num]{"lang"};
  $REORDERING_MODELS[$model_num]{"config"} = $REORDERING_MODELS[$model_num]{"filename"}."-".$REORDERING_MODELS[$model_num]{"collapse"};

  # fix numfeatures
  $REORDERING_MODELS[$model_num]{"numfeatures"} = 1;
  $REORDERING_MODELS[$model_num]{"numfeatures"} = 2 if $REORDERING_MODELS[$model_num]{"dir"} eq "bidirectional";
  if ($REORDERING_MODELS[$model_num]{"collapse"} ne "collapseff") {
    if ($REORDERING_MODELS[$model_num]{"orient"} eq "msd") {
      $REORDERING_MODELS[$model_num]{"numfeatures"} *= 3;
    }
    elsif ($REORDERING_MODELS[$model_num]{"orient"} eq "mslr") {
      $REORDERING_MODELS[$model_num]{"numfeatures"} *= 4;
    }
    else {
      $REORDERING_MODELS[$model_num]{"numfeatures"} *= 2;
    }
  }

  # fix the overall model selection
  if (defined $REORDERING_MODEL_TYPES{$REORDERING_MODELS[$model_num]{"type"}}) {
     $REORDERING_MODEL_TYPES{$REORDERING_MODELS[$model_num]{"type"}} .=
        $REORDERING_MODELS[$model_num]{"orient"}."-"; 
  }
  else  {
     $REORDERING_MODEL_TYPES{$REORDERING_MODELS[$model_num]{"type"}} =
        $REORDERING_MODELS[$model_num]{"orient"};
  }
  $model_num++;
}

# pick the overall most specific model for each reordering model type
for my $mtype ( keys %REORDERING_MODEL_TYPES) {
  if ($REORDERING_MODEL_TYPES{$mtype} =~ /msd/) {
    $REORDERING_MODEL_TYPES{$mtype} = "msd"
  }
  elsif ($REORDERING_MODEL_TYPES{$mtype} =~ /monotonicity/) {
    $REORDERING_MODEL_TYPES{$mtype} = "monotonicity"
  }
  else {
    $REORDERING_MODEL_TYPES{$mtype} = "mslr"
  }
}

### Factored translation models
my $___NOT_FACTORED = !$_FORCE_FACTORED_FILENAMES;
$___NOT_FACTORED = 0 if $_INPUT_FACTOR_MAX;
my $___ALIGNMENT_FACTORS = "0-0";
$___ALIGNMENT_FACTORS = $_ALIGNMENT_FACTORS if defined($_ALIGNMENT_FACTORS);
die("ERROR: format for alignment factors is \"0-0\" or \"0,1,2-0,1\", you provided $___ALIGNMENT_FACTORS\n") if $___ALIGNMENT_FACTORS !~ /^\d+(\,\d+)*\-\d+(\,\d+)*$/;
$___NOT_FACTORED = 0 unless $___ALIGNMENT_FACTORS eq "0-0";

my $___TRANSLATION_FACTORS = undef;
$___TRANSLATION_FACTORS = "0-0" unless defined($_DECODING_STEPS); # single factor default
$___TRANSLATION_FACTORS = $_TRANSLATION_FACTORS if defined($_TRANSLATION_FACTORS);
die("ERROR: format for translation factors is \"0-0\" or \"0-0+1-1\" or \"0-0+0,1-0,1\", you provided $___TRANSLATION_FACTORS\n") 
  if defined $___TRANSLATION_FACTORS && $___TRANSLATION_FACTORS !~ /^\d+(\,\d+)*\-\d+(\,\d+)*(\+\d+(\,\d+)*\-\d+(\,\d+)*)*$/;
$___NOT_FACTORED = 0 unless $___TRANSLATION_FACTORS eq "0-0";

my $___REORDERING_FACTORS = undef;
$___REORDERING_FACTORS = "0-0" if defined($_REORDERING) && ! defined($_DECODING_STEPS); # single factor default
$___REORDERING_FACTORS = $_REORDERING_FACTORS if defined($_REORDERING_FACTORS);
die("ERROR: format for reordering factors is \"0-0\" or \"0-0+1-1\" or \"0-0+0,1-0,1\", you provided $___REORDERING_FACTORS\n") 
  if defined $___REORDERING_FACTORS && $___REORDERING_FACTORS !~ /^\d+(\,\d+)*\-\d+(\,\d+)*(\+\d+(\,\d+)*\-\d+(\,\d+)*)*$/;
$___NOT_FACTORED = 0 if defined($_REORDERING) && $___REORDERING_FACTORS ne "0-0";

my $___GENERATION_FACTORS = undef;
$___GENERATION_FACTORS = $_GENERATION_FACTORS if defined($_GENERATION_FACTORS);
die("ERROR: format for generation factors is \"0-1\" or \"0-1+0-2\" or \"0-1+0,1-1,2\", you provided $___GENERATION_FACTORS\n") 
  if defined $___GENERATION_FACTORS && $___GENERATION_FACTORS !~ /^\d+(\,\d+)*\-\d+(\,\d+)*(\+\d+(\,\d+)*\-\d+(\,\d+)*)*$/;
$___NOT_FACTORED = 0 if defined($___GENERATION_FACTORS);

my $___DECODING_STEPS = "t0";
$___DECODING_STEPS = $_DECODING_STEPS if defined($_DECODING_STEPS);
die("ERROR: format for decoding steps is \"t0,g0,t1,g1:t2\", you provided $___DECODING_STEPS\n") 
  if defined $_DECODING_STEPS && $_DECODING_STEPS !~ /^[tg]\d+([,:][tg]\d+)*$/;

### MAIN

&prepare()                 if $STEPS[1];
&run_giza()                if $STEPS[2];
&word_align()              if $STEPS[3];
&get_lexical_factored()    if $STEPS[4];
&extract_phrase_factored() if $STEPS[5];
&score_phrase_factored()   if $STEPS[6];
&get_reordering_factored() if $STEPS[7];
&get_generation_factored() if $STEPS[8];
&create_ini()              if $STEPS[9];

### (1) PREPARE CORPUS

sub prepare {
    print STDERR "(1) preparing corpus @ ".`date`;
    safesystem("mkdir -p $___CORPUS_DIR") or die("ERROR: could not create corpus dir $___CORPUS_DIR");
    
    print STDERR "(1.0) selecting factors @ ".`date`;
    my ($factor_f,$factor_e) = split(/\-/,$___ALIGNMENT_FACTORS);
    my $corpus = ($___NOT_FACTORED && !$_XML) ? $___CORPUS : $___CORPUS.".".$___ALIGNMENT_FACTORS;

    my $VCB_F, my $VCB_E;

    if ($___NOFORK) {
	if (! $___NOT_FACTORED || $_XML) {
	    &reduce_factors($___CORPUS.".".$___F,$corpus.".".$___F,$factor_f);
	    &reduce_factors($___CORPUS.".".$___E,$corpus.".".$___E,$factor_e);
	}
	
	&make_classes($corpus.".".$___F,$___VCB_F.".classes");
	&make_classes($corpus.".".$___E,$___VCB_E.".classes");
	
	$VCB_F = &get_vocabulary($corpus.".".$___F,$___VCB_F);
	$VCB_E = &get_vocabulary($corpus.".".$___E,$___VCB_E);
	
	&numberize_txt_file($VCB_F,$corpus.".".$___F,
			    $VCB_E,$corpus.".".$___E,
			    $___CORPUS_DIR."/$___F-$___E-int-train.snt");
	
	&numberize_txt_file($VCB_E,$corpus.".".$___E,
			    $VCB_F,$corpus.".".$___F,
			    $___CORPUS_DIR."/$___E-$___F-int-train.snt");
    } 
    else {
	print "Forking...\n";
	if (! $___NOT_FACTORED || $_XML) {
	    my $pid = fork();
	    die "ERROR: couldn't fork" unless defined $pid;
	    if (!$pid) {
		&reduce_factors($___CORPUS.".".$___F,$corpus.".".$___F,$factor_f);
		exit 0;
	    } 
	    else {
		&reduce_factors($___CORPUS.".".$___E,$corpus.".".$___E,$factor_e);
	    }
	    printf "Waiting for second reduce_factors process...\n";
	    waitpid($pid, 0);
	}
	my $pid = fork();
	die "ERROR: couldn't fork" unless defined $pid;
	if (!$pid) {
	    &make_classes($corpus.".".$___F,$___VCB_F.".classes");
	    exit 0;
	} # parent
	my $pid2 = fork();
	die "ERROR: couldn't fork again" unless defined $pid2;
	if (!$pid2) { #child
	    &make_classes($corpus.".".$___E,$___VCB_E.".classes");
	    exit 0;
	}
	
	$VCB_F = &get_vocabulary($corpus.".".$___F,$___VCB_F);
	$VCB_E = &get_vocabulary($corpus.".".$___E,$___VCB_E);
	
	&numberize_txt_file($VCB_F,$corpus.".".$___F,
			    $VCB_E,$corpus.".".$___E,
			    $___CORPUS_DIR."/$___F-$___E-int-train.snt");
	
	&numberize_txt_file($VCB_E,$corpus.".".$___E,
			    $VCB_F,$corpus.".".$___F,
			    $___CORPUS_DIR."/$___E-$___F-int-train.snt");
	printf "Waiting for mkcls processes to finish...\n";
	waitpid($pid2, 0);
	waitpid($pid, 0);
    }

	if (defined $_DICTIONARY)
	{
		my $dict= &make_dicts_files($_DICTIONARY, $VCB_F,$VCB_E,
                                    $___CORPUS_DIR."/gizadict.$___E-$___F",
                                    $___CORPUS_DIR."/gizadict.$___F-$___E");
		if (not $dict)
		{
			print STDERR "WARNING: empty dictionary\n";
			undef $_DICTIONARY;
		}
	}
}

sub open_compressed {
    my ($file) = @_;
    print "FILE: $file\n";

    # add extensions, if necessary
    $file = $file.".bz2" if ! -e $file && -e $file.".bz2";
    $file = $file.".gz"  if ! -e $file && -e $file.".gz";
   
    # pipe zipped, if necessary
    return "$BZCAT $file|" if $file =~ /\.bz2$/;
    return "$ZCAT $file|"  if $file =~ /\.gz$/;    
    return $file;
}

sub reduce_factors {
    my ($full,$reduced,$factors) = @_;

    # my %INCLUDE;
    # foreach my $factor (split(/,/,$factors)) {
	# $INCLUDE{$factor} = 1;
    # }
    my @INCLUDE = sort {$a <=> $b} split(/,/,$factors);

    print STDERR "(1.0.5) reducing factors to produce $reduced  @ ".`date`;
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

    unless ($_XML) {
        # peek at input, to check if we are asked to produce exactly the
        # available factors
        my $inh = open_or_zcat($full);
        my $firstline = <$inh>;
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
          safesystem("ln -s $realfull $reduced")
            or die "Failed to create symlink $realfull -> $reduced";
          return;
        }
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
	s/<\S[^>]*>/ /g if $_XML; # remove xml
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
	    # for(my $factor=0;$factor<=$#FACTOR;$factor++) {
		# next unless defined($INCLUDE{$factor});
		# print OUT "|" unless $first_factor;
		# $first_factor = 0;
		# print OUT $FACTOR[$factor];
	    # }
	} 
	print OUT "\n";
    }
    print STDERR "\n";
    close(OUT);
    close(IN);
    `rm -f $reduced.lock`;
}

sub make_classes {
    my ($corpus,$classes) = @_;
    my $cmd = "$MKCLS -c50 -n2 -p$corpus -V$classes opt";
    print STDERR "(1.1) running mkcls  @ ".`date`."$cmd\n";
    if (-e $classes) {
        print STDERR "  $classes already in place, reusing\n";
        return;
    }
    safesystem("$cmd"); # ignoring the wrong exit code from mkcls (not dying)
}

sub get_vocabulary {
    return unless $___LEXICAL_WEIGHTING;
    my($corpus,$vcb) = @_;
    print STDERR "(1.2) creating vcb file $vcb @ ".`date`;
    
    my %WORD;
    open(TXT,$corpus) or die "ERROR: Can't read $corpus";
    while(<TXT>) {
	chop;
	foreach (split) { $WORD{$_}++; }
    }
    close(TXT);
    
    my @NUM;
    foreach my $word (keys %WORD) {
	my $vcb_with_number = sprintf("%07d %s",$WORD{$word},$word);
	push @NUM,$vcb_with_number;
    }
    
    my %VCB;
    open(VCB,">$vcb") or die "ERROR: Can't write $vcb";
    print VCB "1\tUNK\t0\n";
    my $id=2;
    foreach (reverse sort @NUM) {
	my($count,$word) = split;
	printf VCB "%d\t%s\t%d\n",$id,$word,$count;
	$VCB{$word} = $id;
	$id++;
    }
    close(VCB);
    
    return \%VCB;
}

sub make_dicts_files {
    my ($dictfile,$VCB_SRC,$VCB_TGT,$outfile1, $outfile2) = @_;
    my %numberized_dict;
    print STDERR "(1.3) numberizing dictionaries $outfile1 and $outfile2 @ ".`date`;
    if ((-e $outfile1) && (-e $outfile2)) {
        print STDERR "  dictionary files already in place, reusing\n";
        return;
    }
    open(DICT,$dictfile) or die "ERROR: Can't read $dictfile";
	open(OUT1,">$outfile1") or die "ERROR: Can't write $outfile1";
	open(OUT2,">$outfile2") or die "ERROR: Can't write $outfile2";
    while(my $line = <DICT>) {
		my $src, my $tgt;
		($src, $tgt) = split(/\s+/,$line);
		chomp($tgt); chomp($src);
		if ((not defined($$VCB_TGT{$tgt})) || (not defined($$VCB_SRC{$src})))
		{
			print STDERR "Warning: unknown word in dictionary: $src <=> $tgt\n";
			next;
		}
		$numberized_dict{int($$VCB_TGT{$tgt})} = int($$VCB_SRC{$src}) ;
	}
    close(DICT);
	my @items = sort {$a <=> $b} keys %numberized_dict;
	if (scalar(@items) == 0) { return 0; } 
	foreach my $key (@items)
	{
		print OUT1 "$key $numberized_dict{$key}\n";
		print OUT2 "$numberized_dict{$key} $key\n";
	}
    close(OUT);
	return 1;
}


sub numberize_txt_file {
    my ($VCB_DE,$in_de,$VCB_EN,$in_en,$out) = @_;
    my %OUT;
    print STDERR "(1.3) numberizing corpus $out @ ".`date`;
    if (-e $out) {
        print STDERR "  $out already in place, reusing\n";
        return;
    }
    open(IN_DE,$in_de) or die "ERROR: Can't read $in_de";
    open(IN_EN,$in_en) or die "ERROR: Can't read $in_en";
    open(OUT,">$out") or die "ERROR: Can't write $out";
    while(my $de = <IN_DE>) {
	my $en = <IN_EN>;
	print OUT "1\n";
	print OUT &numberize_line($VCB_EN,$en);
	print OUT &numberize_line($VCB_DE,$de);
    }
    close(IN_DE);
    close(IN_EN);
    close(OUT);
}

sub numberize_line {
    my ($VCB,$txt) = @_;
    chomp($txt);
    my $out = "";
    my $not_first = 0;
    foreach (split(/ /,$txt)) { 
	next if $_ eq '';
	$out .= " " if $not_first++;
	print STDERR "Unknown word '$_'\n" unless defined($$VCB{$_});
	$out .= $$VCB{$_};
    }
    return $out."\n";
}

### (2) RUN GIZA

sub run_giza {
    return &run_giza_on_parts if $___PARTS>1;

    print STDERR "(2) running giza @ ".`date`;
    if ($___DIRECTION == 1 || $___DIRECTION == 2 || $___NOFORK) {
	&run_single_giza($___GIZA_F2E,$___E,$___F,
		     $___VCB_E,$___VCB_F,
		     $___CORPUS_DIR."/$___F-$___E-int-train.snt")
	    unless $___DIRECTION == 2;
	&run_single_giza($___GIZA_E2F,$___F,$___E,
		     $___VCB_F,$___VCB_E,
		     $___CORPUS_DIR."/$___E-$___F-int-train.snt")
	    unless $___DIRECTION == 1;
    } else {
	my $pid = fork();
	if (!defined $pid) {
	    die "ERROR: Failed to fork";
	}
	if (!$pid) { # i'm the child
	    &run_single_giza($___GIZA_F2E,$___E,$___F,
                     $___VCB_E,$___VCB_F,
                     $___CORPUS_DIR."/$___F-$___E-int-train.snt");
	    exit 0; # child exits
	} else { #i'm the parent
	    &run_single_giza($___GIZA_E2F,$___F,$___E,
                     $___VCB_F,$___VCB_E,
                     $___CORPUS_DIR."/$___E-$___F-int-train.snt");
	}
	printf "Waiting for second GIZA process...\n";
	waitpid($pid, 0);
    }
}

sub run_giza_on_parts {
    print STDERR "(2) running giza on $___PARTS cooc parts @ ".`date`;
    my $size = `cat $___CORPUS_DIR/$___F-$___E-int-train.snt | wc -l`;
    die "ERROR: Failed to get number of lines in $___CORPUS_DIR/$___F-$___E-int-train.snt"
      if $size == 0;
    
    if ($___DIRECTION == 1 || $___DIRECTION == 2 || $___NOFORK) {
	&run_single_giza_on_parts($___GIZA_F2E,$___E,$___F,
			      $___VCB_E,$___VCB_F,
			      $___CORPUS_DIR."/$___F-$___E-int-train.snt",$size)
   	    unless $___DIRECTION == 2;
 
	&run_single_giza_on_parts($___GIZA_E2F,$___F,$___E,
			      $___VCB_F,$___VCB_E,
			      $___CORPUS_DIR."/$___E-$___F-int-train.snt",$size)
   	    unless $___DIRECTION == 1;
    } else {
	my $pid = fork();
	if (!defined $pid) {
	    die "ERROR: Failed to fork";
	}
	if (!$pid) { # i'm the child
	    &run_single_giza_on_parts($___GIZA_F2E,$___E,$___F,
			      $___VCB_E,$___VCB_F,
			      $___CORPUS_DIR."/$___F-$___E-int-train.snt",$size);
	    exit 0; # child exits
	} else { #i'm the parent
	    &run_single_giza_on_parts($___GIZA_E2F,$___F,$___E,
			      $___VCB_F,$___VCB_E,
			      $___CORPUS_DIR."/$___E-$___F-int-train.snt",$size);
	}
	printf "Waiting for second GIZA process...\n";
	waitpid($pid, 0);
    }
}

sub run_single_giza_on_parts {
    my($dir,$e,$f,$vcb_e,$vcb_f,$train,$size) = @_;
    
    my $part = 0;

    # break up training data into parts
    open(SNT,$train) or die "ERROR: Can't read $train";
    { 
	my $i=0;
	while(<SNT>) {
	    $i++;
	    if ($i%3==1 && $part < ($___PARTS*$i)/$size && $part<$___PARTS) {
		close(PART) if $part;
		$part++;
		safesystem("mkdir -p $___CORPUS_DIR/part$part") or die("ERROR: could not create $___CORPUS_DIR/part$part");
		open(PART,">$___CORPUS_DIR/part$part/$f-$e-int-train.snt")
                   or die "ERROR: Can't write $___CORPUS_DIR/part$part/$f-$e-int-train.snt";
	    }
	    print PART $_;
	}
    }
    close(PART);
    close(SNT);

    # run snt2cooc in parts
    for(my $i=1;$i<=$___PARTS;$i++) {
	&run_single_snt2cooc("$dir/part$i",$e,$f,$vcb_e,$vcb_f,"$___CORPUS_DIR/part$i/$f-$e-int-train.snt");
    }

    # merge parts
    open(COOC,">$dir/$f-$e.cooc") or die "ERROR: Can't write $dir/$f-$e.cooc";
    my(@PF,@CURRENT);
    for(my $i=1;$i<=$___PARTS;$i++) {
	open($PF[$i],"$dir/part$i/$f-$e.cooc")or die "ERROR: Can't read $dir/part$i/$f-$e.cooc";
	my $pf = $PF[$i];
	$CURRENT[$i] = <$pf>;
	chop($CURRENT[$i]) if $CURRENT[$i];
    }

    while(1) {
	my ($min1,$min2) = (1e20,1e20);
	for(my $i=1;$i<=$___PARTS;$i++) {
	    next unless $CURRENT[$i];
	    my ($w1,$w2) = split(/ /,$CURRENT[$i]);
	    if ($w1 < $min1 || ($w1 == $min1 && $w2 < $min2)) {
		$min1 = $w1;
		$min2 = $w2;
	    }
	}
	last if $min1 == 1e20;
	print COOC "$min1 $min2\n";
	for(my $i=1;$i<=$___PARTS;$i++) {
	    next unless $CURRENT[$i];
	    my ($w1,$w2) = split(/ /,$CURRENT[$i]);
	    if ($w1 == $min1 && $w2 == $min2) {
		my $pf = $PF[$i];
		$CURRENT[$i] = <$pf>;
		chop($CURRENT[$i]) if $CURRENT[$i];
	    }
	}	
    }
    for(my $i=1;$i<=$___PARTS;$i++) {
	close($PF[$i]);
    }
    close(COOC);

    # run giza
    &run_single_giza($dir,$e,$f,$vcb_e,$vcb_f,$train);
}

sub run_single_giza {
    my($dir,$e,$f,$vcb_e,$vcb_f,$train) = @_;

    my %GizaDefaultOptions = 
	(p0 => .999 ,
	 m1 => 5 , 
	 m2 => 0 , 
	 m3 => 3 , 
	 m4 => 3 , 
	 o => "giza" ,
	 nodumps => 1 ,
	 onlyaldumps => 1 ,
	 nsmooth => 4 , 
         model1dumpfrequency => 1,
	 model4smoothfactor => 0.4 ,
	 t => $vcb_f,
         s => $vcb_e,
	 c => $train,
	 CoocurrenceFile => "$dir/$f-$e.cooc",
	 o => "$dir/$f-$e");
	
	if (defined $_DICTIONARY)
	{ $GizaDefaultOptions{d} = $___CORPUS_DIR."/gizadict.$f-$e"; }
	
	# 5 Giza threads
	if (defined $_MGIZA){ $GizaDefaultOptions{"ncpus"} = $_MGIZA_CPUS; }

    if ($_HMM_ALIGN) {
       $GizaDefaultOptions{m3} = 0;
       $GizaDefaultOptions{m4} = 0;
       $GizaDefaultOptions{hmmiterations} = 5;
       $GizaDefaultOptions{hmmdumpfrequency} = 5;
       $GizaDefaultOptions{nodumps} = 0;
    }

    if ($___FINAL_ALIGNMENT_MODEL) {
        $GizaDefaultOptions{nodumps} =               ($___FINAL_ALIGNMENT_MODEL =~ /^[345]$/)? 1: 0;
        $GizaDefaultOptions{model345dumpfrequency} = 0;
        
        $GizaDefaultOptions{model1dumpfrequency} =   ($___FINAL_ALIGNMENT_MODEL eq '1')? 5: 0;
        
        $GizaDefaultOptions{m2} =                    ($___FINAL_ALIGNMENT_MODEL eq '2')? 5: 0;
        $GizaDefaultOptions{model2dumpfrequency} =   ($___FINAL_ALIGNMENT_MODEL eq '2')? 5: 0;
        
        $GizaDefaultOptions{hmmiterations} =         ($___FINAL_ALIGNMENT_MODEL =~ /^(hmm|[345])$/)? 5: 0;
        $GizaDefaultOptions{hmmdumpfrequency} =      ($___FINAL_ALIGNMENT_MODEL eq 'hmm')? 5: 0;
        
        $GizaDefaultOptions{m3} =                    ($___FINAL_ALIGNMENT_MODEL =~ /^[345]$/)? 3: 0;
        $GizaDefaultOptions{m4} =                    ($___FINAL_ALIGNMENT_MODEL =~ /^[45]$/)? 3: 0;
        $GizaDefaultOptions{m5} =                    ($___FINAL_ALIGNMENT_MODEL eq '5')? 3: 0;
    }

    if ($___GIZA_OPTION) {
	foreach (split(/[ ,]+/,$___GIZA_OPTION)) {
	    my ($option,$value) = split(/=/,$_,2);
	    $GizaDefaultOptions{$option} = $value;
	}
    }

    my $GizaOptions;
    foreach my $option (sort keys %GizaDefaultOptions){
	my $value = $GizaDefaultOptions{$option} ;
	$GizaOptions .= " -$option $value" ;
    }
    
    &run_single_snt2cooc($dir,$e,$f,$vcb_e,$vcb_f,$train) if $___PARTS == 1;

    print STDERR "(2.1b) running giza $f-$e @ ".`date`."$GIZA $GizaOptions\n";


    if (-e "$dir/$f-$e.$___GIZA_EXTENSION.gz") {
      print "  $dir/$f-$e.$___GIZA_EXTENSION.gz seems finished, reusing.\n";
      return;
    }
    print "$GIZA $GizaOptions\n";
    return if  $___ONLY_PRINT_GIZA;
    safesystem("$GIZA $GizaOptions");
 
	if (defined $_MGIZA and (!defined $___FINAL_ALIGNMENT_MODEL or $___FINAL_ALIGNMENT_MODEL ne '2')){
		print STDERR "Merging $___GIZA_EXTENSION.part\* tables\n";
		safesystem("$MGIZA_MERGE_ALIGN  $dir/$f-$e.$___GIZA_EXTENSION.part*>$dir/$f-$e.$___GIZA_EXTENSION");
		#system("rm -f $dir/$f-$e/*.part*");
	}


    die "ERROR: Giza did not produce the output file $dir/$f-$e.$___GIZA_EXTENSION. Is your corpus clean (reasonably-sized sentences)?"
      if ! -e "$dir/$f-$e.$___GIZA_EXTENSION";
    safesystem("rm -f $dir/$f-$e.$___GIZA_EXTENSION.gz") or die;
    safesystem("gzip $dir/$f-$e.$___GIZA_EXTENSION") or die;
}

sub run_single_snt2cooc {
    my($dir,$e,$f,$vcb_e,$vcb_f,$train) = @_;
    print STDERR "(2.1a) running snt2cooc $f-$e @ ".`date`."\n";
    safesystem("mkdir -p $dir") or die("ERROR");
    print "$SNT2COOC $vcb_e $vcb_f $train > $dir/$f-$e.cooc\n";
    safesystem("$SNT2COOC $vcb_e $vcb_f $train > $dir/$f-$e.cooc") or die("ERROR");
}

### (3) CREATE WORD ALIGNMENT FROM GIZA ALIGNMENTS

sub word_align {

    print STDERR "(3) generate word alignment @ ".`date`;
    my (%WORD_TRANSLATION,%TOTAL_FOREIGN,%TOTAL_ENGLISH);
    print STDERR "Combining forward and inverted alignment from files:\n";
    print STDERR "  $___GIZA_F2E/$___F-$___E.$___GIZA_EXTENSION.{bz2,gz}\n";
    print STDERR "  $___GIZA_E2F/$___E-$___F.$___GIZA_EXTENSION.{bz2,gz}\n";

    ### build arguments for giza2bal.pl
    my($__ALIGNMENT_CMD,$__ALIGNMENT_INV_CMD);
    
    if (-e "$___GIZA_F2E/$___F-$___E.$___GIZA_EXTENSION.bz2"){
      $__ALIGNMENT_CMD="\"$BZCAT $___GIZA_F2E/$___F-$___E.$___GIZA_EXTENSION.bz2\"";
    } elsif (-e "$___GIZA_F2E/$___F-$___E.$___GIZA_EXTENSION.gz") {
      $__ALIGNMENT_CMD="\"$ZCAT $___GIZA_F2E/$___F-$___E.$___GIZA_EXTENSION.gz\"";
    } else {
      die "ERROR: Can't read $___GIZA_F2E/$___F-$___E.$___GIZA_EXTENSION.{bz2,gz}\n";
    }
  
    if ( -e "$___GIZA_E2F/$___E-$___F.$___GIZA_EXTENSION.bz2"){
      $__ALIGNMENT_INV_CMD="\"$BZCAT $___GIZA_E2F/$___E-$___F.$___GIZA_EXTENSION.bz2\"";
    }elsif (-e "$___GIZA_E2F/$___E-$___F.$___GIZA_EXTENSION.gz"){
      $__ALIGNMENT_INV_CMD="\"$ZCAT $___GIZA_E2F/$___E-$___F.$___GIZA_EXTENSION.gz\"";
    }else{
      die "ERROR: Can't read $___GIZA_E2F/$___E-$___F.$___GIZA_EXTENSION.{bz2,gz}\n\n";
    }
    
   safesystem("mkdir -p $___MODEL_DIR") or die("ERROR: could not create dir $___MODEL_DIR");
   
   #build arguments for symal
    my($__symal_a)="";
    $__symal_a="union" if $___ALIGNMENT eq 'union';
    $__symal_a="intersect" if $___ALIGNMENT=~ /intersect/;
    $__symal_a="grow" if $___ALIGNMENT=~ /grow/;
    $__symal_a="srctotgt" if $___ALIGNMENT=~ /srctotgt/;
    $__symal_a="tgttosrc" if $___ALIGNMENT=~ /tgttosrc/;
 
    
    my($__symal_d,$__symal_f,$__symal_b);
    ($__symal_d,$__symal_f,$__symal_b)=("no","no","no");

    $__symal_d="yes" if $___ALIGNMENT=~ /diag/;
    $__symal_f="yes" if $___ALIGNMENT=~ /final/;
    $__symal_b="yes" if $___ALIGNMENT=~ /final-and/;
    
    safesystem("$GIZA2BAL -d $__ALIGNMENT_INV_CMD -i $__ALIGNMENT_CMD |".
	  "$SYMAL -alignment=\"$__symal_a\" -diagonal=\"$__symal_d\" ".
	  "-final=\"$__symal_f\" -both=\"$__symal_b\" > ".
	  "$___ALIGNMENT_FILE.$___ALIGNMENT") 
      ||
       die "ERROR: Can't generate symmetrized alignment file\n"
	
}

### (4) BUILDING LEXICAL TRANSLATION TABLE

sub get_lexical_factored {
    print STDERR "(4) generate lexical translation table $___TRANSLATION_FACTORS @ ".`date`;
    if ($___NOT_FACTORED && !$_XML) {
	&get_lexical($___CORPUS.".".$___F,
		     $___CORPUS.".".$___E,
		     $___LEXICAL_FILE);
    }
    else {
	foreach my $factor (split(/\+/,$___TRANSLATION_FACTORS)) {
	    print STDERR "(4) [$factor] generate lexical translation table @ ".`date`;
	    my ($factor_f,$factor_e) = split(/\-/,$factor);
	    &reduce_factors($___CORPUS.".".$___F,
			    $___ALIGNMENT_STEM.".".$factor_f.".".$___F,
			    $factor_f);
	    &reduce_factors($___CORPUS.".".$___E,
			    $___ALIGNMENT_STEM.".".$factor_e.".".$___E,
			    $factor_e);
	    my $lexical_file = $___LEXICAL_FILE;
	    $lexical_file .= ".".$factor if !$___NOT_FACTORED;
	    &get_lexical($___ALIGNMENT_STEM.".".$factor_f.".".$___F,
			 $___ALIGNMENT_STEM.".".$factor_e.".".$___E,
			 $lexical_file);
	}
    }
}

sub get_lexical {
    my ($alignment_file_f,$alignment_file_e,$lexical_file) = @_;
    print STDERR "($alignment_file_f,$alignment_file_e,$lexical_file)\n";
    my $alignment_file_a = $___ALIGNMENT_FILE.".".$___ALIGNMENT;

    my (%WORD_TRANSLATION,%TOTAL_FOREIGN,%TOTAL_ENGLISH);

    if (-e "$lexical_file.f2e" && -e "$lexical_file.e2f") {
      print STDERR "  reusing: $lexical_file.f2e and $lexical_file.e2f\n";
      return;
    }

    open(E,&open_compressed($alignment_file_e)) or die "ERROR: Can't read $alignment_file_e";
    open(F,&open_compressed($alignment_file_f)) or die "ERROR: Can't read $alignment_file_f";
    open(A,&open_compressed($alignment_file_a)) or die "ERROR: Can't read $alignment_file_a";

    my $alignment_id = 0;
    while(my $e = <E>) {
        if (($alignment_id++ % 1000) == 0) { print STDERR "!"; }
        chomp($e); fix_spaces(\$e);
        my @ENGLISH = split(/ /,$e);
        my $f = <F>; chomp($f); fix_spaces(\$f);
        my @FOREIGN = split(/ /,$f);
        my $a = <A>; chomp($a); fix_spaces(\$a);

        my (%FOREIGN_ALIGNED,%ENGLISH_ALIGNED);
        foreach (split(/ /,$a)) {
            my ($fi,$ei) = split(/\-/);
	    if ($fi >= scalar(@FOREIGN) || $ei >= scalar(@ENGLISH)) {
		print STDERR "alignment point ($fi,$ei) out of range (0-$#FOREIGN,0-$#ENGLISH) in line $alignment_id, ignoring\n";
	    }
	    else {
		# local counts
		$FOREIGN_ALIGNED{$fi}++;
		$ENGLISH_ALIGNED{$ei}++;
		
		# global counts
		$WORD_TRANSLATION{$FOREIGN[$fi]}{$ENGLISH[$ei]}++;
		$TOTAL_FOREIGN{$FOREIGN[$fi]}++;
		$TOTAL_ENGLISH{$ENGLISH[$ei]}++;
	    }
        }

        # unaligned words
        for(my $ei=0;$ei<scalar(@ENGLISH);$ei++) {
          next if defined($ENGLISH_ALIGNED{$ei});
          $WORD_TRANSLATION{"NULL"}{$ENGLISH[$ei]}++;
          $TOTAL_ENGLISH{$ENGLISH[$ei]}++;
          $TOTAL_FOREIGN{"NULL"}++;
        }
        for(my $fi=0;$fi<scalar(@FOREIGN);$fi++) {
          next if defined($FOREIGN_ALIGNED{$fi});
          $WORD_TRANSLATION{$FOREIGN[$fi]}{"NULL"}++;
          $TOTAL_FOREIGN{$FOREIGN[$fi]}++;
          $TOTAL_ENGLISH{"NULL"}++;
        }
    }
    print STDERR "\n";
    close(A);
    close(F);
    close(E);

    open(F2E,">$lexical_file.f2e") or die "ERROR: Can't write $lexical_file.f2e";
    open(E2F,">$lexical_file.e2f") or die "ERROR: Can't write $lexical_file.e2f";

    foreach my $f (keys %WORD_TRANSLATION) {
	foreach my $e (keys %{$WORD_TRANSLATION{$f}}) {
	    printf F2E "%s %s %.7f\n",$e,$f,$WORD_TRANSLATION{$f}{$e}/$TOTAL_FOREIGN{$f};
	    printf E2F "%s %s %.7f\n",$f,$e,$WORD_TRANSLATION{$f}{$e}/$TOTAL_ENGLISH{$e};
	}
    }
    close(E2F);
    close(F2E);
    print STDERR "Saved: $lexical_file.f2e and $lexical_file.e2f\n";
}

### (5) PHRASE EXTRACTION

sub extract_phrase_factored {
    print STDERR "(5) extract phrases @ ".`date`;
    if ($___NOT_FACTORED) {
	&extract_phrase($___CORPUS.".".$___F,
			$___CORPUS.".".$___E,
			$___EXTRACT_FILE,
			0,1,$REORDERING_LEXICAL);
    }
    else {
	my %EXTRACT_FOR_FACTOR = ();
	my $table_number = 0;
	my @FACTOR_LIST = ();
	foreach my $factor (split(/\+/,"$___TRANSLATION_FACTORS")) {
	    my $factor_key = $factor.":".&get_max_phrase_length($table_number++);
	    push @FACTOR_LIST, $factor_key;
	    $EXTRACT_FOR_FACTOR{$factor_key}{"translation"}++;
	}
	if ($REORDERING_LEXICAL) {
	    foreach my $factor (split(/\+/,"$___REORDERING_FACTORS")) {
		my $factor_key = $factor.":".&get_max_phrase_length(-1); # max
		if (!defined($EXTRACT_FOR_FACTOR{$factor_key}{"translation"})) {
		    push @FACTOR_LIST, $factor_key;	    
		}
		$EXTRACT_FOR_FACTOR{$factor_key}{"reordering"}++;
	    }
	}
	$table_number = 0;
	foreach my $factor_key (@FACTOR_LIST) {
	    my ($factor,$max_length) = split(/:/,$factor_key);
	    print STDERR "(5) [$factor] extract phrases (max length $max_length)@ ".`date`;
	    my ($factor_f,$factor_e) = split(/\-/,$factor);
	    
	    &reduce_factors($___CORPUS.".".$___F,
			    $___ALIGNMENT_STEM.".".$factor_f.".".$___F,
			    $factor_f);
	    &reduce_factors($___CORPUS.".".$___E,
			    $___ALIGNMENT_STEM.".".$factor_e.".".$___E,
			    $factor_e);
	    
	    &extract_phrase($___ALIGNMENT_STEM.".".$factor_f.".".$___F,
			    $___ALIGNMENT_STEM.".".$factor_e.".".$___E,
			    $___EXTRACT_FILE.".".$factor,
			    $table_number++,
			    defined($EXTRACT_FOR_FACTOR{$factor_key}{"translation"}),
			    defined($EXTRACT_FOR_FACTOR{$factor_key}{"reordering"}));
	}
    }
}

sub get_max_phrase_length {
    my ($table_number) = @_;
    
    # single length? that's it then
    if ($___MAX_PHRASE_LENGTH =~ /^\d+$/) {
	return $___MAX_PHRASE_LENGTH;
    }

    my $max_length = 0;
    my @max = split(/,/,$___MAX_PHRASE_LENGTH);

    # maximum of specified lengths
    if ($table_number == -1) {
	foreach (@max) {
	    $max_length = $_ if $_ > $max_length;
	}
	return $max_length;
    }

    # look up length for table 
    $max_length = $max[0]; # fallback: first specified length
    if ($#max >= $table_number) {
	$max_length = $max[$table_number];
    }
    return $max_length;
}

sub get_extract_reordering_flags {
    if ($___MAX_LEXICAL_REORDERING) {
	return " --model wbe-mslr --model phrase-mslr --model hier-mslr";
    }
    return "" unless @REORDERING_MODELS; 
    my $config_string = "";
    for my $type ( keys %REORDERING_MODEL_TYPES) {
	$config_string .= " --model $type-".$REORDERING_MODEL_TYPES{$type};
    }
    return $config_string;
}

sub extract_phrase {
    my ($alignment_file_f,$alignment_file_e,$extract_file,$table_number,$ttable_flag,$reordering_flag) = @_;
    my $alignment_file_a = $___ALIGNMENT_FILE.".".$___ALIGNMENT;
    # Make sure the corpus exists in unzipped form
    my @tempfiles = ();
    foreach my $f ($alignment_file_e, $alignment_file_f, $alignment_file_a) {
     if (! -e $f && -e $f.".gz") {
       safesystem("gunzip < $f.gz > $f") or die("Failed to gunzip corpus $f");
       push @tempfiles, "$f.gz";
     }
    }
    my $cmd;
    if ($_HIERARCHICAL)
    {
        $cmd = "$RULE_EXTRACT $alignment_file_e $alignment_file_f $alignment_file_a $extract_file";
        $cmd .= " --GlueGrammar $___GLUE_GRAMMAR_FILE" if $_GLUE_GRAMMAR;
        $cmd .= " --UnknownWordLabel $_UNKNOWN_WORD_LABEL_FILE" if $_TARGET_SYNTAX && defined($_UNKNOWN_WORD_LABEL_FILE);
        if (!defined($_GHKM)) {
          $cmd .= " --SourceSyntax" if $_SOURCE_SYNTAX;
          $cmd .= " --TargetSyntax" if $_TARGET_SYNTAX;
        }
        $cmd .= " ".$_EXTRACT_OPTIONS if defined($_EXTRACT_OPTIONS);
    }
    else
    {
		if ( $_EPPEX ) {
			# eppex sets max_phrase_length itself (as the maximum phrase length for which any Lossy Counter is defined)
      		$cmd = "$EPPEX $alignment_file_e $alignment_file_f $alignment_file_a $extract_file $_EPPEX";
		}
		else {
      my $max_length = &get_max_phrase_length($table_number);
      print "MAX $max_length $reordering_flag $table_number\n";
      $max_length = &get_max_phrase_length(-1) if $reordering_flag;

      $cmd = "$PHRASE_EXTRACT $alignment_file_e $alignment_file_f $alignment_file_a $extract_file $max_length";
		}
      if ($reordering_flag) {
        $cmd .= " orientation";
        $cmd .= get_extract_reordering_flags();
        $cmd .= " --NoTTable" if !$ttable_flag;
        $cmd .= " ".$_EXTRACT_OPTIONS if defined($_EXTRACT_OPTIONS);
      }
    }
    map { die "File not found: $_" if ! -e $_ } ($alignment_file_e, $alignment_file_f, $alignment_file_a);
    print STDERR "$cmd\n";
    safesystem("$cmd") or die "ERROR: Phrase extraction failed (missing input files?)";
    foreach my $f (@tempfiles) {
      unlink $f;
    }
    if (! $___DONT_ZIP) { 
      safesystem("gzip $extract_file.o") if -e "$extract_file.o";
      safesystem("gzip $extract_file.sid") if -e "$extract_file.sid";
      if ($ttable_flag) {
        safesystem("gzip $extract_file.inv") or die("ERROR");
        safesystem("gzip $extract_file") or die("ERROR");
      }
    }
}

### (6) PHRASE SCORING

sub score_phrase_factored {
    print STDERR "(6) score phrases @ ".`date`;
    my @SPECIFIED_TABLE = @_PHRASE_TABLE;
    if ($___NOT_FACTORED) {
	my $file = "$___MODEL_DIR/".($_HIERARCHICAL?"rule-table":"phrase-table");
	$file = shift @SPECIFIED_TABLE if scalar(@SPECIFIED_TABLE);
	&score_phrase($file,$___LEXICAL_FILE,$___EXTRACT_FILE);
    }
    else {
	foreach my $factor (split(/\+/,$___TRANSLATION_FACTORS)) {
	    print STDERR "(6) [$factor] score phrases @ ".`date`;
	    my ($factor_f,$factor_e) = split(/\-/,$factor);
	    my $file = "$___MODEL_DIR/".($_HIERARCHICAL?"rule-table":"phrase-table").".$factor";
	    $file = shift @SPECIFIED_TABLE if scalar(@SPECIFIED_TABLE);
	    &score_phrase($file,$___LEXICAL_FILE.".".$factor,$___EXTRACT_FILE.".".$factor);
	}
    }
}

sub score_phrase {
    my ($ttable_file,$lexical_file,$extract_file) = @_;

    if ($___PHRASE_SCORER eq "phrase-extract") {
        &score_phrase_phrase_extract($ttable_file,$lexical_file,$extract_file);
    } elsif ($___PHRASE_SCORER eq "memscore") {
        &score_phrase_memscore($ttable_file,$lexical_file,$extract_file);
    } else {
        die "ERROR: Unknown phrase scorer: ".$___PHRASE_SCORER;
    }
}

sub score_phrase_phrase_extract {
    my ($ttable_file,$lexical_file,$extract_file) = @_;

    # remove consolidation options
    my $ONLY_DIRECT = (defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /OnlyDirect/);
    my $PHRASE_COUNT = (!defined($_SCORE_OPTIONS) || $_SCORE_OPTIONS !~ /NoPhraseCount/);
    my $LOW_COUNT = (defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /LowCountFeature/);
    my $UNALIGNED_COUNT = (defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /UnalignedPenalty/);
    my ($UNALIGNED_FW_COUNT,$UNALIGNED_FW_F,$UNALIGNED_FW_E);
    if (defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /UnalignedFunctionWordPenalty +(\S+) +(\S+)/) {
      $UNALIGNED_FW_COUNT = 1;
      $UNALIGNED_FW_F = $1;
      $UNALIGNED_FW_E = $2;
    }
    my $GOOD_TURING = (defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /GoodTuring/);
    my $KNESER_NEY = (defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /KneserNey/);
    my $LOG_PROB = (defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /LogProb/);
    my $NEG_LOG_PROB = (defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /NegLogProb/);
    my $NO_LEX = (defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /NoLex/);
    my $MIN_COUNT_HIERARCHICAL = (defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /MinCountHierarchical ([\d\.]+)/) ? $1 : undef;
    my $CORE_SCORE_OPTIONS = "";
    $CORE_SCORE_OPTIONS .= " --LogProb" if $LOG_PROB;
    $CORE_SCORE_OPTIONS .= " --NegLogProb" if $NEG_LOG_PROB;
    $CORE_SCORE_OPTIONS .= " --NoLex" if $NO_LEX;

    my $substep = 1;
    for my $direction ("f2e","e2f") {
	next if $___CONTINUE && -e "$ttable_file.half.$direction";
	next if $___CONTINUE && $direction eq "e2f" && -e "$ttable_file.half.e2f.sorted";
	my $inverse = "";
        my $extract_filename = $extract_file;
	if ($direction eq "e2f") {
	    $inverse = " --Inverse";
            $extract_filename = $extract_file.".inv";
        }
	my $extract = "$extract_filename.sorted";

	if (!($___CONTINUE && -e "$extract_filename.sorted")) {
	    # sorting
	    print STDERR "(6.".($substep++).")  sorting $direction @ ".`date`;
	    if (-e "$extract_filename.gz") {
		safesystem("gunzip < $extract_filename.gz | LC_ALL=C sort $__SORT_BUFFER_SIZE -T $___TEMP_DIR > $extract_filename.sorted") or die("ERROR");
	    }
	    else {
		safesystem("LC_ALL=C sort $__SORT_BUFFER_SIZE -T $___TEMP_DIR $extract_filename > $extract_filename.sorted") or die("ERROR");
	    }
        }

	print STDERR "(6.".($substep++).")  creating table half $ttable_file.half.$direction @ ".`date`;

        my $cmd = "$PHRASE_SCORE $extract $lexical_file.$direction $ttable_file.half.$direction $inverse";
        $cmd .= " --Hierarchical" if $_HIERARCHICAL;
        $cmd .= " --WordAlignment" if $_PHRASE_WORD_ALIGNMENT;
        $cmd .= " --KneserNey $ttable_file.coc" if $KNESER_NEY;
        $cmd .= " --GoodTuring $ttable_file.coc" if $GOOD_TURING && $inverse eq "";
        $cmd .= " --UnalignedPenalty" if $UNALIGNED_COUNT;
        $cmd .= " --UnalignedFunctionWordPenalty ".($inverse ? $UNALIGNED_FW_F : $UNALIGNED_FW_E) if $UNALIGNED_FW_COUNT;
        $cmd .= " --MinCountHierarchical $MIN_COUNT_HIERARCHICAL" if $MIN_COUNT_HIERARCHICAL;
        $cmd .= " $CORE_SCORE_OPTIONS" if defined($_SCORE_OPTIONS);
        print $cmd."\n";
        safesystem($cmd) or die "ERROR: Scoring of phrases failed";	    
        if (! $debug) { safesystem("rm -f $extract") or die("ERROR"); }
    }

    # sorting inverse phrase-table-half to sync up with regular one
    print STDERR "(6.5) sorting inverse e2f table@ ".`date`;
    if (! ($___CONTINUE && -e "$ttable_file.half.e2f.sorted")) {
	safesystem("LC_ALL=C sort $__SORT_BUFFER_SIZE -T $___TEMP_DIR $ttable_file.half.e2f > $ttable_file.half.e2f.sorted") or die("ERROR");
	if (! $debug) { safesystem("rm -f $ttable_file.half.e2f") or die("ERROR"); }
    }

    # merging the two halves
    print STDERR "(6.6) consolidating the two halves @ ".`date`;
    return if $___CONTINUE && -e "$ttable_file.gz";
    my $cmd = "$PHRASE_CONSOLIDATE $ttable_file.half.f2e $ttable_file.half.e2f.sorted $ttable_file";
    $cmd .= " --Hierarchical" if $_HIERARCHICAL;
    $cmd .= " --LogProb" if $LOG_PROB;
    $cmd .= " --NegLogProb" if $NEG_LOG_PROB;
    $cmd .= " --OnlyDirect" if $ONLY_DIRECT;
    $cmd .= " --NoPhraseCount" unless $PHRASE_COUNT;
    $cmd .= " --LowCountFeature" if $LOW_COUNT;
    $cmd .= " --GoodTuring $ttable_file.coc" if $GOOD_TURING;
    $cmd .= " --KneserNey $ttable_file.coc" if $KNESER_NEY;
    safesystem($cmd) or die "ERROR: Consolidating the two phrase table halves failed";
    if (! $debug) { safesystem("rm -f $ttable_file.half.*") or die("ERROR"); }
    if (! $___DONT_ZIP) {
        safesystem("gzip $ttable_file") || die("ERROR: could not gzip $ttable_file");
    }
}

sub score_phrase_memscore {
    my ($ttable_file,$lexical_file,$extract_file) = @_;

    return if $___CONTINUE && -e "$ttable_file.gz";

    my $options = $___MEMSCORE_OPTIONS;
    $options =~ s/\$LEX_F2E/$lexical_file.f2e/g;
    $options =~ s/\$LEX_E2F/$lexical_file.e2f/g;

    # The output is sorted to avoid breaking scripts that rely on the
    # sorting behaviour of the previous scoring algorithm.
    my $cmd = "$MEMSCORE $options | LC_ALL=C sort $__SORT_BUFFER_SIZE -T $___TEMP_DIR | gzip >$ttable_file.gz";
    if (-e "$extract_file.gz") {
        $cmd = "$ZCAT $extract_file.gz | ".$cmd;
    } else {
        $cmd = $cmd." <".$extract_file;
    }

    print $cmd."\n";
    safesystem($cmd) or die "ERROR: Scoring of phrases failed";
}

### (7) LEARN REORDERING MODEL

sub get_reordering_factored {
    print STDERR "(7) learn reordering model @ ".`date`;

    my @SPECIFIED_TABLE = @_REORDERING_TABLE;
    if ($REORDERING_LEXICAL) {
	if ($___NOT_FACTORED) {
	    print STDERR "(7.1) [no factors] learn reordering model @ ".`date`;
	    # foreach my $model (@REORDERING_MODELS) {
	    # 	#my $file = "$___MODEL_DIR/reordering-table.";
	    # 	$file .= $model->{"all"};
	    # 	#$file = shift @SPECIFIED_TABLE if scalar(@SPECIFIED_TABLE);
	    # 	$model->{"file"} = $file;
	    # }
        my $file = "$___MODEL_DIR/reordering-table";
	    $file = shift @SPECIFIED_TABLE if scalar(@SPECIFIED_TABLE);
        $file .= ".";
	    &get_reordering($___EXTRACT_FILE,$file);
	}
 	else {
	    foreach my $factor (split(/\+/,$___REORDERING_FACTORS)) {
		print STDERR "(7.1) [$factor] learn reordering model @ ".`date`;
		my ($factor_f,$factor_e) = split(/\-/,$factor);
		# foreach my $model (@REORDERING_MODELS) { 
		#     my $file = "$___MODEL_DIR/reordering-table.$factor";
		#     $file .= $model->{"all"};
		#     $file = shift @SPECIFIED_TABLE if scalar(@SPECIFIED_TABLE);
		#     $model->{"file"} = $file;
		# }
        my $file ="$___MODEL_DIR/reordering-table.$factor";
		$file = shift @SPECIFIED_TABLE if scalar(@SPECIFIED_TABLE);
        $file .= ".";
		&get_reordering("$___EXTRACT_FILE.$factor",$file);
	    }
	} 
    }
    else {
	print STDERR "  ... skipping this step, reordering is not lexicalized ...\n";
    }
}

sub get_reordering {
    my ($extract_file,$reo_model_path) = @_;
    if (-e "$extract_file.o.gz") {
	safesystem("gunzip < $extract_file.o.gz | LC_ALL=C sort $__SORT_BUFFER_SIZE -T $___TEMP_DIR > $extract_file.o.sorted") or die("ERROR");
    }
    else {
        safesystem("LC_ALL=C sort -T $___TEMP_DIR $extract_file.o > $extract_file.o.sorted") or die("ERROR");
    }

    my $smooth = $___REORDERING_SMOOTH;

    print STDERR "(7.2) building tables @ ".`date`;

    #create cmd string for lexical reordering scoring
    my $cmd = "$LEXICAL_REO_SCORER $extract_file.o.sorted $smooth $reo_model_path";
    $cmd .= " --SmoothWithCounts" if ($smooth =~ /(.+)u$/);
    for my $mtype (keys %REORDERING_MODEL_TYPES) {
	$cmd .= " --model \"$mtype $REORDERING_MODEL_TYPES{$mtype}";
	foreach my $model (@REORDERING_MODELS) {
	    if ($model->{"type"} eq $mtype) {
		$cmd .= " ".$model->{"filename"};
	    }
	}
	$cmd .= "\"";
    }
    
    #Call the lexical reordering scorer
    safesystem("$cmd") or die "ERROR: Lexical reordering scoring failed";

    if (! $debug) { safesystem("rm $extract_file.o.sorted") or die("ERROR");}
}



### (8) LEARN GENERATION MODEL

my $factor_e_source;
sub get_generation_factored {
    print STDERR "(8) learn generation model @ ".`date`;
    if (defined $___GENERATION_FACTORS) {
	my @SPECIFIED_TABLE = @_GENERATION_TABLE;
	my @TYPE = @_GENERATION_TYPE;
  my $corpus = $___CORPUS.".".$___E.$___CORPUS_COMPRESSION;
  $corpus = $_GENERATION_CORPUS if defined($_GENERATION_CORPUS);
	foreach my $factor (split(/\+/,$___GENERATION_FACTORS)) {
	    my ($factor_e_source,$factor_e) = split(/\-/,$factor);
	    my $file = "$___MODEL_DIR/generation.$factor";
	    $file = shift @SPECIFIED_TABLE if scalar(@SPECIFIED_TABLE);
	    my $type = "double";
	    $type = shift @TYPE if scalar @TYPE;
	    &get_generation($file,$type,$factor,$factor_e_source,$factor_e,$corpus);
	}
    } 
    else {
	print STDERR "  no generation model requested, skipping step\n";
    }
}

sub get_generation {
    my ($file,$type,$factor,$factor_e_source,$factor_e,$corpus) = @_;
    print STDERR "(8) [$factor] generate generation table @ ".`date`;
    $file = "$___MODEL_DIR/generation.$factor" unless $file;
    my (%WORD_TRANSLATION,%TOTAL_FOREIGN,%TOTAL_ENGLISH);

    my %INCLUDE_SOURCE;
    foreach my $factor (split(/,/,$factor_e_source)) {	
	$INCLUDE_SOURCE{$factor} = 1;
    }
    my %INCLUDE;
    foreach my $factor (split(/,/,$factor_e)) {
	$INCLUDE{$factor} = 1;
    }

    my (%GENERATION,%GENERATION_TOTAL_SOURCE,%GENERATION_TOTAL_TARGET);
    *E = open_or_zcat($corpus);
    while(<E>) {
	chomp;
	foreach (split) {
	    my @FACTOR = split(/\|/);

	    my ($source,$target);
	    my $first_factor = 1;
	    foreach my $factor (split(/,/,$factor_e_source)) {
		$source .= "|" unless $first_factor;
		$first_factor = 0;
		$source .= $FACTOR[$factor];
	    }

	    $first_factor = 1;
	    foreach my $factor (split(/,/,$factor_e)) {
		$target .= "|" unless $first_factor;
		$first_factor = 0;
		$target .= $FACTOR[$factor];
	    }	    
	    $GENERATION{$source}{$target}++;
	    $GENERATION_TOTAL_SOURCE{$source}++;
	    $GENERATION_TOTAL_TARGET{$target}++;
	}
    } 
    close(E);
    
    open(GEN,">$file") or die "ERROR: Can't write $file";
    foreach my $source (keys %GENERATION) {
	foreach my $target (keys %{$GENERATION{$source}}) {
	    printf GEN ("%s %s %.7f ",$source,$target,
                        $GENERATION{$source}{$target}/$GENERATION_TOTAL_SOURCE{$source});
            printf GEN (" %.7f",
                        $GENERATION{$source}{$target}/$GENERATION_TOTAL_TARGET{$target})
                unless $type eq 'single';
            print GEN "\n";
	}
    }
    close(GEN);
    safesystem("rm -f $file.gz") or die("ERROR");
    safesystem("gzip $file") or die("ERROR");
}

### (9) CREATE CONFIGURATION FILE

sub create_ini {
    print STDERR "(9) create moses.ini @ ".`date`;
    
    &full_path(\$___MODEL_DIR);
    &full_path(\$___VCB_E);
    &full_path(\$___VCB_F);
    `mkdir -p $___MODEL_DIR`;
    open(INI,">$___CONFIG") or die("ERROR: Can't write $___CONFIG");
    print INI "#########################
### MOSES CONFIG FILE ###
#########################
\n";
    
    if (defined $___TRANSLATION_FACTORS) {
	print INI "# input factors\n";
	print INI "[input-factors]\n";
	my $INPUT_FACTOR_MAX = 0;
	foreach my $table (split /\+/, $___TRANSLATION_FACTORS) {
	    my ($factor_list, $output) = split /-+/, $table;
	    foreach (split(/,/,$factor_list)) {
		$INPUT_FACTOR_MAX = $_ if $_>$INPUT_FACTOR_MAX;
	    }  
	}
	$INPUT_FACTOR_MAX = $_INPUT_FACTOR_MAX if $_INPUT_FACTOR_MAX; # use specified, if exists
	for (my $c = 0; $c <= $INPUT_FACTOR_MAX; $c++) { print INI "$c\n"; }
    } else {
      die "ERROR: No translation steps defined, cannot prepare [input-factors] section\n";
    }

    my %stepsused;
    print INI "\n# mapping steps
[mapping]\n";
   my $path = 0;
   my %FIRST_TTABLE;
   foreach (split(/:/,$___DECODING_STEPS)) {
     my $first_ttable_flag = 1;
     foreach (split(/,/,$_)) {
       s/t/T /g; 
       s/g/G /g;
       my ($type, $num) = split /\s+/;
       if ($first_ttable_flag && $type eq "T") {
         $FIRST_TTABLE{$num}++;
         $first_ttable_flag = 0;
       }
       $stepsused{$type} = $num+1 if !defined $stepsused{$type} || $stepsused{$type} < $num+1;
       print INI $path." ".$_."\n";
     }
     $path++;
   }
   print INI "1 T 1\n" if $_GLUE_GRAMMAR;

   if (defined($_DECODING_GRAPH_BACKOFF)) {
     $_DECODING_GRAPH_BACKOFF =~ s/\s+/ /g;
     $_DECODING_GRAPH_BACKOFF =~ s/^ //;
     print INI "\n[decoding-graph-backoff]\n";
     foreach (split(/ /,$_DECODING_GRAPH_BACKOFF)) {
       print INI "$_\n";
     }
   }
   print INI "\n# translation tables: table type (hierarchical(0), textual (0), binary (1)), source-factors, target-factors, number of scores, file 
# OLD FORMAT is still handled for back-compatibility
# OLD FORMAT translation tables: source-factors, target-factors, number of scores, file 
# OLD FORMAT a binary table type (1) is assumed 
[ttable-file]\n";
   my $num_of_ttables = 0;
   my @SPECIFIED_TABLE = @_PHRASE_TABLE;
   my $basic_weight_count = 4; # both directions, lex and phrase
   $basic_weight_count-=2 if defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /NoLex/;
   $basic_weight_count+=2 if defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /UnalignedPenalty/; # word ins/del
   $basic_weight_count+=2 if defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /UnalignedFunctionWordPenalty/;
   $basic_weight_count /= 2 if defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /OnlyDirect/;
   $basic_weight_count++ unless defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /NoPhraseCount/; # phrase count feature
   $basic_weight_count++ if defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /LowCountFeature/; # low count feature
   foreach my $f (split(/\+/,$___TRANSLATION_FACTORS)) {
     $num_of_ttables++;
     my $ff = $f;
     $ff =~ s/\-/ /;
     my $file = "$___MODEL_DIR/".($_HIERARCHICAL?"rule-table":"phrase-table").($___NOT_FACTORED ? "" : ".$f").".gz";
     $file = shift @SPECIFIED_TABLE if scalar(@SPECIFIED_TABLE);
     my $phrase_table_impl = ($_HIERARCHICAL ? 6 : 0);
     print INI "$phrase_table_impl $ff $basic_weight_count $file\n";
   }
   if ($_GLUE_GRAMMAR) {
     &full_path(\$___GLUE_GRAMMAR_FILE);
     print INI "6 0 0 1 $___GLUE_GRAMMAR_FILE\n";
   }
   if ($num_of_ttables != $stepsused{"T"}) {
     print STDERR "WARNING: Your [mapping-steps] require translation steps up to id $stepsused{T} but you defined translation steps 0..$num_of_ttables\n";
     exit 1 if $num_of_ttables < $stepsused{"T"}; # fatal to define less
   }

    if (defined $___GENERATION_FACTORS) {
      my @TYPE = @_GENERATION_TYPE;
      print INI "\n# generation models: source-factors, target-factors, number-of-weights, filename\n";
      print INI "[generation-file]\n";
      my $cnt = 0;
      my @SPECIFIED_TABLE = @_GENERATION_TABLE;
      foreach my $f (split(/\+/,$___GENERATION_FACTORS)) {
        my $weights_per_generation_model = 2;
        $weights_per_generation_model = 1 if scalar(@TYPE) && (shift @TYPE) eq 'single';
        $cnt++;
        my $ff = $f;
        $ff =~ s/\-/ /;
	my $file = "$___MODEL_DIR/generation.$f";
	$file = shift @SPECIFIED_TABLE if scalar(@SPECIFIED_TABLE);
        print INI "$ff $weights_per_generation_model $file\n";
      }
      if ($cnt != $stepsused{"G"}) {
        print STDERR "WARNING: Your [mapping-steps] require generation steps up to id $stepsused{G} but you defined generation steps 0..$cnt\n";
        exit 1 if $cnt < $stepsused{"G"}; # fatal to define less
      }
    } else {
      print INI "\n# no generation models, no generation-file section\n";
    }

  print INI "\n# language models: type(srilm/irstlm), factors, order, file\n[lmodel-file]\n";
  foreach my $lm (@___LM) {
    my ($f, $o, $fn, $type) = @{$lm};
    if ($fn !~ /^\//) {
      my $path = `pwd`; chop($path);
      $fn = $path."/".$fn;
    }
    $type = 0 unless $type;
    print INI "$type $f $o $fn\n";
  }

  print INI "\n\n\# limit on how many phrase translations e for each phrase f are loaded\n# 0 = all elements loaded\n[ttable-limit]\n20\n";
  foreach(1 .. ($num_of_ttables-1)) {
    print INI (defined($FIRST_TTABLE{$_})?"20":"0")."\n";
  }
  print INI "\n";

  my $weight_d_count = 1;
  if ($___REORDERING ne "distance") {
    my $file = "# distortion (reordering) files\n\[distortion-file]\n";
    my $factor_i = 0;
 
    my @SPECIFIED_TABLE = @_REORDERING_TABLE;
    foreach my $factor (split(/\+/,$___REORDERING_FACTORS)) {
	foreach my $model (@REORDERING_MODELS) {
	    $weight_d_count += $model->{"numfeatures"};
	    my $table_file = "$___MODEL_DIR/reordering-table";
	    $table_file .= ".$factor" unless $___NOT_FACTORED;
	    $table_file = shift @SPECIFIED_TABLE if scalar(@SPECIFIED_TABLE);
	    $table_file .= ".";
	    $table_file .= $model->{"filename"};
	    $table_file .= ".gz";
	    $file .= "$factor ".$model->{"config"}." ".$model->{"numfeatures"}." $table_file\n";
	}
        $factor_i++;
      }
      print INI $file."\n";
  }
  else {
    $weight_d_count = 1;
  }
  
  if (!$_HIERARCHICAL) {
    print INI "# distortion (reordering) weight\n[weight-d]\n";
    for(my $i=0;$i<$weight_d_count;$i++) { 
      print INI "".(0.6/(scalar @REORDERING_MODELS+1))."\n";
    }
  }
  print INI "\n# language model weights\n[weight-l]\n";
  my $lmweighttotal = 0.5;
  foreach(1..scalar @___LM) {
    printf INI "%.4f\n", $lmweighttotal / scalar @___LM;
  }

  print INI "\n\n# translation model weights\n[weight-t]\n";
  foreach my $f (split(/\+/,$___TRANSLATION_FACTORS)) {
     for(1..$basic_weight_count) {
       printf INI "%.2f\n", 1/$basic_weight_count;
     }
  }
  print INI "1.0\n" if $_HIERARCHICAL; # glue grammar

    if (defined $___GENERATION_FACTORS) {
      print INI "\n# generation model weights\n";
      print INI "[weight-generation]\n";
      my @TYPE = @_GENERATION_TYPE;
      foreach my $f (split(/\+/,$___GENERATION_FACTORS)) {
        print INI "0.3\n";
        print INI "0\n" unless scalar(@TYPE) && (shift @TYPE) eq 'single';
      }
    } else {
      print INI "\n# no generation models, no weight-generation section\n";
    }

  print INI "\n# word penalty\n[weight-w]\n-1\n\n";

  if ($_HIERARCHICAL) {
    print INI "[unknown-lhs]\n$_UNKNOWN_WORD_LABEL_FILE\n\n" if $_TARGET_SYNTAX && defined($_UNKNOWN_WORD_LABEL_FILE);
    print INI "[cube-pruning-pop-limit]\n1000\n\n";
    print INI "[non-terminals]\nX\n\n";
    print INI "[search-algorithm]\n3\n\n";
    print INI "[inputtype]\n3\n\n";
    print INI "[max-chart-span]\n";
    foreach (split(/\+/,$___TRANSLATION_FACTORS)) { print INI "20\n"; }
    print INI "1000\n";
  }
  else {
    print INI "[distortion-limit]\n6\n";
  }

  # only set the factor delimiter if it is non-standard
  unless ($___FACTOR_DELIMITER eq '|') {
    print INI "\n# delimiter between factors in input\n[factor-delimiter]\n$___FACTOR_DELIMITER\n\n"
  }

  if ($_ADDITIONAL_INI) {
    print INI "\n# additional settings\n\n";
    foreach (split(/<br>/i,$_ADDITIONAL_INI)) { print INI $_."\n"; }
  }

  close(INI);
}

sub full_path {
    my ($PATH) = @_;
    return if $$PATH =~ /^\//;
    $$PATH = `pwd`."/".$$PATH;
    $$PATH =~ s/[\r\n]//g;
    $$PATH =~ s/\/\.\//\//g;
    $$PATH =~ s/\/+/\//g;
    my $sanity = 0;
    while($$PATH =~ /\/\.\.\// && $sanity++<10) {
	$$PATH =~ s/\/+/\//g;
	$$PATH =~ s/\/[^\/]+\/\.\.\//\//g;
    }
    $$PATH =~ s/\/[^\/]+\/\.\.$//;
    $$PATH =~ s/\/+$//;
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

sub fix_spaces(){
        my ($in) = @_;
        $$in =~ s/[ \t]+/ /g; $$in =~ s/[ \t]$//; $$in =~ s/^[ \t]//;    
}

