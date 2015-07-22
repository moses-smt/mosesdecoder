#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";
use FindBin qw($Bin);
use File::Spec::Functions;
use File::Spec::Unix;
use File::Basename;

# Train Factored Phrase Model
# (c) 2006-2009 Philipp Koehn
# with contributions from other JHU WS participants
# Modified to extract and score mbot rules only!!!!
# Train a mbot model from a parallel corpus
# -----------------------------------------------------
$ENV{"LC_ALL"} = "C";
my $SCRIPTS_ROOTDIR = $Bin;
if ($SCRIPTS_ROOTDIR eq '') {
  $SCRIPTS_ROOTDIR = dirname(__FILE__);
}
$SCRIPTS_ROOTDIR =~ s/\/training$//;
$SCRIPTS_ROOTDIR = $ENV{"SCRIPTS_ROOTDIR"} if defined($ENV{"SCRIPTS_ROOTDIR"});

my($_ROOT_DIR, $_CORPUS_DIR, $_CORPUS, $_MODEL_DIR, $_MBOT_MODEL_DIR, $_SORT_BUFFER_SIZE, $_BIN_DIR,
   $_SORT_BATCH_SIZE,  $_SORT_COMPRESS, $_SORT_PARALLEL, $_FIRST_STEP, $_LAST_STEP, $_F, $_E, $_SCRIPTS_DIR,
   $_MAX_PHRASE_LENGTH, $_ALIGNMENT_FILE, $_LEXICAL_FILE, $_EXTRACT_FILE, @_PHRASE_TABLE, $_PARALLEL,
   $_SOURCE_SYNTAX, $_TARGET_SYNTAX, $_GLUE_GRAMMAR, $_GLUE_GRAMMAR_FILE, $_MBOT_GRAMMAR, $_MBOT_GRAMMAR_FILE,
   $_UNKNOWN_WORD_LABEL_FILE, @_EXTRACT_OPTIONS, @_SCORE_OPTIONS, $_CONFIG, $_DO_STEPS, $_VERBOSE, $_HELP, @_LM);

my $_CORES = 1;

my $debug = 0; # debug this script, do not delete any files in debug mode

$_HELP = 1
    unless &GetOptions('root-dir=s' => \$_ROOT_DIR,
           'scripts-root-dir=s' => \$SCRIPTS_ROOTDIR,
		       'corpus-dir=s' => \$_CORPUS_DIR,
		       'corpus=s' => \$_CORPUS,
		       'mbot-model-dir=s' => \$_MBOT_MODEL_DIR,
		       'f=s' => \$_F,
		       'e=s' => \$_E,
		       'max-phrase-length=s' => \$_MAX_PHRASE_LENGTH,
		       'lexical-file=s' => \$_LEXICAL_FILE,
		       'sort-buffer-size=s' => \$_SORT_BUFFER_SIZE,
		       'sort-batch-size=i' => \$_SORT_BATCH_SIZE,
		       'sort-compress=s' => \$_SORT_COMPRESS,
		       'sort-parallel=i' => \$_SORT_PARALLEL,
		       'extract-file=s' => \$_EXTRACT_FILE,
		       'alignment-file=s' => \$_ALIGNMENT_FILE,
		       'config=s' => \$_CONFIG,
		       'first-step=i' => \$_FIRST_STEP,
		       'last-step=i' => \$_LAST_STEP,
		       'lm=s' => \@_LM,
		       'help' => \$_HELP,
           'debug' => \$debug,
           'parallel' => \$_PARALLEL,
		       'glue-grammar' => \$_GLUE_GRAMMAR,
		       'glue-grammar-file=s' => \$_GLUE_GRAMMAR_FILE,
           'mbot-grammar' => \$_MBOT_GRAMMAR,
		       'mbot-grammar-file=s' => \$_MBOT_GRAMMAR_FILE,
		       'unknown-word-label-file=s' => \$_UNKNOWN_WORD_LABEL_FILE,
		       'extract-options=s' => \@_EXTRACT_OPTIONS,
		       'score-options=s' => \@_SCORE_OPTIONS,
		       'source-syntax' => \$_SOURCE_SYNTAX,
           'target-syntax' => \$_TARGET_SYNTAX,
		       'do-steps=s' => \$_DO_STEPS,
		       'cores=i' => \$_CORES
           );

if ($_HELP) {
  print "Train MBOT Model

Steps: (--first-step to --last-step)
  (1) extract mbot rules
  (2) compute rule probabilities for mbot rules
  (3) create decoder config file

  For more, please check manual or contact seemanna\@ims.uni-stuttgart.de\n";
  exit(1);
}

