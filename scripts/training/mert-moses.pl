#!/usr/bin/perl -w 
# $Id$
# Usage:
# mert-moses.pl <foreign> <english> <decoder-executable> <decoder-config>
# For other options see below or run 'mert-moses.pl --help'

# Notes:
# <foreign> and <english> should be raw text files, one sentence per line
# <english> can be a prefix, in which case the files are <english>0, <english>1, etc. are used

# Excerpts from revision history

# Sept 2011   multi-threaded mert (Barry Haddow)
# 3 Aug 2011  Added random directions, historic best, pairwise ranked (PK)
# Jul 2011    simplifications (Ondrej Bojar)
#             -- rely on moses' -show-weights instead of parsing moses.ini
#                ... so moses is also run once *before* mert starts, checking
#                    the model to some extent
#             -- got rid of the 'triples' mess;
#                use --range to supply bounds for random starting values:
#                --range tm:-3..3 --range lm:-3..3
# 5 Aug 2009  Handling with different reference length policies (shortest, average, closest) for BLEU
#             and case-sensistive/insensitive evaluation (Nicola Bertoldi)
# 5 Jun 2008  Forked previous version to support new mert implementation.
# 13 Feb 2007 Better handling of default values for lambda, now works with multiple
#             models and lexicalized reordering
# 11 Oct 2006 Handle different input types through parameter --inputype=[0|1]
#             (0 for text, 1 for confusion network, default is 0) (Nicola Bertoldi)
# 10 Oct 2006 Allow skip of filtering of phrase tables (--no-filter-phrase-table)
#             useful if binary phrase tables are used (Nicola Bertoldi)
# 28 Aug 2006 Use either closest or average or shortest (default) reference
#             length as effective reference length
#             Use either normalization or not (default) of texts (Nicola Bertoldi)
# 31 Jul 2006 move gzip run*.out to avoid failure wit restartings
#             adding default paths
# 29 Jul 2006 run-filter, score-nbest and mert run on the queue (Nicola; Ondrej had to type it in again)
# 28 Jul 2006 attempt at foolproof usage, strong checking of input validity, merged the parallel and nonparallel version (Ondrej Bojar)
# 27 Jul 2006 adding the safesystem() function to handle with process failure
# 22 Jul 2006 fixed a bug about handling relative path of configuration file (Nicola Bertoldi)
# 21 Jul 2006 adapted for Moses-in-parallel (Nicola Bertoldi)
# 18 Jul 2006 adapted for Moses and cleaned up (PK)
# 21 Jan 2005 unified various versions, thorough cleanup (DWC)
#             now indexing accumulated n-best list solely by feature vectors
# 14 Dec 2004 reimplemented find_threshold_points in C (NMD)
# 25 Oct 2004 Use either average or shortest (default) reference
#             length as effective reference length (DWC)
# 13 Oct 2004 Use alternative decoders (DWC)
# Original version by Philipp Koehn

use strict;
use FindBin qw($RealBin);
use File::Basename;
use File::Path;
use File::Spec;
use Cwd;

my $SCRIPTS_ROOTDIR = $RealBin;
$SCRIPTS_ROOTDIR =~ s/\/training$//;
$SCRIPTS_ROOTDIR = $ENV{"SCRIPTS_ROOTDIR"} if defined($ENV{"SCRIPTS_ROOTDIR"});

my $minimum_required_change_in_weights = 0.00001;
    # stop if no lambda changes more than this

my $verbose = 0;
my $usage = 0; # request for --help

# We assume that if you don't specify working directory,
# we set the default is set to `pwd`/mert-work
my $___WORKING_DIR = File::Spec->catfile(Cwd::getcwd(), "mert-work");
my $___DEV_F = undef; # required, input text to decode
my $___DEV_E = undef; # required, basename of files with references
my $___DECODER = undef; # required, pathname to the decoder executable
my $___CONFIG = undef; # required, pathname to startup ini file
my $___N_BEST_LIST_SIZE = 100;
my $___LATTICE_SAMPLES = 0;
my $queue_flags = "-hard";  # extra parameters for parallelizer
      # the -l ws0ssmt was relevant only to JHU 2006 workshop
my $___JOBS = undef; # if parallel, number of jobs to use (undef or 0 -> serial)
my $___DECODER_FLAGS = ""; # additional parametrs to pass to the decoder
my $continue = 0; # should we try to continue from the last saved step?
my $skip_decoder = 0; # and should we skip the first decoder run (assuming we got interrupted during mert)
my $___FILTER_PHRASE_TABLE = 1; # filter phrase table
my $___PREDICTABLE_SEEDS = 0;
my $___START_WITH_HISTORIC_BESTS = 0; # use best settings from all previous iterations as starting points [Foster&Kuhn,2009]
my $___RANDOM_DIRECTIONS = 0; # search in random directions only
my $___NUM_RANDOM_DIRECTIONS = 0; # number of random directions, also works with default optimizer [Cer&al.,2008]
my $___RANDOM_RESTARTS = 20;
my $___RETURN_BEST_DEV = 0; # return the best weights according to dev, not the last

# Flags related to PRO (Hopkins & May, 2011)
my $___PAIRWISE_RANKED_OPTIMIZER = 0; # flag to enable PRO.
my $___PRO_STARTING_POINT = 0; # get a starting point from pairwise ranked optimizer
my $___HISTORIC_INTERPOLATION = 0; # interpolate optimize weights with previous iteration's weights [Hopkins&May,2011,5.4.3]
# MegaM's options for PRO optimization.
# TODO: Should we also add these values to options of this script?
my $megam_default_options = "-fvals -maxi 30 -nobias binary";

# Flags related to Batch MIRA (Cherry & Foster, 2012)
my $___BATCH_MIRA = 0; # flg to enable batch MIRA

# Train phrase model mixture weights with PRO (Haddow, NAACL 2012)
my $__PROMIX_TRAINING = undef; # Location of main script (contrib/promix/main.py)
# The phrase tables. These should be gzip text format.
my @__PROMIX_TABLES;
# used to filter output
my $__REMOVE_SEGMENTATION = "$SCRIPTS_ROOTDIR/ems/support/remove-segmentation-markup.perl";

my $__THREADS = 0;

# Parameter for effective reference length when computing BLEU score
# Default is to use shortest reference
# Use "--shortest" to use shortest reference length
# Use "--average" to use average reference length
# Use "--closest" to use closest reference length
# Only one between --shortest, --average and --closest can be set
# If more than one choice the defualt (--shortest) is used
my $___SHORTEST = 0;
my $___AVERAGE = 0;
my $___CLOSEST = 0;

# Use "--nocase" to compute case-insensitive scores
my $___NOCASE = 0;

# Use "--nonorm" to non normalize translation before computing scores
my $___NONORM = 0;

# set 0 if input type is text, set 1 if input type is confusion network
my $___INPUTTYPE = 0;


my $mertdir = undef; # path to new mert directory
my $mertargs = undef; # args to pass through to mert & extractor
my $mertmertargs = undef; # args to pass through to mert only
my $extractorargs = undef; # args to pass through to extractor only
my $proargs = undef; # args to pass through to pro only

# Args to pass through to batch mira only.  This flags is useful to
# change MIRA's hyperparameters such as regularization parameter C,
# BLEU decay factor, and the number of iterations of MIRA.
my $batch_mira_args = undef;

my $filtercmd = undef; # path to filter-model-given-input.pl
my $filterfile = undef;
my $qsubwrapper = undef;
my $moses_parallel_cmd = undef;
my $old_sge = 0; # assume sge<6.0
my $___CONFIG_ORIG = undef; # pathname to startup ini file before filtering
my $___ACTIVATE_FEATURES = undef; # comma-separated (or blank-separated) list of features to work on
                                  # if undef work on all features
                                  # (others are fixed to the starting values)
my $___RANGES = undef;
my $___USE_CONFIG_WEIGHTS_FIRST = 0; # use weights in configuration file for first iteration
my $prev_aggregate_nbl_size = -1; # number of previous step to consider when loading data (default =-1)
                                  # -1 means all previous, i.e. from iteration 1
                                  # 0 means no previous data, i.e. from actual iteration
                                  # 1 means 1 previous data , i.e. from the actual iteration and from the previous one
                                  # and so on
my $maximum_iterations = 25;