# convert all paths to absolute paths
$SCRIPTS_ROOTDIR = File::Spec->rel2abs($SCRIPTS_ROOTDIR) if defined($SCRIPTS_ROOTDIR);
$_ROOT_DIR = File::Spec->rel2abs($_ROOT_DIR) if defined($_ROOT_DIR);
$_BIN_DIR = File::Spec->rel2abs($_BIN_DIR) if defined($_BIN_DIR);
$_CORPUS_DIR = File::Spec->rel2abs($_CORPUS_DIR) if defined($_CORPUS_DIR);
$_CORPUS = File::Spec->rel2abs($_CORPUS) if defined($_CORPUS);
$_MBOT_MODEL_DIR = File::Spec->rel2abs($_MBOT_MODEL_DIR) if defined($_MBOT_MODEL_DIR);
$_ALIGNMENT_FILE = File::Spec->rel2abs($_ALIGNMENT_FILE) if defined($_ALIGNMENT_FILE);
$_GLUE_GRAMMAR_FILE = File::Spec->rel2abs($_GLUE_GRAMMAR_FILE) if defined($_GLUE_GRAMMAR_FILE);
$_MBOT_GRAMMAR_FILE = File::Spec->rel2abs($_MBOT_GRAMMAR_FILE) if defined($_MBOT_GRAMMAR_FILE);
$_UNKNOWN_WORD_LABEL_FILE = File::Spec->rel2abs($_UNKNOWN_WORD_LABEL_FILE) if defined($_UNKNOWN_WORD_LABEL_FILE);
$_LEXICAL_FILE = File::Spec->rel2abs($_LEXICAL_FILE) if defined($_LEXICAL_FILE);
$_EXTRACT_FILE = File::Spec->rel2abs($_EXTRACT_FILE) if defined($_EXTRACT_FILE);
foreach (@_PHRASE_TABLE) { $_ = File::Spec->rel2abs($_); }

my $_SCORE_OPTIONS; # allow multiple switches
foreach (@_SCORE_OPTIONS) { $_SCORE_OPTIONS .= $_." "; }
chop($_SCORE_OPTIONS) if $_SCORE_OPTIONS;
my $_EXTRACT_OPTIONS; # allow multiple switches
foreach (@_EXTRACT_OPTIONS) { $_EXTRACT_OPTIONS .= $_." "; }
chop($_EXTRACT_OPTIONS) if $_EXTRACT_OPTIONS;

print STDERR "Using SCRIPTS_ROOTDIR: $SCRIPTS_ROOTDIR\n";

# Setting the steps to perform
my $___VERBOSE = 0;
my $___FIRST_STEP = 1;
my $___LAST_STEP = 3;
$___VERBOSE = $_VERBOSE if $_VERBOSE;
$___FIRST_STEP = $_FIRST_STEP if $_FIRST_STEP;
$___LAST_STEP =  $_LAST_STEP  if $_LAST_STEP;
my $___DO_STEPS = $___FIRST_STEP."-".$___LAST_STEP;
$___DO_STEPS = $_DO_STEPS if $_DO_STEPS;
my @STEPS = (0,0,0);

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
  die("Only steps between 1 and 3 can be used") if ($f < 1 || $l > 3);
  die("The first step must be smaller than the last step") if ($f > $l);

  for (my $i=$f; $i<=$l; $i++) {
    $STEPS[$i] = 1;
  }
}

my @___LM = ();
if ($STEPS[3]) {
  die "ERROR: use --lm factor:order:filename:type to specify at least one language model"
    if scalar @_LM == 0;
  foreach my $lm (@_LM) {
    my $type = 0; # default to srilm
    my ($f, $order, $filename);
    ($f, $order, $filename, $type) = split /:/, $lm, 4;
    die "ERROR: Wrong format of --lm. Expected: --lm factor:order:filename:type"
      if $f !~ /^[0-9,]+$/ || $order !~ /^[0-9]+$/ || !defined $filename;
    die "ERROR: Filename is not absolute: $filename"
      unless file_name_is_absolute $filename;
    die "ERROR: Language model file not found or empty: $filename"
      if ! -e $filename;
    push @___LM, [ $f, $order, $filename, $type ];
  }
}

# set varibles to defaults or from options
my $___ROOT_DIR = ".";
$___ROOT_DIR = $_ROOT_DIR if $_ROOT_DIR;
my $___CORPUS_DIR  = $___ROOT_DIR . "/corpus";
$___CORPUS_DIR = $_CORPUS_DIR if $_CORPUS_DIR;
die("ERROR: use --corpus to specify corpus") unless $_CORPUS || !$STEPS[1];
my $___CORPUS      = $_CORPUS;
my $___BIN_DIR = "$SCRIPTS_ROOTDIR/../mbot-extract";

# mbot model dir and alignment and extract file
my $___MBOT_MODEL_DIR = $___ROOT_DIR . "/mbot-model";
$___MBOT_MODEL_DIR = $_MBOT_MODEL_DIR if $_MBOT_MODEL_DIR;
my $___LEXICAL_FILE = $___MBOT_MODEL_DIR."/lex";
$___LEXICAL_FILE = $_LEXICAL_FILE if $_LEXICAL_FILE;
my $___ALIGNMENT_FILE = $___MBOT_MODEL_DIR . "/aligned.grow-diag-final-and";
$___ALIGNMENT_FILE = $_ALIGNMENT_FILE if $_ALIGNMENT_FILE;
my $___EXTRACT_FILE = $___MBOT_MODEL_DIR . "/extracted-rules";
$___EXTRACT_FILE = $_EXTRACT_FILE if $_EXTRACT_FILE;
my $___GLUE_GRAMMAR_FILE = $___MBOT_MODEL_DIR . "/glue-grammar";
$___GLUE_GRAMMAR_FILE = $_GLUE_GRAMMAR_FILE if $_GLUE_GRAMMAR_FILE;
my $___MBOT_GRAMMAR_FILE = $___MBOT_MODEL_DIR . "/mbot-glue-grammar";
$___MBOT_GRAMMAR_FILE = $_MBOT_GRAMMAR_FILE if $_MBOT_GRAMMAR_FILE;
my $___UNKNOWN_WORD_LABEL_FILE =  $___MBOT_MODEL_DIR . "/unknown-lhs.txt";
$___UNKNOWN_WORD_LABEL_FILE = $_UNKNOWN_WORD_LABEL_FILE if $_UNKNOWN_WORD_LABEL_FILE;
my $___MAX_PHRASE_LENGTH = "10";
$___MAX_PHRASE_LENGTH = $_MAX_PHRASE_LENGTH if $_MAX_PHRASE_LENGTH;
my $RULE_FILE = $___MBOT_MODEL_DIR . "/mbot-rule-table.gz";
my $___CONFIG = $___MBOT_MODEL_DIR."/moses.ini";
$___CONFIG = $_CONFIG if $_CONFIG;

# parallel extract
my $SPLIT_EXEC = 'split';

my $SORT_EXEC = 'sort';

my $__SORT_BUFFER_SIZE = "";
$__SORT_BUFFER_SIZE = "-S $_SORT_BUFFER_SIZE" if $_SORT_BUFFER_SIZE;

my $__SORT_BATCH_SIZE = "";
$__SORT_BATCH_SIZE = "--batch-size $_SORT_BATCH_SIZE" if $_SORT_BATCH_SIZE;

my $__SORT_COMPRESS = "";
$__SORT_COMPRESS = "--compress-program $_SORT_COMPRESS" if $_SORT_COMPRESS;

my $__SORT_PARALLEL = "";
$__SORT_PARALLEL = "--parallel $_SORT_PARALLEL" if $_SORT_PARALLEL;


# supporting scripts/binaries from this package
my $RULE_EXTRACT = "$___BIN_DIR/extract_mbot_rules";
$RULE_EXTRACT = "$SCRIPTS_ROOTDIR/generic/extract-parallel-mbot.perl $_CORES $SPLIT_EXEC \"$SORT_EXEC $__SORT_BUFFER_SIZE $__SORT_BATCH_SIZE $__SORT_COMPRESS $__SORT_PARALLEL\" $RULE_EXTRACT";

my $RULE_SCORE = "$___BIN_DIR/score_mbot";
$RULE_SCORE = "$SCRIPTS_ROOTDIR/generic/score-parallel-mbot.perl $_CORES \"$SORT_EXEC $__SORT_BUFFER_SIZE $__SORT_BATCH_SIZE $__SORT_COMPRESS $__SORT_PARALLEL\" $RULE_SCORE";
my $RULE_CONSOLIDATE = "$___BIN_DIR/consolidate_mbot";

# utilities
my $ZCAT = "gzip -cd";
my $BZCAT = "bzcat";

# foreign/English language extension
die("ERROR: use --f to specify foreign language") unless $_F;
die("ERROR: use --e to specify English language") unless $_E;
my $___F = $_F;
my $___E = $_E;