use Getopt::Long;
GetOptions(
  "working-dir=s" => \$___WORKING_DIR,
  "input=s" => \$___DEV_F,
  "inputtype=i" => \$___INPUTTYPE,
  "refs=s" => \$___DEV_E,
  "decoder=s" => \$___DECODER,
  "config=s" => \$___CONFIG,
  "nbest=i" => \$___N_BEST_LIST_SIZE,
  "lattice-samples=i" => \$___LATTICE_SAMPLES,
  "queue-flags=s" => \$queue_flags,
  "jobs=i" => \$___JOBS,
  "decoder-flags=s" => \$___DECODER_FLAGS,
  "continue" => \$continue,
  "skip-decoder" => \$skip_decoder,
  "shortest" => \$___SHORTEST,
  "average" => \$___AVERAGE,
  "closest" => \$___CLOSEST,
  "nocase" => \$___NOCASE,
  "nonorm" => \$___NONORM,
  "help" => \$usage,
  "verbose" => \$verbose,
  "mertdir=s" => \$mertdir,
  "mertargs=s" => \$mertargs,
  "extractorargs=s" => \$extractorargs,
  "proargs=s" => \$proargs,
  "mertmertargs=s" => \$mertmertargs,
  "rootdir=s" => \$SCRIPTS_ROOTDIR,
  "filtercmd=s" => \$filtercmd, # allow to override the default location
  "filterfile=s" => \$filterfile, # input to filtering script (useful for lattices/confnets)
  "qsubwrapper=s" => \$qsubwrapper, # allow to override the default location
  "mosesparallelcmd=s" => \$moses_parallel_cmd, # allow to override the default location
  "old-sge" => \$old_sge, #passed to moses-parallel
  "filter-phrase-table!" => \$___FILTER_PHRASE_TABLE, # (dis)allow of phrase tables
  "predictable-seeds" => \$___PREDICTABLE_SEEDS, # make random restarts deterministic
  "historic-bests" => \$___START_WITH_HISTORIC_BESTS, # use best settings from all previous iterations as starting points
  "random-directions" => \$___RANDOM_DIRECTIONS, # search only in random directions
  "number-of-random-directions=i" => \$___NUM_RANDOM_DIRECTIONS, # number of random directions
  "random-restarts=i" => \$___RANDOM_RESTARTS, # number of random restarts
  "return-best-dev" => \$___RETURN_BEST_DEV, # return the best weights according to dev, not the last
  "activate-features=s" => \$___ACTIVATE_FEATURES, #comma-separated (or blank-separated) list of features to work on (others are fixed to the starting values)
  "range=s@" => \$___RANGES,
  "use-config-weights-for-first-run" => \$___USE_CONFIG_WEIGHTS_FIRST, # use the weights in the configuration file when running the decoder for the first time
  "prev-aggregate-nbestlist=i" => \$prev_aggregate_nbl_size, #number of previous step to consider when loading data (default =-1, i.e. all previous)
  "maximum-iterations=i" => \$maximum_iterations,
  "pairwise-ranked" => \$___PAIRWISE_RANKED_OPTIMIZER,
  "pro-starting-point" => \$___PRO_STARTING_POINT,
  "historic-interpolation=f" => \$___HISTORIC_INTERPOLATION,
  "batch-mira" => \$___BATCH_MIRA,
  "batch-mira-args=s" => \$batch_mira_args,
  "promix-training=s" => \$__PROMIX_TRAINING,
  "promix-table=s" => \@__PROMIX_TABLES,
  "threads=i" => \$__THREADS
) or exit(1);

# the 4 required parameters can be supplied on the command line directly
# or using the --options
if (scalar @ARGV == 4) {
  # required parameters: input_file references_basename decoder_executable
  $___DEV_F = shift;
  $___DEV_E = shift;
  $___DECODER = shift;
  $___CONFIG = shift;
}

if ($usage || !defined $___DEV_F || !defined $___DEV_E || !defined $___DECODER || !defined $___CONFIG) {
  print STDERR "usage: $0 input-text references decoder-executable decoder.ini
Options:
  --working-dir=mert-dir ... where all the files are created
  --nbest=100            ... how big nbestlist to generate
  --lattice-samples      ... how many lattice samples (Chatterjee & Cancedda, emnlp 2010)
  --jobs=N               ... set this to anything to run moses in parallel
  --mosesparallelcmd=STR ... use a different script instead of moses-parallel
  --queue-flags=STRING   ... anything you with to pass to qsub, eg.
                             '-l ws06osssmt=true'. The default is: '-hard'
                             To reset the parameters, please use
                             --queue-flags=' '
                             (i.e. a space between the quotes).
  --decoder-flags=STRING ... extra parameters for the decoder
  --continue             ... continue from the last successful iteration
  --skip-decoder         ... skip the decoder run for the first time,
                             assuming that we got interrupted during
                             optimization
  --shortest --average --closest
                         ... Use shortest/average/closest reference length
                             as effective reference length (mutually exclusive)
  --nocase               ... Do not preserve case information; i.e.
                             case-insensitive evaluation (default is false).
  --nonorm               ... Do not use text normalization (flag is not active,
                             i.e. text is NOT normalized)
  --filtercmd=STRING     ... path to filter-model-given-input.pl
  --filterfile=STRING    ... path to alternative to input-text for filtering
                             model. useful for lattice decoding
  --rootdir=STRING       ... where do helpers reside (if not given explicitly)
  --mertdir=STRING       ... path to new mert implementation
  --mertargs=STRING      ... extra args for both extractor and mert
  --extractorargs=STRING ... extra args for extractor only
  --mertmertargs=STRING  ... extra args for mert only
  --scorenbestcmd=STRING ... path to score-nbest.py
  --old-sge              ... passed to parallelizers, assume Grid Engine < 6.0
  --inputtype=[0|1|2]    ... Handle different input types: (0 for text,
                             1 for confusion network, 2 for lattices,
                             default is 0)
  --no-filter-phrase-table ... disallow filtering of phrase tables
                              (useful if binary phrase tables are available)
  --random-restarts=INT  ... number of random restarts (default: 20)
  --predictable-seeds    ... provide predictable seeds to mert so that random
                             restarts are the same on every run
  --range=tm:0..1,-1..1  ... specify min and max value for some features
                             --range can be repeated as needed.
                             The order of the various --range specifications
                             is important only within a feature name.
                             E.g.:
                               --range=tm:0..1,-1..1 --range=tm:0..2
                             is identical to:
                               --range=tm:0..1,-1..1,0..2
                             but not to:
                               --range=tm:0..2 --range=tm:0..1,-1..1
  --activate-features=STRING  ... comma-separated list of features to optimize,
                                  others are fixed to the starting values
                                  default: optimize all features
                                  example: tm_0,tm_4,d_0
  --prev-aggregate-nbestlist=INT ... number of previous step to consider when
                                     loading data (default = $prev_aggregate_nbl_size)
                                    -1 means all previous, i.e. from iteration 1
                                     0 means no previous data, i.e. only the
                                       current iteration
                                     N means this and N previous iterations

  --maximum-iterations=ITERS ... Maximum number of iterations. Default: $maximum_iterations
  --return-best-dev          ... Return the weights according to dev bleu, instead of returning
                                 the last iteration
  --random-directions               ... search only in random directions
  --number-of-random-directions=int ... number of random directions
                                        (also works with regular optimizer, default: 0)
  --pairwise-ranked         ... Use PRO for optimisation (Hopkins and May, emnlp 2011)
  --pro-starting-point      ... Use PRO to get a starting point for MERT
  --batch-mira              ... Use Batch MIRA for optimisation (Cherry and Foster, NAACL 2012)
  --batch-mira-args=STRING  ... args to pass through to batch MIRA. This flag is useful to
                                change MIRA's hyperparameters such as regularization parameter C,
                                BLEU decay factor, and the number of iterations of MIRA.
  --promix-training=STRING  ... PRO-based mixture model training (Haddow, NAACL 2013)
  --promix-tables=STRING    ... Phrase tables for PRO-based mixture model training.
  --threads=NUMBER          ... Use multi-threaded mert (must be compiled in).
  --historic-interpolation  ... Interpolate optimized weights with prior iterations' weight
                                (parameter sets factor [0;1] given to current weights)
";
  exit 1;
}


# Check validity of input parameters and set defaults if needed

print STDERR "Using SCRIPTS_ROOTDIR: $SCRIPTS_ROOTDIR\n";

# path of script for filtering phrase tables and running the decoder
$filtercmd = File::Spec->catfile($SCRIPTS_ROOTDIR, "training", "filter-model-given-input.pl") if !defined $filtercmd;

if ( ! -x $filtercmd && ! $___FILTER_PHRASE_TABLE) {
  warn "Filtering command not found: $filtercmd.";
  warn "Use --filtercmd=PATH to specify a valid one or --no-filter-phrase-table";
  exit 1;
}

$qsubwrapper = File::Spec->catfile($SCRIPTS_ROOTDIR, "generic", "qsub-wrapper.pl") if !defined $qsubwrapper;

$moses_parallel_cmd = File::Spec->catfile($SCRIPTS_ROOTDIR, "generic", "moses-parallel.pl")
  if !defined $moses_parallel_cmd;