# don't fork
my $___NOFORK = !defined $_PARALLEL;


### MAIN
&extract_mbot_rules() if $STEPS[1];
&score_mbot_rules() if $STEPS[2];
&create_ini() if $STEPS[3];

### (1) MBOT RULE EXTRACTION
sub extract_mbot_rules {
  print STDERR "(1) extract mbot rules @ ".`date`;
  &extract_phrase($___CORPUS . "." . $___F,
                  $___CORPUS . "." . $___E,
                  $___EXTRACT_FILE,
                  $___ALIGNMENT_FILE);
}

sub extract_phrase {
  my ($alignment_file_f,$alignment_file_e,$extract_file,$alignment_file_a) = @_;
  # Make sure the corpus exists in unzipped form
  my @tempfiles = ();
  foreach my $f ($alignment_file_e, $alignment_file_f, $alignment_file_a) {
    if (! -e $f && -e $f.".gz") {
      safesystem("gunzip < $f.gz > $f") or die("Failed to gunzip corpus $f");
      push @tempfiles, "$f.gz";
    }
  }
  my $cmd;
  $cmd = "$RULE_EXTRACT $alignment_file_e $alignment_file_f $alignment_file_a $extract_file";
  $cmd .= " --GlueGrammar $___GLUE_GRAMMAR_FILE";
  $cmd .= " --MbotGrammar $___MBOT_GRAMMAR_FILE";
  $cmd .= " --UnknownWordLabel $___UNKNOWN_WORD_LABEL_FILE";
  $cmd .= " --SourceSyntax" if $_SOURCE_SYNTAX;
  #$cmd .= " --TargetSyntax";
  $cmd .= " --MaxSpan $___MAX_PHRASE_LENGTH";
  $cmd .= " --GZOutput";
  $cmd .= " " . $_EXTRACT_OPTIONS if defined ($_EXTRACT_OPTIONS);

  map { die "File not found: $_" if ! -e $_ } ($alignment_file_e, $alignment_file_f, $alignment_file_a);
  print STDERR "EXECUTING: $cmd\n";
  safesystem("$cmd") or die "ERROR: MBOT rule extraction failed (missing input files?)";
  foreach my $f (@tempfiles) {
    unlink $f;
  }
}

### (2) SCORE MBOT RULES
sub score_mbot_rules {
  print STDERR "(2) score mbot rules @ ".`date`;

  my $ttable_file = "$___MBOT_MODEL_DIR/mbot-rule-table";
  my $PHRASE_COUNT = (!defined($_SCORE_OPTIONS) || $_SCORE_OPTIONS !~ /NoPhraseCount/);
  my $UNALIGNED_COUNT = (defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /UnalignedPenalty/);
  my $GOOD_TURING = (defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /GoodTuring/);

  my $substep = 1;
  my $isParent = 1;
  my @children;

  for my $direction ("f2e","e2f") {
    if ($___NOFORK and @children > 0) {
      waitpid((shift @children), 0);
		  $substep+=2;
    }
    my $pid = fork();

    if ($pid == 0) {
      next if -e "$ttable_file.half.$direction";
      next if $direction eq "e2f" && -e "$ttable_file.half.e2f.gz";
      my $inverse = "";
      my $extract_filename = $___EXTRACT_FILE;
      if ($direction eq "e2f") {
        $inverse = "--Inverse";
        $extract_filename = $___EXTRACT_FILE.".inv";
      }

      my $extract = "$extract_filename.sorted.gz";

      print STDERR "(2.".($substep++).")  creating table half $ttable_file.half.$direction @ ".`date`;

      my $cmd = "$RULE_SCORE $extract $___LEXICAL_FILE.$direction $ttable_file.half.$direction.gz $inverse";
      $cmd .= " --GoodTuring" if $GOOD_TURING && $inverse eq "";
      $cmd .= " --UnalignedPenalty" if $UNALIGNED_COUNT;

      # sorting
      if ($direction eq "e2f") {
        $cmd .= " 1 ";
      }
      else {
        $cmd .= " 0 ";
      }

      print STDERR $cmd."\n";
      safesystem($cmd) or die "ERROR: Scoring of phrases failed";

      exit();
    }
    else { # parent
      push(@children, $pid);
    }
  }

  # wait for everything to finish
  if ($isParent) {
    foreach (@children) {
      waitpid($_, 0);
    }
  }
  else {
    die "shouldn't be here";
  }

  # merging the two halves
  print STDERR "(2.6) consolidating the two halves @ ".`date`;
  return if -e "$ttable_file.gz";
  my $cmd = "$RULE_CONSOLIDATE $ttable_file.half.f2e.gz $ttable_file.half.e2f.gz /dev/stdout";
  $cmd .= " --GoodTuring $ttable_file.half.f2e.gz.coc" if $GOOD_TURING;
  $cmd .= " --PhraseCount" if $PHRASE_COUNT;
  $cmd .= " --Mbot";

  $cmd .= " | gzip -c > $ttable_file.gz";

  safesystem($cmd) or die "ERROR: Consolidating the two phrase table halves failed";
  if (! $debug) { safesystem("rm -f $ttable_file.half.*") or die("ERROR"); }
}