if (!defined $mertdir) {
  $mertdir = File::Spec->catfile(File::Basename::dirname($SCRIPTS_ROOTDIR), "bin");
  die "mertdir does not exist: $mertdir" if ! -x $mertdir;
  print STDERR "Assuming --mertdir=$mertdir\n";
}

my $mert_extract_cmd = File::Spec->catfile($mertdir, "extractor");
my $mert_mert_cmd    = File::Spec->catfile($mertdir, "mert");
my $mert_pro_cmd     = File::Spec->catfile($mertdir, "pro");
my $mert_mira_cmd    = File::Spec->catfile($mertdir, "kbmira");
my $mert_eval_cmd    = File::Spec->catfile($mertdir, "evaluator");

die "Not executable: $mert_extract_cmd" if ! -x $mert_extract_cmd;
die "Not executable: $mert_mert_cmd"    if ! -x $mert_mert_cmd;
die "Not executable: $mert_pro_cmd"     if ! -x $mert_pro_cmd;
die "Not executable: $mert_mira_cmd"    if ! -x $mert_mira_cmd;
die "Not executable: $mert_eval_cmd"    if ! -x $mert_eval_cmd;

my $pro_optimizer = File::Spec->catfile($mertdir, "megam_i686.opt");  # or set to your installation

if (($___PAIRWISE_RANKED_OPTIMIZER || $___PRO_STARTING_POINT) && ! -x $pro_optimizer) {
  print "Could not find $pro_optimizer, installing it in $mertdir\n";
  my $megam_url = "http://hal3.name/megam";
  if (&is_mac_osx()) {
    die "Error: Sorry for Mac OS X users! Please get the source code of megam and compile by hand. Please see $megam_url for details.";
  }

  `cd $mertdir; wget $megam_url/megam_i686.opt.gz;`;
  `gunzip $pro_optimizer.gz`;
  `chmod +x $pro_optimizer`;
  die("ERROR: Installation of megam_i686.opt failed! Install by hand from $megam_url") unless -x $pro_optimizer;
}

if ($__PROMIX_TRAINING) {
  die "Not executable $__PROMIX_TRAINING" unless -x $__PROMIX_TRAINING;
  die "For promix training, specify the tables using --promix-table arguments" unless @__PROMIX_TABLES;
  die "For mixture model, need at least 2 tables" unless scalar(@__PROMIX_TABLES) > 1;
  
  for my $TABLE (@__PROMIX_TABLES) {
    die "Phrase table $TABLE not found" unless -r $TABLE;
  }
  die "To use promix training, need to specify a filter and binarisation command" unless   $filtercmd =~ /Binarizer/;
}

$mertargs = "" if !defined $mertargs;

my $scconfig = undef;
if ($mertargs =~ /\-\-scconfig\s+(.+?)(\s|$)/) {
  $scconfig = $1;
  $scconfig =~ s/\,/ /g;
  $mertargs =~ s/\-\-scconfig\s+(.+?)(\s|$)//;
}

# handling reference lengh strategy
$scconfig .= &setup_reference_length_type();

# handling case-insensitive flag
$scconfig .= &setup_case_config();

$scconfig =~ s/^\s+//;
$scconfig =~ s/\s+$//;
$scconfig =~ s/\s+/,/g;

$scconfig = "--scconfig $scconfig" if ($scconfig);

my $mert_extract_args = $mertargs;
$mert_extract_args .= " $scconfig";

$extractorargs = "" unless $extractorargs;
$mert_extract_args .= " $extractorargs";

$mertmertargs = "" if !defined $mertmertargs;

$proargs = "" unless $proargs;

my $mert_mert_args = "$mertargs $mertmertargs";
$mert_mert_args =~ s/\-+(binary|b)\b//;
$mert_mert_args .= " $scconfig";
if ($___ACTIVATE_FEATURES) {
  $mert_mert_args .= " -o \"$___ACTIVATE_FEATURES\"";
}

my ($just_cmd_filtercmd, $x) = split(/ /, $filtercmd);
die "Not executable: $just_cmd_filtercmd" if ! -x $just_cmd_filtercmd;
die "Not executable: $moses_parallel_cmd" if defined $___JOBS && ! -x $moses_parallel_cmd;
die "Not executable: $qsubwrapper"        if defined $___JOBS && ! -x $qsubwrapper;
die "Not executable: $___DECODER"         if ! -x $___DECODER;

my $input_abs = ensure_full_path($___DEV_F);
die "File not found: $___DEV_F (interpreted as $input_abs)." if ! -e $input_abs;

$___DEV_F = $input_abs;

# Option to pass to qsubwrapper and moses-parallel
my $pass_old_sge = $old_sge ? "-old-sge" : "";

my $decoder_abs = ensure_full_path($___DECODER);
die "File not executable: $___DECODER (interpreted as $decoder_abs)."
  if ! -x $decoder_abs;
$___DECODER = $decoder_abs;

my $ref_abs = ensure_full_path($___DEV_E);
# check if English dev set (reference translations) exist and store a list of all references
my @references;
if (-e $ref_abs) {
  push @references, $ref_abs;
} else {
  # if multiple file, get a full list of the files
  my $part = 0;
  if (! -e $ref_abs . "0" && -e $ref_abs . ".ref0") {
    $ref_abs .= ".ref";
  }
  while (-e $ref_abs . $part) {
    push @references, $ref_abs . $part;
    $part++;
  }
  die("Reference translations not found: $___DEV_E (interpreted as $ref_abs)") unless $part;
}

my $config_abs = ensure_full_path($___CONFIG);
die "File not found: $___CONFIG (interpreted as $config_abs)." if ! -e $config_abs;
$___CONFIG = $config_abs;

# moses should use our config
if ($___DECODER_FLAGS =~ /(^|\s)-(config|f) /
    || $___DECODER_FLAGS =~ /(^|\s)-(ttable-file|t) /
    || $___DECODER_FLAGS =~ /(^|\s)-(distortion-file) /
    || $___DECODER_FLAGS =~ /(^|\s)-(generation-file) /
    || $___DECODER_FLAGS =~ /(^|\s)-(lmodel-file) /
    || $___DECODER_FLAGS =~ /(^|\s)-(global-lexical-file) /
  ) {
  die "It is forbidden to supply any of -config, -ttable-file, -distortion-file, -generation-file or -lmodel-file in the --decoder-flags.\nPlease use only the --config option to give the config file that lists all the supplementary files.";
}

# as weights are normalized in the next steps (by cmert)
# normalize initial LAMBDAs, too
my $need_to_normalize = 1;

#store current directory and create the working directory (if needed)
my $cwd = Cwd::getcwd();

mkpath($___WORKING_DIR);

# open local scope
{

#chdir to the working directory
chdir($___WORKING_DIR) or die "Can't chdir to $___WORKING_DIR";

# fixed file names
my $mert_outfile = "mert.out";
my $mert_logfile = "mert.log";
my $weights_in_file = "init.opt";
my $weights_out_file = "weights.txt";
my $finished_step_file = "finished_step.txt";

# set start run
my $start_run = 1;
my $bestpoint = undef;
my $devbleu = undef;
my $sparse_weights_file = undef;

my $prev_feature_file = undef;
my $prev_score_file = undef;
my $prev_init_file = undef;
my @allnbests;

# If we're doing promix training, need to make sure the appropriate
# tables are in place
my @_PROMIX_TABLES_BIN;
if ($__PROMIX_TRAINING) {
  print STDERR "Training mixture model using promix\n";
  for (my $i = 0; $i < scalar(@__PROMIX_TABLES); ++$i) {
    # Create filtered, binarised tables
    my $filtered_config = "moses_$i.ini";
    substitute_ttable($___CONFIG, $filtered_config, $__PROMIX_TABLES[$i]); 
    #TODO: Remove reordering table from config, as we don't need to filter
    # and binarise it.
    my $filtered_path = "filtered_$i";
    my $___FILTER_F  = $___DEV_F;
    $___FILTER_F = $filterfile if (defined $filterfile);
    my $cmd = "$filtercmd ./$filtered_path $filtered_config $___FILTER_F";
    &submit_or_exec($cmd, "filterphrases_$i.out", "filterphrases_$i.err");
    push (@_PROMIX_TABLES_BIN,"$filtered_path/phrase-table.0-0.1.1");
  }
}
 
if ($___FILTER_PHRASE_TABLE) {
  my $outdir = "filtered";
  if (-e "$outdir/moses.ini") {
    print STDERR "Assuming the tables are already filtered, reusing $outdir/moses.ini\n";
  } else {
    # filter the phrase tables with respect to input, use --decoder-flags
    print STDERR "filtering the phrase tables... ".`date`;
    my $___FILTER_F  = $___DEV_F;
    $___FILTER_F = $filterfile if (defined $filterfile);
    my $cmd = "$filtercmd ./$outdir $___CONFIG $___FILTER_F";
    &submit_or_exec($cmd, "filterphrases.out", "filterphrases.err");
  }

  # make a backup copy of startup ini filepath
  $___CONFIG_ORIG = $___CONFIG;
  # the decoder should now use the filtered model
  $___CONFIG = "$outdir/moses.ini";
} else{
  # do not filter phrase tables (useful if binary phrase tables are available)
  # use the original configuration file
  $___CONFIG_ORIG = $___CONFIG;
}

# we run moses to check validity of moses.ini and to obtain all the feature
# names
my $featlist = get_featlist_from_moses($___CONFIG);
$featlist = insert_ranges_to_featlist($featlist, $___RANGES);

# Mark which features are disabled:
if (defined $___ACTIVATE_FEATURES) {
  my %enabled = map { ($_, 1) } split /[, ]+/, $___ACTIVATE_FEATURES;
  my %cnt;
  for (my $i = 0; $i < scalar(@{$featlist->{"names"}}); $i++) {
    my $name = $featlist->{"names"}->[$i];
    $cnt{$name} = 0 if !defined $cnt{$name};
    $featlist->{"enabled"}->[$i] = $enabled{$name . "_" . $cnt{$name}};
    $cnt{$name}++;
  }
} else {
  # all enabled
  for(my $i = 0; $i < scalar(@{$featlist->{"names"}}); $i++) {
    $featlist->{"enabled"}->[$i] = 1;
  }
}

print STDERR "MERT starting values and ranges for random generation:\n";
for (my $i = 0; $i < scalar(@{$featlist->{"names"}}); $i++) {
  my $name = $featlist->{"names"}->[$i];
  my $val = $featlist->{"values"}->[$i];
  my $min = $featlist->{"mins"}->[$i];
  my $max = $featlist->{"maxs"}->[$i];
  my $enabled = $featlist->{"enabled"}->[$i];
  printf STDERR "  %5s = %7.3f", $name, $val;
  if ($enabled) {
    printf STDERR " (%5.2f .. %5.2f)\n", $min, $max;
  } else {
    print STDERR " --- inactive, not optimized ---\n";
  }
}

if ($continue) {
  # getting the last finished step
  print STDERR "Trying to continue an interrupted optimization.\n";
  open my $fh, '<', $finished_step_file or die "$finished_step_file: $!";
  my $step = <$fh>;
  chomp $step;
  close $fh;

  print STDERR "Last finished step is $step\n";

  # getting the first needed step
  my $firststep;
  if ($prev_aggregate_nbl_size == -1) {
    $firststep = 1;
  } else {
    $firststep = $step - $prev_aggregate_nbl_size + 1;
    $firststep = ($firststep > 0) ? $firststep : 1;
  }

  #checking if all needed data are available
  if ($firststep <= $step) {
    print STDERR "First previous needed data index is $firststep\n";
    print STDERR "Checking whether all needed data (from step $firststep to step $step) are available\n";

    for (my $prevstep = $firststep; $prevstep <= $step; $prevstep++) {
        print STDERR "Checking whether data of step $prevstep are available\n";
      if (! -e "run$prevstep.features.dat") {
          die "Can't start from step $step, because run$prevstep.features.dat was not found!";
      } else {
        if (defined $prev_feature_file) {
          $prev_feature_file = "${prev_feature_file},run$prevstep.features.dat";
        } else {
          $prev_feature_file = "run$prevstep.features.dat";
        }
      }
      if (! -e "run$prevstep.scores.dat") {
          die "Can't start from step $step, because run$prevstep.scores.dat was not found!";
      } else {
        if (defined $prev_score_file) {
          $prev_score_file = "${prev_score_file},run$prevstep.scores.dat";
        } else {
          $prev_score_file = "run$prevstep.scores.dat";
        }
      }
      if (! -e "run$prevstep.${weights_in_file}") {
          die "Can't start from step $step, because run$prevstep.${weights_in_file} was not found!";
      } else{
        if (defined $prev_init_file) {
          $prev_init_file = "${prev_init_file},run$prevstep.${weights_in_file}";
        } else{
          $prev_init_file = "run$prevstep.${weights_in_file}";
        }
      }
    }
    if (! -e "run$step.weights.txt") {
      die "Can't start from step $step, because run$step.weights.txt was not found!";
    }
    if (! -e "run$step.$mert_logfile") {
      die "Can't start from step $step, because run$step.$mert_logfile was not found!";
    }
    if (! -e "run$step.best$___N_BEST_LIST_SIZE.out.gz") {
      die "Can't start from step $step, because run$step.best$___N_BEST_LIST_SIZE.out.gz was not found!";
    }
    print STDERR "All needed data are available\n";
    print STDERR "Loading information from last step ($step)\n";

    my %dummy; # sparse features
    ($bestpoint, $devbleu) = &get_weights_from_mert("run$step.$mert_outfile","run$step.$mert_logfile", scalar @{$featlist->{"names"}}, \%dummy);
    die "Failed to parse mert.log, missed Best point there."
      if !defined $bestpoint || !defined $devbleu;
    print "($step) BEST at $step $bestpoint => $devbleu at ".`date`;
    my @newweights = split /\s+/, $bestpoint;

    # Sanity check: order of lambdas must match
    sanity_check_order_of_lambdas($featlist,
      "gunzip -c < run$step.best$___N_BEST_LIST_SIZE.out.gz |");

    # update my cache of lambda values
    $featlist->{"values"} = \@newweights;
  } else {
    print STDERR "No previous data are needed\n";
  }
  $start_run = $step + 1;
}

###### MERT MAIN LOOP

my $run = $start_run - 1;

my $oldallsorted    = undef;
my $allsorted       = undef;
my $nbest_file      = undef;
my $lsamp_file      = undef; # Lattice samples
my $orig_nbest_file = undef; # replaced if lattice sampling
# For mixture modelling
my @promix_weights;
my $num_mixed_phrase_features;
my $interpolated_config;
my $uninterpolated_config; # backup of config without interpolated ttable

while (1) {
  $run++;
  if ($maximum_iterations && $run > $maximum_iterations) {
    print "Maximum number of iterations exceeded - stopping\n";
    last;
  }
  print "run $run start at ".`date`;

  if ($__PROMIX_TRAINING) {
    # Need to create an ini file for the interpolated phrase table
    if (!@promix_weights) {
      # Create initial weights, distributing evenly between tables
      # total number of weights is 1 less than number of phrase features, multiplied
      # by the number of tables
      $num_mixed_phrase_features = (grep { $_ eq 'tm' } @{$featlist->{"names"}}) - 1;
  
      @promix_weights = (1.0/scalar(@__PROMIX_TABLES)) x 
        ($num_mixed_phrase_features * scalar(@__PROMIX_TABLES));
    }
    
    # backup orig config, so we always add the table into it
    $uninterpolated_config= $___CONFIG unless $uninterpolated_config; 

    # Interpolation
    my $interpolated_phrase_table = "interpolate";
    for my $itable (@_PROMIX_TABLES_BIN) {
      $interpolated_phrase_table .= " 1:$itable";
    }
    
    # Create an ini file for the interpolated phrase table
    $interpolated_config ="moses.interpolated.ini"; 
    substitute_ttable($uninterpolated_config, $interpolated_config, $interpolated_phrase_table, "99");

    # Append the multimodel weights
    open(ITABLE,">>$interpolated_config") || die "Failed to append weights to $interpolated_config";
    print ITABLE "\n";
    print ITABLE "[weight-t-multimodel]\n";
    #for my $feature (0..($num_mixed_phrase_features-1)) {
    #  for my $table (0..(scalar(@__PROMIX_TABLES)-1)) {
    #    print ITABLE $promix_weights[$table * $num_mixed_phrase_features + $feature];
    #    print ITABLE "\n";
    #  }
    #}
    for my $iweight (@promix_weights) {
      print ITABLE $iweight . "\n";
    }

    close ITABLE;

    # the decoder should now use the interpolated model
    $___CONFIG = "$interpolated_config";

  }

  # run beamdecoder with option to output nbestlists
  # the end result should be (1) @NBEST_LIST, a list of lists; (2) @SCORE, a list of lists of lists


  # In case something dies later, we might wish to have a copy
  create_config($___CONFIG, "./run$run.moses.ini", $featlist, $run, (defined $devbleu ? $devbleu : "--not-estimated--"), $sparse_weights_file);

  # Save dense weights to simplify best dev recovery
  {
    my $densefile = "run$run.dense";
    my @vals = @{$featlist->{"values"}};
    my @names = @{$featlist->{"names"}};
    open my $denseout, '>', $densefile or die "Can't write $densefile (WD now $___WORKING_DIR)";
    for (my $i = 0; $i < scalar(@{$featlist->{"names"}}); $i++) {
        print $denseout "$names[$i]= $vals[$i]\n";
    }
    close $denseout;
  }

  # skip running the decoder if the user wanted
  if (! $skip_decoder) {
    print "($run) run decoder to produce n-best lists\n";
    ($nbest_file, $lsamp_file) = run_decoder($featlist, $run, $need_to_normalize);
    $need_to_normalize = 0;
    if ($___LATTICE_SAMPLES) {
      my $combined_file = "$nbest_file.comb";
      safesystem("sort -k1,1n $nbest_file $lsamp_file > $combined_file") or
          die("failed to merge nbest and lattice samples");
      safesystem("gzip -f $nbest_file; gzip -f $lsamp_file") or
          die "Failed to gzip nbests and lattice samples";
      $orig_nbest_file = "$nbest_file.gz";
      $orig_nbest_file = "$nbest_file.gz";
      $lsamp_file      = "$lsamp_file.gz";
      $lsamp_file      = "$lsamp_file.gz";
      $nbest_file      = "$combined_file";
    }
    safesystem("gzip -f $nbest_file") or die "Failed to gzip run*out";
    $nbest_file = $nbest_file.".gz";
  } else {
    $nbest_file = "run$run.best$___N_BEST_LIST_SIZE.out.gz";
    print "skipped decoder run $run\n";
    $skip_decoder = 0;
    $need_to_normalize = 0;
  }

  # extract score statistics and features from the nbest lists
  print STDERR "Scoring the nbestlist.\n";

  my $base_feature_file = "features.dat";
  my $base_score_file   = "scores.dat";
  my $feature_file      = "run$run.${base_feature_file}";
  my $score_file        = "run$run.${base_score_file}";

  my $cmd = "$mert_extract_cmd $mert_extract_args --scfile $score_file --ffile $feature_file -r " . join(",", @references) . " -n $nbest_file";
  $cmd .= " -d" if $__PROMIX_TRAINING; # Allow duplicates
  # remove segmentation
  $cmd .= " -l $__REMOVE_SEGMENTATION" if  $__PROMIX_TRAINING;
  $cmd = &create_extractor_script($cmd, $___WORKING_DIR);
  &submit_or_exec($cmd, "extract.out","extract.err");

  # Create the initial weights file for mert: init.opt
  my @MIN  = @{$featlist->{"mins"}};
  my @MAX  = @{$featlist->{"maxs"}};
  my @CURR = @{$featlist->{"values"}};
  my @NAME = @{$featlist->{"names"}};

  open my $out, '>', $weights_in_file or die "Can't write $weights_in_file (WD now $___WORKING_DIR)";
  print $out join(" ", @CURR) . "\n";
  print $out join(" ", @MIN) . "\n";  # this is where we could pass MINS
  print $out join(" ", @MAX) . "\n";  # this is where we could pass MAXS
  close $out;
  # print join(" ", @NAME)."\n";

  # make a backup copy labelled with this run number
  safesystem("\\cp -f $weights_in_file run$run.$weights_in_file") or die;

  my $DIM = scalar(@CURR); # number of lambdas

  # run mert
  $cmd = "$mert_mert_cmd -d $DIM $mert_mert_args";

  my $mert_settings = " -n $___RANDOM_RESTARTS";
  my $seed_settings = "";
  if ($___PREDICTABLE_SEEDS) {
    my $seed = $run * 1000;
    $seed_settings .= " -r $seed";
  }
  $mert_settings .= $seed_settings;
  if ($___RANDOM_DIRECTIONS) {
    if ($___NUM_RANDOM_DIRECTIONS == 0) {
      $mert_settings .= " -m 50";
    }
    $mert_settings .= " -t random-direction";
  }
  if ($___NUM_RANDOM_DIRECTIONS) {
    $mert_settings .= " -m $___NUM_RANDOM_DIRECTIONS";
  }
  if ($__THREADS) {
    $mert_settings .= " --threads $__THREADS";
  }

  my $ffiles = "";
  my $scfiles = "";

  if (defined $prev_feature_file) {
    $ffiles = "$prev_feature_file,$feature_file";
  } else{
    $ffiles = "$feature_file";
  }

  if (defined $prev_score_file) {
    $scfiles = "$prev_score_file,$score_file";
  } else{
    $scfiles = "$score_file";
  }

  my $mira_settings = "";
  if ($___BATCH_MIRA && $batch_mira_args) {
    $mira_settings .= "$batch_mira_args ";
  }

  $mira_settings .= " --dense-init run$run.$weights_in_file";
  if (-e "run$run.sparse-weights") {
    $mira_settings .= " --sparse-init run$run.sparse-weights";
  }
  my $file_settings = " --ffile $ffiles --scfile $scfiles";
  my $pro_file_settings = "--ffile " . join(" --ffile ", split(/,/, $ffiles)) .
                          " --scfile " .  join(" --scfile ", split(/,/, $scfiles));

  push @allnbests, $nbest_file;
  my $promix_file_settings = 
                          "--scfile " .  join(" --scfile ", split(/,/, $scfiles)) .
                          " --nbest " . join(" --nbest ", @allnbests);

  if ($___START_WITH_HISTORIC_BESTS && defined $prev_init_file) {
    $file_settings .= " --ifile $prev_init_file,run$run.$weights_in_file";
  } else {
    $file_settings .= " --ifile run$run.$weights_in_file";
  }

  $cmd .= $file_settings;

  my %sparse_weights; # sparse features
  my $pro_optimizer_cmd = "$pro_optimizer $megam_default_options run$run.pro.data";
  if ($___PAIRWISE_RANKED_OPTIMIZER) {  # pro optimization
    $cmd = "$mert_pro_cmd $proargs $seed_settings $pro_file_settings -o run$run.pro.data ; echo 'not used' > $weights_out_file; $pro_optimizer_cmd";
    &submit_or_exec($cmd, $mert_outfile, $mert_logfile);
  } elsif ($___PRO_STARTING_POINT) {  # First, run pro, then mert
    # run pro...
    my $pro_cmd = "$mert_pro_cmd $proargs $seed_settings $pro_file_settings -o run$run.pro.data ; $pro_optimizer_cmd";
    &submit_or_exec($pro_cmd, "run$run.pro.out", "run$run.pro.err");
    # ... get results ...
    ($bestpoint,$devbleu) = &get_weights_from_mert("run$run.pro.out","run$run.pro.err",scalar @{$featlist->{"names"}},\%sparse_weights, \@promix_weights);
    # Get the pro outputs ready for mert. Add the weight ranges,
    # and a weight and range for the single sparse feature
    $cmd =~ s/--ifile (\S+)/--ifile run$run.init.pro/;
    open(MERT_START,$1);
    open(PRO_START,">run$run.init.pro");
    print PRO_START $bestpoint." 1\n";
    my $mert_line = <MERT_START>;
    $mert_line = <MERT_START>;
    chomp $mert_line;
    print PRO_START $mert_line." 0\n";
    $mert_line = <MERT_START>;
    chomp $mert_line;
    print PRO_START $mert_line." 1\n";
    close(PRO_START);

    # Write the sparse weights to file so mert can use them
    open(SPARSE_WEIGHTS,">run$run.merge-weights");
    foreach my $fname (keys %sparse_weights) {
      print SPARSE_WEIGHTS "$fname $sparse_weights{$fname}\n";
    }
    close(SPARSE_WEIGHTS);
    $cmd = $cmd." --sparse-weights run$run.merge-weights";

    # ... and run mert
    $cmd =~ s/(--ifile \S+)/$1,run$run.init.pro/;
    &submit_or_exec($cmd . $mert_settings, $mert_outfile, $mert_logfile);
  } elsif ($___BATCH_MIRA) { # batch MIRA optimization
    safesystem("echo 'not used' > $weights_out_file") or die;
    $cmd = "$mert_mira_cmd $mira_settings $seed_settings $pro_file_settings -o $mert_outfile";
    &submit_or_exec($cmd, "run$run.mira.out", $mert_logfile);
  } elsif ($__PROMIX_TRAINING) {
    # PRO trained  mixture model
    safesystem("echo 'not used' > $weights_out_file") or die;
    $cmd = "$__PROMIX_TRAINING $promix_file_settings";
    $cmd .= " -t mix ";
    $cmd .= join(" ", map {"-p $_"} @_PROMIX_TABLES_BIN);
    $cmd .= " -i $___DEV_F";
    print "Starting promix optimisation at " . `date`;
    &submit_or_exec($cmd, "$mert_outfile", $mert_logfile);
    print "Finished promix optimisation at " . `date`;
  } else {  # just mert
    &submit_or_exec($cmd . $mert_settings, $mert_outfile, $mert_logfile);
  } 

  die "Optimization failed, file $weights_out_file does not exist or is empty"
    if ! -s $weights_out_file;

  # backup copies
  safesystem("\\cp -f extract.err run$run.extract.err") or die;
  safesystem("\\cp -f extract.out run$run.extract.out") or die;
  safesystem("\\cp -f $mert_outfile run$run.$mert_outfile") or die;
  safesystem("\\cp -f $mert_logfile run$run.$mert_logfile") or die;
  safesystem("touch $mert_logfile run$run.$mert_logfile") or die;
  safesystem("\\cp -f $weights_out_file run$run.$weights_out_file") or die; # this one is needed for restarts, too
  if ($__PROMIX_TRAINING) {
    safesystem("\\cp -f $interpolated_config run$run.$interpolated_config") or die;
  }

  print "run $run end at ".`date`;

  ($bestpoint,$devbleu) = &get_weights_from_mert("run$run.$mert_outfile","run$run.$mert_logfile",scalar @{$featlist->{"names"}},\%sparse_weights,\@promix_weights);
  my $merge_weight = 0;
  print "New mixture weights: " . join(" ", @promix_weights) . "\n";

  die "Failed to parse mert.log, missed Best point there."
    if !defined $bestpoint || !defined $devbleu;

  print "($run) BEST at $run: $bestpoint => $devbleu at ".`date`;

  # update my cache of lambda values
  my @newweights = split /\s+/, $bestpoint;

  if ($___PRO_STARTING_POINT) {
    $merge_weight = pop @newweights;
  }

  # interpolate with prior's interation weight, if historic-interpolation is specified
  if ($___HISTORIC_INTERPOLATION>0 && $run>3) {
    my %historic_sparse_weights;
    if (-e "run$run.sparse-weights") {
      open my $sparse_fh, '<', "run$run.sparse-weights" or die "run$run.sparse-weights: $!";
      while (<$sparse_fh>) {
        chop;
        my ($feature, $weight) = split;
        $historic_sparse_weights{$feature} = $weight;
      }
      close $sparse_fh;
    }
    my $prev = $run - 1;
    my @historic_weights = split /\s+/, `cat run$prev.$weights_out_file`;
    for(my $i = 0; $i < scalar(@newweights); $i++) {
      $newweights[$i] = $___HISTORIC_INTERPOLATION * $newweights[$i] + (1 - $___HISTORIC_INTERPOLATION) * $historic_weights[$i];
    }
    print "interpolate with " . join(",", @historic_weights) . " to " . join(",", @newweights);
    foreach (keys %sparse_weights) {
      $sparse_weights{$_} *= $___HISTORIC_INTERPOLATION;
      #print STDERR "sparse_weights{$_} *= $___HISTORIC_INTERPOLATION -> $sparse_weights{$_}\n";
    }
    foreach (keys %historic_sparse_weights) {
      $sparse_weights{$_} += (1 - $___HISTORIC_INTERPOLATION) * $historic_sparse_weights{$_};
      #print STDERR "sparse_weights{$_} += (1-$___HISTORIC_INTERPOLATION) * $historic_sparse_weights{$_} -> $sparse_weights{$_}\n";
    }
  }
  if ($___HISTORIC_INTERPOLATION > 0) {
    open my $weights_fh, '>', "run$run.$weights_out_file" or die "run$run.$weights_out_file: $!";
    print $weights_fh join(" ", @newweights);
    close $weights_fh;
  }

  $featlist->{"values"} = \@newweights;

  if (scalar keys %sparse_weights) {
    $sparse_weights_file = "run" . ($run + 1) . ".sparse-weights";
    open my $sparse_fh, '>', $sparse_weights_file or die "$sparse_weights_file: $!";
    foreach my $feature (keys %sparse_weights) {
      my $sparse_weight = $sparse_weights{$feature};
      if ($___PRO_STARTING_POINT) {
        $sparse_weight *= $merge_weight;
      }
      print $sparse_fh "$feature $sparse_weight\n";
    }
    close $sparse_fh;
  }

  ## additional stopping criterion: weights have not changed
  my $shouldstop = 1;
  for (my $i = 0; $i < @CURR; $i++) {
    die "Lost weight! mert reported fewer weights (@newweights) than we gave it (@CURR)"
      if !defined $newweights[$i];
    if (abs($CURR[$i] - $newweights[$i]) >= $minimum_required_change_in_weights) {
      $shouldstop = 0;
      last;
    }
  }

  &save_finished_step($finished_step_file, $run);

  if ($shouldstop) {
    print STDERR "None of the weights changed more than $minimum_required_change_in_weights. Stopping.\n";
    last;
  }

  my $firstrun;
  if ($prev_aggregate_nbl_size == -1) {
    $firstrun = 1;
  } else {
    $firstrun = $run - $prev_aggregate_nbl_size + 1;
    $firstrun = ($firstrun > 0) ? $firstrun : 1;
  }

  print "loading data from $firstrun to $run (prev_aggregate_nbl_size=$prev_aggregate_nbl_size)\n";
  $prev_feature_file = undef;
  $prev_score_file   = undef;
  $prev_init_file    = undef;
  for (my $i = $firstrun; $i <= $run; $i++) {
    if (defined $prev_feature_file) {
      $prev_feature_file = "${prev_feature_file},run${i}.${base_feature_file}";
    } else {
      $prev_feature_file = "run${i}.${base_feature_file}";
    }

    if (defined $prev_score_file) {
      $prev_score_file = "${prev_score_file},run${i}.${base_score_file}";
    } else {
      $prev_score_file = "run${i}.${base_score_file}";
    }

    if (defined $prev_init_file) {
      $prev_init_file = "${prev_init_file},run${i}.${weights_in_file}";
    } else {
      $prev_init_file = "run${i}.${weights_in_file}";
    }
  }
  print "loading data from $prev_feature_file\n" if defined($prev_feature_file);
  print "loading data from $prev_score_file\n"   if defined($prev_score_file);
  print "loading data from $prev_init_file\n"    if defined($prev_init_file);
}

if (defined $allsorted) {
    safesystem ("\\rm -f $allsorted") or die;
}

safesystem("\\cp -f $weights_in_file run$run.$weights_in_file") or die;
safesystem("\\cp -f $mert_logfile run$run.$mert_logfile") or die;

if($___RETURN_BEST_DEV) {
  my $bestit=1;
  my $bestbleu=0;
  my $evalout = "eval.out";
  for (my $i = 1; $i < $run; $i++) {
    my $cmd = "$mert_eval_cmd --reference " . join(",", @references) . " -s BLEU --candidate run$i.out";
    $cmd .= " -l $__REMOVE_SEGMENTATION" if defined( $__PROMIX_TRAINING);
    safesystem("$cmd 2> /dev/null 1> $evalout");
    open my $fh, '<', $evalout or die "Can't read $evalout : $!";
    my $bleu = <$fh>;
    chomp $bleu;
    if($bleu > $bestbleu) {
      $bestbleu = $bleu;
      $bestit = $i;
    }
    close $fh;
  }
  print "copying weights from best iteration ($bestit, bleu=$bestbleu) to moses.ini\n";
  my $best_sparse_file = undef;
  if(defined $sparse_weights_file) {
      $best_sparse_file = "run$bestit.sparse-weights";
  }
  create_config($___CONFIG_ORIG, "./moses.ini", get_featlist_from_file("run$bestit.dense"),
                $bestit, $bestbleu, $best_sparse_file);
}
else {
  create_config($___CONFIG_ORIG, "./moses.ini", $featlist, $run, $devbleu, $sparse_weights_file);
}

# just to be sure that we have the really last finished step marked
&save_finished_step($finished_step_file, $run);

#chdir back to the original directory # useless, just to remind we were not there
chdir($cwd);
print "Training finished at " . `date`;
} # end of local scope