### (3) CREATE CONFIGURATION FILE
sub create_ini {
  print STDERR "(3) create moses.ini @ ".`date`;

  &full_path(\$___MBOT_MODEL_DIR);
  `mkdir -p $___MBOT_MODEL_DIR`;
  open(INI,">$___CONFIG") or die ("ERROR: Can't write $___CONFIG");
    print INI "#########################
### MOSES CONFIG FILE ###
#########################
\n";

  # input factors
  print INI "[input-factors]\n0\n\n";

  # mapping steps
  print INI "[mapping]\n";
  print INI "0 T 0\n";
  print INI "1 T 1\n" if $_GLUE_GRAMMAR;
  print INI "2 T 2\n" if $_MBOT_GRAMMAR;
  print INI "\n";

  # translation tables
  my @SPECIFIED_TABLE = @_PHRASE_TABLE;

  # number of weights
  my $basic_weight_count = 4; # both directions, lex and phrase
  $basic_weight_count-=2 if defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /NoLex/;
  #$basic_weight_count+=2 if defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS =~ /UnalignedPenalty/; # word ins/del
  $basic_weight_count++ if defined($_SCORE_OPTIONS) && $_SCORE_OPTIONS !~ /NoPhraseCount/; # phrase count feature

  my $file = "$___MBOT_MODEL_DIR/mbot-rule-table.gz";
  my $phrase_table_impl = 11;

  print INI "[ttable-file]\n";
  print INI "11 0 0 " . $basic_weight_count . " " . $file . "\n";
  print INI "11 0 0 1 " . $___GLUE_GRAMMAR_FILE . "\n";
  print INI "11 0 0 1 " . $___MBOT_GRAMMAR_FILE . "\n";
  print INI "\n";

  # language model
  foreach my $lm (@___LM) {
    my ($f, $o, $fn, $type) = @{$lm};
    print INI "[lmodel-file]\n" . $type . " " . $f . " " . $o . " $fn\n\n";
  }

  # ttable limit
  print INI "[ttable-limit]\n";
  print INI "100\n\n";

  print INI "[weight-l]\n0.5\n\n";

  print INI "[weight-t]\n";
  my $value = 1/$basic_weight_count;
  my $i = 1;
  while ( $i <= $basic_weight_count ) {
    print INI "$value\n";
    $i++;
  }
  print INI "1.0\n";
  print INI "1.0\n";
  print INI "\n";

  # word penalty
  print INI "[weight-w]\n-1.0\n\n";

  # gap penalty
  print INI "[weight-gap]\n1.0\n\n";

  # unknown lhs file
  print INI "[unknown-lhs]\n$___UNKNOWN_WORD_LABEL_FILE\n\n";
  print INI "[cube-pruning-pop-limit]\n1000\n\n";
  if ( $_SOURCE_SYNTAX ) {
    print INI "[non-terminals]\nROOT\n\n";
  }
  else {
    print INI "[non-terminals]\nX\n\n";
  }
  print INI "[search-algorithm]\n3\n\n";
  print INI "[inputtype]\n3\n\n";
  print INI "[max-chart-span]\n20\n";
  print INI "1000\n" if $_GLUE_GRAMMAR;
  print INI "1000\n" if $_MBOT_GRAMMAR;
  print INI "\n# Do the source labels have to match the parse tree at rule application
# 1 : Yes
# 2 : No\n";
  if ( $_SOURCE_SYNTAX ) {
    print INI "[source-label-matching-at-rule-application]\n1\n\n";
  }
  else {
    print INI "[source-label-matching-at-rule-application]\n2\n\n";
  }
  print INI "[threads]\n$_CORES\n\n";
}


#######################################
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
  }
  elsif ($fn =~ /\.gz$/) {
    $read = "$ZCAT $fn|";
  }
  my $hdl;
  open($hdl,$read) or die "Can't read $fn ($read)";
  return $hdl;
}