sub get_weights_from_mert {
  my ($outfile, $logfile, $weight_count, $sparse_weights, $mix_weights) = @_;
  my ($bestpoint, $devbleu);
  if ($___PAIRWISE_RANKED_OPTIMIZER || ($___PRO_STARTING_POINT && $logfile =~ /pro/)
          || $___BATCH_MIRA || $__PROMIX_TRAINING) {
    open my $fh, '<', $outfile or die "Can't open $outfile: $!";
    my @WEIGHT;
    @$mix_weights = ();
    for (my $i = 0; $i < $weight_count; $i++) { push @WEIGHT, 0; }
    my $sum = 0.0;
    while (<$fh>) {
      if (/^F(\d+) ([\-\.\de]+)/) {     # regular features
        $WEIGHT[$1] = $2;
        $sum += abs($2);
      } elsif (/^M(\d+_\d+) ([\-\.\de]+)/) {     # mix weights
        push @$mix_weights,$2;
      } elsif (/^(.+_.+) ([\-\.\de]+)/) { # sparse features
        $$sparse_weights{$1} = $2;
      }
    }
    close $fh;
    die "It seems feature values are invalid or unable to read $outfile." if $sum < 1e-09;
  
    $devbleu = "unknown";
    foreach (@WEIGHT) { $_ /= $sum; }
    foreach (keys %{$sparse_weights}) { $$sparse_weights{$_} /= $sum; }
    $bestpoint = join(" ", @WEIGHT);

    if($___BATCH_MIRA) {
      open my $fh2, '<', $logfile or die "Can't open $logfile: $!";
      while(<$fh2>) {
        if(/Best BLEU = ([\-\d\.]+)/) {
          $devbleu = $1;
        }
      }
      close $fh2;
    }
  } else {
    open my $fh, '<', $logfile or die "Can't open $logfile: $!";
    while (<$fh>) {
      if (/Best point:\s*([\s\d\.\-e]+?)\s*=> ([\-\d\.]+)/) {
        $bestpoint = $1;
        $devbleu = $2;
        last;
      }
    }
    close $fh;
  }
  return ($bestpoint, $devbleu);
}

sub run_decoder {
    my ($featlist, $run, $need_to_normalize) = @_;
    my $filename_template = "run%d.best$___N_BEST_LIST_SIZE.out";
    my $filename = sprintf($filename_template, $run);
    my $lsamp_filename = undef;
    if ($___LATTICE_SAMPLES) {
      my $lsamp_filename_template = "run%d.lsamp$___LATTICE_SAMPLES.out";
      $lsamp_filename = sprintf($lsamp_filename_template, $run);
    }

    # user-supplied parameters
    print "params = $___DECODER_FLAGS\n";

    # parameters to set all model weights (to override moses.ini)
    my @vals = @{$featlist->{"values"}};
    if ($need_to_normalize) {
      print STDERR "Normalizing lambdas: @vals\n";
      my $totlambda = 0;
      grep($totlambda += abs($_), @vals);
      grep($_ /= $totlambda, @vals);
    }
    # moses now does not seem accept "-tm X -tm Y" but needs "-tm X Y"
    my %model_weights;
    for(my $i=0; $i<scalar(@{$featlist->{"names"}}); $i++) {
      my $name = $featlist->{"names"}->[$i];
      $model_weights{$name} = "$name=" if !defined $model_weights{$name};
      $model_weights{$name} .= sprintf " %.6f", $vals[$i];
    }
    my $decoder_config = "";
    $decoder_config = "-weight-overwrite '" . join(" ", values %model_weights) ."'" unless $___USE_CONFIG_WEIGHTS_FIRST && $run==1;
    $decoder_config .= " -weight-file run$run.sparse-weights" if -e "run$run.sparse-weights";
    $decoder_config .= " -report-segmentation" if $__PROMIX_TRAINING;
    print STDERR "DECODER_CFG = $decoder_config\n";
    print "decoder_config = $decoder_config\n";

    # run the decoder
    my $decoder_cmd;
    my $lsamp_cmd = "";
    if ($___LATTICE_SAMPLES) {
      $lsamp_cmd = " -lattice-samples $lsamp_filename $___LATTICE_SAMPLES ";
    }

    if (defined $___JOBS && $___JOBS > 0) {
      $decoder_cmd = "$moses_parallel_cmd $pass_old_sge -config $___CONFIG -inputtype $___INPUTTYPE -qsub-prefix mert$run -queue-parameters \"$queue_flags\" -decoder-parameters \"$___DECODER_FLAGS $decoder_config\" $lsamp_cmd -n-best-list \"$filename $___N_BEST_LIST_SIZE\" -input-file $___DEV_F -jobs $___JOBS -decoder $___DECODER > run$run.out";
    } else {
      $decoder_cmd = "$___DECODER $___DECODER_FLAGS  -config $___CONFIG -inputtype $___INPUTTYPE $decoder_config $lsamp_cmd -n-best-list $filename $___N_BEST_LIST_SIZE -input-file $___DEV_F > run$run.out";
    }

    safesystem($decoder_cmd) or die "The decoder died. CONFIG WAS $decoder_config \n";

    sanity_check_order_of_lambdas($featlist, $filename);
    return ($filename, $lsamp_filename);
}


sub insert_ranges_to_featlist {
  my $featlist = shift;
  my $ranges = shift;

  $ranges = [] if !defined $ranges;

  # first collect the ranges from options
  my $niceranges;
  foreach my $range (@$ranges) {
    my $name = undef;
    foreach my $namedpair (split /,/, $range) {
      if ($namedpair =~ /^(.*?):/) {
        $name = $1;
        $namedpair =~ s/^.*?://;
      }
      my ($min, $max) = split /\.\./, $namedpair;
      die "Bad min '$min' in --range=$range" if $min !~ /^-?[0-9.]+$/;
      die "Bad max '$max' in --range=$range" if $min !~ /^-?[0-9.]+$/;
      die "No name given in --range=$range" if !defined $name;
      push @{$niceranges->{$name}}, [$min, $max];
    }
  }

  # now populate featlist
  my $seen = undef;
  for(my $i = 0; $i < scalar(@{$featlist->{"names"}}); $i++) {
    my $name = $featlist->{"names"}->[$i];
    $seen->{$name} ++;
    my $min = 0.0;
    my $max = 1.0;
    if (defined $niceranges->{$name}) {
      my $minmax = shift @{$niceranges->{$name}};
      ($min, $max) = @$minmax if defined $minmax;
    }
    $featlist->{"mins"}->[$i] = $min;
    $featlist->{"maxs"}->[$i] = $max;
  }
  return $featlist;
}

sub sanity_check_order_of_lambdas {
  my $featlist = shift;
  my $filename_or_stream = shift;

  my @expected_lambdas = @{$featlist->{"names"}};
  my @got = get_order_of_scores_from_nbestlist($filename_or_stream);
  die "Mismatched lambdas. Decoder returned @got, we expected @expected_lambdas"
    if "@got" ne "@expected_lambdas";
}

sub get_featlist_from_moses {
  # run moses with the given config file and return the list of features and
  # their initial values
  my $configfn = shift;
  my $featlistfn = "./features.list";
  if (-e $featlistfn && ! -z $featlistfn) {   # exists & not empty
    print STDERR "Using cached features list: $featlistfn\n";
  } else {
    print STDERR "Asking moses for feature names and values from $___CONFIG\n";
    my $cmd = "$___DECODER $___DECODER_FLAGS -config $configfn  -inputtype $___INPUTTYPE -show-weights > $featlistfn";
    safesystem($cmd) or die "Failed to run moses with the config $configfn";
  }
  return get_featlist_from_file($featlistfn);
}

sub get_featlist_from_file {
  my $featlistfn = shift;
  # read feature list
  my @names = ();
  my @startvalues = ();
  open my $fh, '<', $featlistfn or die "Can't read $featlistfn : $!";
  my $nr = 0;
  my @errs = ();
  while (<$fh>) {
    $nr++;
    chomp;
    if (/^(\S+)= (.+)$/) { # only for feature functions with dense features
      my ($longname, $valuesStr) = ($1, $2);
      next if (!defined($valuesStr));
    
      my @values = split(/ /, $valuesStr);
		  foreach my $value (@values) {
			  push @errs, "$featlistfn:$nr:Bad initial value of $longname: $value\n"
				  if $value !~ /^[+-]?[0-9.\-e]+$/;
			  push @names, $longname;
			  push @startvalues, $value;
      }
    }
  }
  close $fh;

  if (scalar @errs) {
    warn join("", @errs);
    exit 1;
  }
  return {"names"=>\@names, "values"=>\@startvalues};
}


sub get_order_of_scores_from_nbestlist {
  # read the first line and interpret the ||| label: num num num label2: num ||| column in nbestlist
  # return the score labels in order
  my $fname_or_source = shift;
  # print STDERR "Peeking at the beginning of nbestlist to get order of scores: $fname_or_source\n";
  open my $fh, $fname_or_source or die "Failed to get order of scores from nbestlist '$fname_or_source': $!";
  my $line = <$fh>;
  close $fh;
  die "Line empty in nbestlist '$fname_or_source'" if !defined $line;
  my ($sent, $hypo, $scores, $total) = split /\|\|\|/, $line;
  $scores =~ s/^\s*|\s*$//g;
  die "No scores in line: $line" if $scores eq "";

  my @order = ();
  my $label = undef;
  my $sparse = 0; # we ignore sparse features here
  foreach my $tok (split /\s+/, $scores) {
    if ($tok =~ /.+_.+=/) {
      $sparse = 1;
    } elsif ($tok =~ /^([a-z][0-9a-z]*)=/i) {
      $label = $1;
    } elsif ($tok =~ /^-?[-0-9.\-e]+$/) {
      if (!$sparse) {
        # a score found, remember it
        die "Found a score but no label before it! Bad nbestlist '$fname_or_source'!"
          if !defined $label;
        push @order, $label;
      }
      $sparse = 0;
    } else {
      die "Not a label, not a score '$tok'. Failed to parse the scores string: '$scores' of nbestlist '$fname_or_source'";
    }
  }
  print STDERR "The decoder returns the scores in this order: @order\n";
  return @order;
}

sub create_config {
  # TODO: too many arguments. you might want to consider using hashes
  my $infn                = shift; # source config
  my $outfn               = shift; # where to save the config
  my $featlist            = shift; # the lambdas we should write
  my $iteration           = shift;  # just for verbosity
  my $bleu_achieved       = shift; # just for verbosity
  my $sparse_weights_file = shift; # only defined when optimizing sparse features

  for (my $i = 0; $i < scalar(@{$featlist->{"names"}}); $i++) {
    my $name = $featlist->{"names"}->[$i];
    my $val = $featlist->{"values"}->[$i];
    # ensure long name
		print STDERR "featlist: $name=$val \n";
  }

  my %P; # the hash of all parameters we wish to override

  # first convert the command line parameters to the hash
  # ensure local scope of vars
  {
    my $parameter = undef;
    print "Parsing --decoder-flags: |$___DECODER_FLAGS|\n";
    $___DECODER_FLAGS =~ s/^\s*|\s*$//;
    $___DECODER_FLAGS =~ s/\s+/ /;
    foreach (split(/ /, $___DECODER_FLAGS)) {
      if (/^\-([^\d].*)$/) {
        $parameter = $1;
      } else {
				my $value = $_;
        die "Found value with no -paramname before it: $value"
            if !defined $parameter;
        push @{$P{$parameter}}, $value;
      }
    }
  }

  if (defined($sparse_weights_file)) {
    push @{$P{"weight-file"}}, File::Spec->catfile($___WORKING_DIR, $sparse_weights_file);
  }

  # create new moses.ini decoder config file by cloning and overriding the original one
  open my $ini_fh, '<', $infn or die "Can't read $infn: $!";
  delete($P{"config"}); # never output
  print "Saving new config to: $outfn\n";

  open my $out, '>', $outfn or die "Can't write $outfn: $!";
  print $out "# MERT optimized configuration\n";
  print $out "# decoder $___DECODER\n";
  print $out "# BLEU $bleu_achieved on dev $___DEV_F\n";
  print $out "# We were before running iteration $iteration\n";
  print $out "# finished ".`date`;

  my $line = <$ini_fh>;
  while(1) {
    last unless $line;

    # skip until hit [parameter]
    if ($line !~ /^\[(.+)\]\s*$/) {
      $line = <$ini_fh>;
      print $out $line if $line =~ /^\#/ || $line =~ /^\s+$/;
      next;
    }

    # parameter name
    my $parameter = $1;

		if ($parameter eq "weight") {
			# leave weights 'til last. We're changing it
			while ($line = <$ini_fh>) {
			  last if $line =~ /^\[/;
			}
		}
	  elsif (defined($P{$parameter})) {
			# found a param (thread, verbose etc) that we're overriding. Leave to the end
			while ($line = <$ini_fh>) {
			  last if $line =~ /^\[/;
			}
	  }
		else {
			# unchanged parameter, write old
			print $out "[$parameter]\n";
			while ($line = <$ini_fh>) {
				last if $line =~ /^\[/;
				print $out $line;
			}
		}
	}

  # write all additional parameters
  foreach my $parameter (keys %P) {
    print $out "\n[$parameter]\n";
    foreach (@{$P{$parameter}}) {
      print $out $_."\n";
    }
  }

  # write all weights
  print $out "[weight]\n";
  
  my $prevName = "";
  my $outStr = "";
  for (my $i = 0; $i < scalar(@{$featlist->{"names"}}); $i++) {
    my $name = $featlist->{"names"}->[$i];
    my $val = $featlist->{"values"}->[$i];
    
    if ($prevName eq $name) {
      $outStr .= " $val";
    }
    else {
  		print $out "$outStr\n";
  		$outStr = "$name= $val";
  		$prevName = $name;
    }
  }
	print $out "$outStr\n";

  close $ini_fh;
  close $out;
  print STDERR "Saved: $outfn\n";
}

# Create a new ini file, with the first ttable replaced by the given one
# and its type set to text
sub substitute_ttable {
  my ($old_ini, $new_ini, $new_ttable, $ttable_type) = @_;
  $ttable_type = "0" unless defined($ttable_type);
  open(NEW_INI,">$new_ini") || die "Failed to create $new_ini";
  open(INI,$old_ini) || die "Failed to open $old_ini";
  while(<INI>) {
    if (/\[ttable-file\]/) {
      print NEW_INI "[ttable-file]\n";
      my $ttable_config = <INI>;
      chomp $ttable_config;
      my @ttable_fields = split /\s+/, $ttable_config;
      $ttable_fields[0] = $ttable_type;
      $ttable_fields[4] = $new_ttable;
      print NEW_INI join(" ", @ttable_fields) . "\n";
    } else {
      print NEW_INI;
    }
  }
  close NEW_INI;
  close INI;
}


sub safesystem {
  print STDERR "Executing: @_\n";
  system(@_);
  if ($? == -1) {
      warn "Failed to execute: @_\n  $!";
      exit(1);
  } elsif ($? & 127) {
      printf STDERR "Execution of: @_\n  died with signal %d, %s coredump\n",
          ($? & 127),  ($? & 128) ? 'with' : 'without';
      exit(1);
  } else {
    my $exitcode = $? >> 8;
    warn "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}

sub ensure_full_path {
  my $PATH = shift;
  $PATH =~ s/\/nfsmnt//;
  return $PATH if $PATH =~ /^\//;

  my $dir = Cwd::getcwd();
  $PATH = File::Spec->catfile($dir, $PATH);
  $PATH =~ s/[\r\n]//g;
  $PATH =~ s/\/\.\//\//g;
  $PATH =~ s/\/+/\//g;
  my $sanity = 0;
  while($PATH =~ /\/\.\.\// && $sanity++ < 10) {
    $PATH =~ s/\/+/\//g;
    $PATH =~ s/\/[^\/]+\/\.\.\//\//g;
  }
  $PATH =~ s/\/[^\/]+\/\.\.$//;
  $PATH =~ s/\/+$//;
  $PATH =~ s/\/nfsmnt//;
  return $PATH;
}

sub submit_or_exec {
  my ($cmd, $stdout, $stderr) = @_;
  print STDERR "exec: $cmd\n";
  if (defined $___JOBS && $___JOBS > 0) {
    safesystem("$qsubwrapper $pass_old_sge -command='$cmd' -queue-parameter=\"$queue_flags\" -stdout=$stdout -stderr=$stderr" )
      or die "ERROR: Failed to submit '$cmd' (via $qsubwrapper)";
  } else {
    safesystem("$cmd > $stdout 2> $stderr") or die "ERROR: Failed to run '$cmd'.";
  }
}

sub create_extractor_script() {
  my ($cmd, $outdir) = @_;
  my $script_path = File::Spec->catfile($outdir, "extractor.sh");

  open my $out, '>', $script_path
      or die "Couldn't open $script_path for writing: $!\n";
  print $out "#!/bin/bash\n";
  print $out "cd $outdir\n";
  print $out "$cmd\n";
  close $out;

  `chmod +x $script_path`;

  return $script_path;
}

sub save_finished_step {
  my ($filename, $step) = @_;
  open my $fh, '>', $filename or die "$filename: $!";
  print $fh $step . "\n";
  close $fh;
}

# It returns a config for mert/extractor.
sub setup_reference_length_type {
  if (($___CLOSEST + $___AVERAGE + $___SHORTEST) > 1) {
    die "You can specify just ONE reference length strategy (closest or shortest or average) not both\n";
  }

  if ($___SHORTEST) {
    return " reflen:shortest";
  } elsif ($___AVERAGE) {
    return " reflen:average";
  } elsif ($___CLOSEST) {
    return " reflen:closest";
  } else {
    return "";
  }
}

sub setup_case_config {
  if ($___NOCASE) {
    return " case:false";
  } else {
    return " case:true";
  }
}

sub is_mac_osx {
  return ($^O eq "darwin") ? 1 : 0;
}
