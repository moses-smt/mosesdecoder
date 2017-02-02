#!/usr/bin/perl -w 

# Usage:
# zmert-moses.pl <foreign> <english> <decoder-executable> <decoder-config>
# For other options see below or run 'zmert-moses.pl --help'

# Notes:
# <foreign> and <english> should be raw text files, one sentence per line
# <english> can be a prefix, in which case the files are <english>0, <english>1, etc. are used

# Revision history

# 29 Dec 2009 Derived from mert-moses-new.pl (Kamil Kos)

use FindBin qw($RealBin);
use File::Basename;
my $SCRIPTS_ROOTDIR = $RealBin;
$SCRIPTS_ROOTDIR =~ s/\/training$//;
$SCRIPTS_ROOTDIR = $ENV{"SCRIPTS_ROOTDIR"} if defined($ENV{"SCRIPTS_ROOTDIR"});

# for each _d_istortion, _l_anguage _m_odel, _t_ranslation _m_odel and _w_ord penalty, there is a list
# of [ default value, lower bound, upper bound ]-triples. In most cases, only one triple is used,
# but the translation model has currently 5 features

# defaults for initial values and ranges are:

my $default_triples = {
    # these two basic models exist even if not specified, they are
    # not associated with any model file
    "w" => [ [ 0.0, -1.0, 1.0 ] ],  # word penalty
};

my $additional_triples = {
    # if the more lambda parameters for the weights are needed
    # (due to additional tables) use the following values for them
    "d"  => [ [ 1.0, 0.0, 2.0 ],    # lexicalized reordering model
	      [ 1.0, 0.0, 2.0 ],
	      [ 1.0, 0.0, 2.0 ],
	      [ 1.0, 0.0, 2.0 ],
	      [ 1.0, 0.0, 2.0 ],
	      [ 1.0, 0.0, 2.0 ],
	      [ 1.0, 0.0, 2.0 ] ],
    "lm" => [ [ 1.0, 0.0, 2.0 ] ],  # language model
    "g"  => [ [ 1.0, 0.0, 2.0 ],    # generation model
	      [ 1.0, 0.0, 2.0 ] ],
    "tm" => [ [ 0.3, 0.0, 0.5 ],    # translation model
	      [ 0.2, 0.0, 0.5 ],
	      [ 0.3, 0.0, 0.5 ],
	      [ 0.2, 0.0, 0.5 ],
	      [ 0.0,-1.0, 1.0 ] ],  # ... last weight is phrase penalty
    "lex"=> [ [ 0.1, 0.0, 0.2 ] ],  # global lexical model
};

# moses.ini file uses FULL names for lambdas, while this training script internally (and on the command line)
# uses ABBR names.
my $ABBR_FULL_MAP = "d=weight-d lm=weight-l tm=weight-t w=weight-w g=weight-generation lex=weight-lex";
my %ABBR2FULL = map {split/=/,$_,2} split /\s+/, $ABBR_FULL_MAP;
my %FULL2ABBR = map {my ($a, $b) = split/=/,$_,2; ($b, $a);} split /\s+/, $ABBR_FULL_MAP;

# We parse moses.ini to figure out how many weights do we need to optimize.
# For this, we must know the correspondence between options defining files
# for models and options assigning weights to these models.
my $TABLECONFIG_ABBR_MAP = "ttable-file=tm lmodel-file=lm distortion-file=d generation-file=g global-lexical-file=lex";
my %TABLECONFIG2ABBR = map {split(/=/,$_,2)} split /\s+/, $TABLECONFIG_ABBR_MAP;

# There are weights that do not correspond to any input file, they just increase the total number of lambdas we optimize
#my $extra_lambdas_for_model = {
#  "w" => 1,  # word penalty
#  "d" => 1,  # basic distortion
#};

my $verbose = 0;
my $___MERT_VERBOSE = 1; # verbosity of zmert (values: 0-2)
my $___DECODER_VERBOSE = 1; # should decoder output be included? - 0:no,1:yes
my $___SAVE_INTER = 2; # save intermediate nbest-lists
my $usage = 0; # request for --help
my $___WORKING_DIR = "mert-work";
my $___DEV_F = undef; # required, input text to decode
my $___DEV_E = undef; # required, basename of files with references
my $___DECODER = undef; # required, pathname to the decoder executable
my $___CONFIG = undef; # required, pathname to startup ini file
my $___N_BEST_LIST_SIZE = 100;
my $___MAX_MERT_ITER = 0; # do not limit the number of iterations
my $queue_flags = "-l mem_free=0.5G -hard";  # extra parameters for parallelizer
      # the -l ws0ssmt is relevant only to JHU workshop
my $___JOBS = undef; # if parallel, number of jobs to use (undef -> serial)
my $___DECODER_FLAGS = ""; # additional parametrs to pass to the decoder
my $___LAMBDA = undef; # string specifying the seed weights and boundaries of all lambdas
my $skip_decoder = 0; # and should we skip the first decoder run (assuming we got interrupted during mert)
my $___FILTER_PHRASE_TABLE = 1; # filter phrase table
my $___PREDICTABLE_SEEDS = 0;
my $___METRIC = "BLEU 4 shortest"; # name of metric that will be used for minimum error training, followed by metric parameters (see zmert documentation)
my $___SEMPOSBLEU_WEIGHTS = "1 1"; # weights of SemPOS and BLEU
my $___LAMBDAS_OUT = undef; # file where final lambdas should be written
my $___EXTRACT_SEMPOS = "none"; # how shall we get the SemPOS factor (only for SemPOS metric)
      # options: 1) 'none' - moses generates SemPOS factor in required format 
      #             (<word_form>|<SemPOS>)
      #          2) 'factors:<factor_index_list>' - extract factors from decoder output on positions from <factor_index_list>
      #              <factor_index_list> contains indices of factors separated by comma, e.g. '0,1,4'
      #          3) 'tmt' - moses outputs only <word_form> and we need to 
      #             generate factors like SemPOS with TectoMT (see http://ufal.mff.cuni.cz/tectomt/)

# set 1 if using with async decoder
my $___ASYNC = 0; 

# Use "--norm" to select normalization in mert
my $___NORM = "none";

# set 0 if input type is text, set 1 if input type is confusion network
my $___INPUTTYPE = 0; 

my $mertdir = "$SCRIPTS_ROOTDIR/../zmert/";  # path to zmert directory
my $filtercmd = undef; # path to filter-model-given-input.pl
my $clonecmd = "$SCRIPTS_ROOTDIR/training/clone_moses_model.pl"; # executable clone_moses_model.pl
my $qsubwrapper = undef;
my $moses_parallel_cmd = undef;
my $old_sge = 0; # assume sge<6.0
my $___ACTIVATE_FEATURES = undef; # comma-separated (or blank-separated) list of features to work on 
                                  # if undef work on all features
                                  # (others are fixed to the starting values)
my %active_features; # hash with features to optimize; optimize all if empty

use strict;
use Getopt::Long;
GetOptions(
  "working-dir=s" => \$___WORKING_DIR,
  "input=s" => \$___DEV_F,
  "inputtype=i" => \$___INPUTTYPE,
  "refs=s" => \$___DEV_E,
  "decoder=s" => \$___DECODER,
  "config=s" => \$___CONFIG,
  "nbest:i" => \$___N_BEST_LIST_SIZE,
  "maxiter:i" => \$___MAX_MERT_ITER,
  "queue-flags:s" => \$queue_flags,
  "jobs=i" => \$___JOBS,
  "decoder-flags=s" => \$___DECODER_FLAGS,
  "lambdas=s" => \$___LAMBDA,
  "metric=s" => \$___METRIC,
  "semposbleu-weights:s" => \$___SEMPOSBLEU_WEIGHTS,
  "extract-sempos=s" => \$___EXTRACT_SEMPOS,
  "norm:s" => \$___NORM,
  "help" => \$usage,
  "verbose" => \$verbose,
  "mert-verbose:i" => \$___MERT_VERBOSE,
  "decoder-verbose:i" => \$___DECODER_VERBOSE,
  "mertdir:s" => \$mertdir, # allow to override the default location of zmert.jar
  "lambdas-out:s" => \$___LAMBDAS_OUT,
  "rootdir=s" => \$SCRIPTS_ROOTDIR,
  "filtercmd=s" => \$filtercmd, # allow to override the default location
  "qsubwrapper=s" => \$qsubwrapper, # allow to override the default location
  "mosesparallelcmd=s" => \$moses_parallel_cmd, # allow to override the default location
  "old-sge" => \$old_sge, #passed to moses-parallel
  "filter-phrase-table!" => \$___FILTER_PHRASE_TABLE, # allow (disallow)filtering of phrase tables
  "predictable-seeds:s" => \$___PREDICTABLE_SEEDS, # allow (disallow) switch on/off reseeding of random restarts
  "async=i" => \$___ASYNC, #whether script to be used with async decoder
  "activate-features=s" => \$___ACTIVATE_FEATURES #comma-separated (or blank-separated) list of features to work on (others are fixed to the starting values)
) or exit(1);

print "Predict $___PREDICTABLE_SEEDS\n";

# the 4 required parameters can be supplied on the command line directly
# or using the --options
if (scalar @ARGV == 4) {
  # required parameters: input_file references_basename decoder_executable
  $___DEV_F = shift;
  $___DEV_E = shift;
  $___DECODER = shift;
  $___CONFIG = shift;
}

if ($___ASYNC) {
	delete $default_triples->{"w"};
	$additional_triples->{"w"} = [ [ 0.0, -1.0, 1.0 ] ];
}

print STDERR "After default: $queue_flags\n";

if ($usage || !defined $___DEV_F || !defined$___DEV_E || !defined$___DECODER || !defined $___CONFIG) {
  print STDERR "usage: zmert-moses.pl input-text references decoder-executable decoder.ini
Options:
  --working-dir=mert-dir ... where all the files are created
  --nbest=100 ... how big nbestlist to generate
  --maxiter=N ... maximum number of zmert iterations
  --jobs=N  ... set this to anything to run moses in parallel
  --mosesparallelcmd=STRING ... use a different script instead of moses-parallel
  --queue-flags=STRING  ... anything you with to pass to 
              qsub, eg. '-l ws06osssmt=true'
              The default is 
								-l mem_free=0.5G -hard
              To reset the parameters, please use \"--queue-flags=' '\" (i.e. a space between
              the quotes).
  --decoder-flags=STRING ... extra parameters for the decoder
  --lambdas=STRING  ... default values and ranges for lambdas, a complex string
         such as 'd:1,0.5-1.5 lm:1,0.5-1.5 tm:0.3,0.25-0.75;0.2,0.25-0.75;0.2,0.25-0.75;0.3,0.25-0.75;0,-0.5-0.5 w:0,-0.5-0.5'
  --allow-unknown-lambdas ... keep going even if someone supplies a new lambda
         in the lambdas option (such as 'superbmodel:1,0-1'); optimize it, too
  --lambdas-out=STRING ... file where final lambdas should be written
  --metric=STRING ... metric name for optimization with metric parameters
         such as 'BLEU 4 closest' or 'SemPOS 0 1'. Use default parameters by specifying 'BLEU' or 'SemPOS'
  --semposbleu-weights=STRING ... weights for SemPOS and BLEU in format 'N:M' where 'N' is SemPOS weight and 'M' BLEU weight
         used only with SemPOS_BLEU metric
  --extract-sempos=STRING ... none|factors:<factor_list>|tmt
         'none' ... decoder generates all required factors for optimization metric
         'factors:<factor_list>' ... extract factors with index in <factor_list> from decoder output
                 e.g. 'factors:0,2,3' to extract first, third and fourth factor from decoder output
         'tmt' ... use TectoMT (see http://ufal.mff.cuni.cz/tectomt) to generate required factors
  --norm ... Select normalization for zmert
  --mert-verbose=N ... verbosity of zmert [0|1|2]
  --decoder-verbose=N ... decoder verbosity [0|1] - 1=decoder output included
  --mertdir=STRING ... directory with zmert.jar
  --filtercmd=STRING  ... path to filter-model-given-input.pl
  --rootdir=STRING  ... where do helpers reside (if not given explicitly)
  --mertdir=STRING ... path to zmert implementation
  --scorenbestcmd=STRING  ... path to score-nbest.py
  --old-sge ... passed to moses-parallel, assume Sun Grid Engine < 6.0
  --inputtype=[0|1|2] ... Handle different input types (0 for text, 1 for confusion network, 2 for lattices, default is 0)
  --no-filter-phrase-table ... disallow filtering of phrase tables
                              (useful if binary phrase tables are available)
  --predictable-seeds ... provide predictable seeds to mert so that random restarts are the same on every run
  --activate-features=STRING  ... comma-separated list of features to work on
                                  (if undef work on all features)
                                  # (others are fixed to the starting values)
  --verbose ... verbosity of this script
  --help ... print this help

";
  exit 1;
}

# ensure we know where is tectomt, if we need it
if( !defined $ENV{"TMT_ROOT"} && $___EXTRACT_SEMPOS =~ /tmt/) {
  die "Cannot find TMT_ROOT. Is TectoMT really initialized?";
}
my $TMT_ROOT = $ENV{"TMT_ROOT"};

my $srunblocks = "$TMT_ROOT/tools/srunblocks_streaming/srunblocks";
my $scenario_file = "scenario"; 
my $qruncmd = "/home/bojar/diplomka/bin/qruncmd";
my $srunblocks_cmd = "$srunblocks --errorlevel=FATAL $scenario_file czech_source_sentence factored_output";
if (defined $___JOBS && $___JOBS > 1) {
  die "Can't run $qruncmd" if ! -x $qruncmd;
  $srunblocks_cmd = "$qruncmd --jobs=$___JOBS --join '$srunblocks_cmd'";
}


# update variables if input is confusion network
if ($___INPUTTYPE == 1)
{
  $ABBR_FULL_MAP = "$ABBR_FULL_MAP I=weight-i";
  %ABBR2FULL = map {split/=/,$_,2} split /\s+/, $ABBR_FULL_MAP;
  %FULL2ABBR = map {my ($a, $b) = split/=/,$_,2; ($b, $a);} split /\s+/, $ABBR_FULL_MAP;

  push @{$default_triples -> {"I"}}, [ 1.0, 0.0, 2.0 ];
  #$extra_lambdas_for_model -> {"I"} = 1; #Confusion network posterior
}

# update variables if input is lattice
if ($___INPUTTYPE == 2)
{
# TODO
}

if (defined $___ACTIVATE_FEATURES)
{
  %active_features = map {$_ => 1} split( /,/, $___ACTIVATE_FEATURES);
}

# Check validity of input parameters and set defaults if needed

print STDERR "Using SCRIPTS_ROOTDIR: $SCRIPTS_ROOTDIR\n";

# path of script for filtering phrase tables and running the decoder
$filtercmd="$SCRIPTS_ROOTDIR/training/filter-model-given-input.pl" if !defined $filtercmd;

$qsubwrapper="$SCRIPTS_ROOTDIR/generic/qsub-wrapper.pl" if !defined $qsubwrapper;

$moses_parallel_cmd = "$SCRIPTS_ROOTDIR/generic/moses-parallel.pl"
  if !defined $moses_parallel_cmd;



die "Error: need to specify the zmert.jar directory" if !defined $mertdir;

my $zmert_classpath = ensure_full_path("$mertdir/zmert.jar");
die "File not found: $mertdir/zmert.jar (interpreted as $zmert_classpath)"
  if ! -e $zmert_classpath;

my ($just_cmd_filtercmd,$x) = split(/ /,$filtercmd);
die "Not executable: $just_cmd_filtercmd" if ! -x $just_cmd_filtercmd;
die "Not executable: $moses_parallel_cmd" if defined $___JOBS && ! -x $moses_parallel_cmd;
die "Not executable: $qsubwrapper" if defined $___JOBS && ! -x $qsubwrapper;
die "Not executable: $___DECODER" if ! -x $___DECODER;

my $input_abs = ensure_full_path($___DEV_F);
die "File not found: $___DEV_F (interpreted as $input_abs)."
  if ! -e $input_abs;
$___DEV_F = $input_abs;


# Option to pass to qsubwrapper and moses-parallel
my $pass_old_sge = $old_sge ? "-old-sge" : "";

my $decoder_abs = ensure_full_path($___DECODER);
die "File not found: $___DECODER (interpreted as $decoder_abs)."
  if ! -x $decoder_abs;
$___DECODER = $decoder_abs;


my $ref_abs = ensure_full_path($___DEV_E);
# check if English dev set (reference translations) exist and store a list of all references
my @references;
my @references_factored;
if (-e $ref_abs) {
  push @references, $ref_abs;
}
else {
  # if multiple file, get a full list of the files
    my $part = 0;
    while (-e $ref_abs.$part) {
        push @references, $ref_abs.$part;
        $part++;
    }
    die("Reference translations not found: $___DEV_E (interpreted as $ref_abs)") unless $part;
}

my $config_abs = ensure_full_path($___CONFIG);
die "File not found: $___CONFIG (interpreted as $config_abs)."
  if ! -e $config_abs;
$___CONFIG = $config_abs;



# check validity of moses.ini and collect number of models and lambdas per model
# need to make a copy of $extra_lambdas_for_model, scan_config spoils it
#my %copy_of_extra_lambdas_for_model = %$extra_lambdas_for_model;
my %used_triples = %{$default_triples};
my ($models_used) = scan_config($___CONFIG);

# Parse the lambda config string and convert it to a nice structure in the same format as $used_triples
if (defined $___LAMBDA) {
  my %specified_triples;
  # interpreting lambdas from command line
  foreach (split(/\s+/,$___LAMBDA)) {
      my ($name,$values) = split(/:/);
      die "Malformed setting: '$_', expected name:values\n" if !defined $name || !defined $values;
      foreach my $startminmax (split/;/,$values) {
	  if ($startminmax =~ /^(-?[\.\d]+),(-?[\.\d]+)-(-?[\.\d]+)$/) {
	      my $start = $1;
	      my $min = $2;
	      my $max = $3;
              push @{$specified_triples{$name}}, [$start, $min, $max];
	  }
	  else {
	      die "Malformed feature range definition: $name => $startminmax\n";
	  }
      } 
  }
  # sanity checks for specified lambda triples
  foreach my $name (keys %used_triples) {
      die "No lambdas specified for '$name', but ".($#{$used_triples{$name}}+1)." needed.\n"
	  unless defined($specified_triples{$name});
      die "Number of lambdas specified for '$name' (".($#{$specified_triples{$name}}+1).") does not match number needed (".($#{$used_triples{$name}}+1).")\n"
	  if (($#{$used_triples{$name}}) != ($#{$specified_triples{$name}}));
  }
  foreach my $name (keys %specified_triples) {
      die "Lambdas specified for '$name' ".(@{$specified_triples{$name}}).", but none needed.\n"
	  unless defined($used_triples{$name});
  }
  %used_triples = %specified_triples;
}

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

#store current directory and create the working directory (if needed)
my $cwd = `pawd 2>/dev/null`; 
if(!$cwd){$cwd = `pwd`;}
chomp($cwd);

safesystem("mkdir -p $___WORKING_DIR") or die "Can't mkdir $___WORKING_DIR";

{
# open local scope

#chdir to the working directory
chdir($___WORKING_DIR) or die "Can't chdir to $___WORKING_DIR";

# fixed file names
my $mert_logfile = "zmert.log";

if ($___FILTER_PHRASE_TABLE){
  # filter the phrase tables wih respect to input, use --decoder-flags
  print "filtering the phrase tables... ".`date`;
  my $cmd = "$filtercmd ./filtered $___CONFIG $___DEV_F";
  if (defined $___JOBS) {
    safesystem("$qsubwrapper $pass_old_sge -command='$cmd' -queue-parameter=\"$queue_flags\" -stdout=filterphrases.out -stderr=filterphrases.err" )
      or die "Failed to submit filtering of tables to the queue (via $qsubwrapper)";
  } else {
    safesystem($cmd) or die "Failed to filter the tables.";
  }

  # the decoder should now use the filtered model
  $___CONFIG = "filtered/moses.ini";
}
else{
  # make a local clone of moses.ini
  safesystem("$clonecmd $___CONFIG");
  $___CONFIG = "moses.ini";
}

$___CONFIG = ensure_full_path($___CONFIG);

my $PARAMETERS;
$PARAMETERS = $___DECODER_FLAGS;

my $nbest_file = "zmert.best$___N_BEST_LIST_SIZE.out";

# Run zmert to optimize lambdas
# We need to prepare:
#	1) decoder launch script (decoder_cmd) - must be executable
#	2) zmert configuration file (zmert_cfg.txt)
#	3) parameters we want to optimize (params.txt)
#	4) decoder configuration file (decoder_cfg_inter.txt)


my $zmert_cfg = ensure_full_path("zmert_cfg.txt");
my $opt_params = "params.txt"; # zmert requires path relative to launch path
my $decoder_cfg_inter = "decoder_cfg_inter.txt"; # zmert requires path relative to launch path
my $decoder_cmd_file = ensure_full_path("decoder_cmd");
my $iteration_file = "iteration";

my $LAMBDAS_FILE = ensure_full_path("finalWeights.txt");

# prepare script that will launch moses from template
# it will include an update script that will adjust feature weights according to
# the last zmert iteration (they are stored in file $decoder_cfg_inter)

# prepare lauch command with all parameters
my $decoder_cmd;
if (defined $___JOBS) {
  $decoder_cmd = "$moses_parallel_cmd $pass_old_sge -config $___CONFIG -inputtype $___INPUTTYPE -qsub-prefix zmert -queue-parameters '$queue_flags' -decoder-parameters '$PARAMETERS' -n-best-list '$nbest_file $___N_BEST_LIST_SIZE' -input-file $___DEV_F -jobs $___JOBS -decoder $___DECODER > moses.out";
} else {
  $decoder_cmd = "$___DECODER $PARAMETERS -config $___CONFIG -inputtype $___INPUTTYPE -n-best-list $nbest_file $___N_BEST_LIST_SIZE -i $___DEV_F > moses.out";
}

my $zmert_decoder_cmd = "$SCRIPTS_ROOTDIR/training/zmert-decoder.pl";

# number of factors that a given metric requires
my $metric_num_factors = 1;	

# SemPOS metric requires 2 parameters specifying position of t_lemma and sempos factor
# e.g. for t_lemma|sempos|factor3|factor4|... the values are 0 and 1 (default setting)
if( $___METRIC =~ /^SemPOS$/) {
  $___METRIC .= " 0 1";
  $metric_num_factors = 2;
}
# SemPOS_BLEU metric requires 7 parameters
# 1) weight of SemPOS 2) weight of BLEU 
# 3) index of t_lemma for SemPOS 4) index of sempos for SemPOS
# 5) max ngram for BLEU 6) ref length strategy for BLEU
# 7) index of factor to compute BLEU on
elsif( $___METRIC =~ /^SemPOS_BLEU$/) {
  $___SEMPOSBLEU_WEIGHTS =~ /^.*:.*$/ or die "--semposbleu-weights is not in format <sempos_weight>:<bleu_weight>";
  $___SEMPOSBLEU_WEIGHTS =~ s/:/ /;
  $___METRIC .= " $___SEMPOSBLEU_WEIGHTS 1 2 4 closest 0";
  $metric_num_factors = 3;
}
elsif( $___METRIC =~ /^BLEU$/) {
  $___METRIC .= " 4 closest";
}
 elsif( $___METRIC =~ /^TER$/) {
  $___METRIC .= " nocase punc 20 50";
}
elsif( $___METRIC =~ /^TER-BLEU$/) {
  $___METRIC .= " nocase punc 20 50 4 closest";
}

if( $___EXTRACT_SEMPOS =~ /tmt/) {
  my $print_string = "";
  if( $___METRIC =~ /SemPOS_BLEU/) {
    $print_string = "Print::ForSemPOSBLEUMetric TMT_PARAM_PRINT_FOR_SEMPOS_BLEU_METRIC=m:form|t_lemma|gram/sempos TMT_PARAM_PRINT_FOR_SEMPOS_BLEU_METRIC_DESTINATION=factored_output";
  } elsif( $___METRIC =~ /SemPOS/) {
    $print_string = "Print::ForSemPOSMetric TMT_PARAM_PRINT_FOR_SEMPOS_METRIC=t_lemma|gram/sempos TMT_PARAM_PRINT_FOR_SEMPOS_METRIC_DESTINATION=factored_output";
  } else {
    die "Trying to get factors using tmt for unknown metric $___METRIC";
  }

  open( SCENARIO, ">$scenario_file") or die "Cannot open $scenario_file";
  print SCENARIO << "FILE_EOF";
SCzechW_to_SCzechM::Tokenize_joining_numbers
SCzechW_to_SCzechM::TagMorce
# SCzechM_to_SCzechN::Czech_named_ent_SVM_recognizer
# SCzechM_to_SCzechN::Geo_ne_recognizer
# SCzechM_to_SCzechN::Embed_instances
SCzechM_to_SCzechA::McD_parser_local TMT_PARAM_MCD_CZ_MODEL=pdt20_train_autTag_golden_latin2_pruned_0.02.model
# SCzechM_to_SCzechA::McD_parser_local TMT_PARAM_MCD_CZ_MODEL=pdt20_train_autTag_golden_latin2_pruned_0.10.model
SCzechM_to_SCzechA::Fix_atree_after_McD
SCzechM_to_SCzechA::Fix_is_member
SCzechA_to_SCzechT::Mark_auxiliary_nodes
SCzechA_to_SCzechT::Build_ttree
SCzechA_to_SCzechT::Fill_is_member
SCzechA_to_SCzechT::Rehang_unary_coord_conj
SCzechA_to_SCzechT::Assign_coap_functors
SCzechA_to_SCzechT::Fix_is_member
SCzechA_to_SCzechT::Distrib_coord_aux
SCzechA_to_SCzechT::Mark_clause_heads
SCzechA_to_SCzechT::Mark_relclause_heads
SCzechA_to_SCzechT::Mark_relclause_coref
SCzechA_to_SCzechT::Fix_tlemmas
SCzechA_to_SCzechT::Assign_nodetype
SCzechA_to_SCzechT::Assign_grammatemes
SCzechA_to_SCzechT::Detect_formeme
SCzechA_to_SCzechT::Add_PersPron
SCzechA_to_SCzechT::Mark_reflpron_coref
SCzechA_to_SCzechT::TBLa2t_phaseFd
$print_string
FILE_EOF
  close( SCENARIO);
}

my $feats_order = join( " ", keys %used_triples);

open( DECODER_CMD, ">$decoder_cmd_file") or die "Cannot open $decoder_cmd_file";
  print DECODER_CMD <<"FILE_EOF";
#!/usr/bin/perl -w

use strict;

my %FULL2ABBR = map {my (\$a, \$b) = split/=/,\$_,2; (\$b, \$a);} split /\\s+/, "$ABBR_FULL_MAP";

open( ITERATION, "<$iteration_file") or die "Cannot open $iteration_file";
my \$iteration = <ITERATION>;
close( ITERATION);
chomp( \$iteration);

my \@features_order = qw( $feats_order );

# extract feature weights from last zmert iteration (stored in \$decoder_cfg_inter)
print "Updating decoder config file from file $decoder_cfg_inter\n";

my \$moses_ini = "$___CONFIG";

open( IN, "$decoder_cfg_inter") or die "Cannot open file $decoder_cfg_inter (reading updated lambdas)";
FILE_EOF

print DECODER_CMD <<'FILE_EOF';
my %lambdas = ();
my $lastName = "";
while( my $line = <IN>) {
  chomp($line); 
  my ($name, $val) = split( /\s+/, $line);
  $name =~ s/_\d+$//;      # remove index of the lambda
  push( @{$lambdas{$name}}, $val);
}
close(IN);


my $moses_ini_old = "$moses_ini";
$moses_ini_old =~ s/^(.*)\/([^\/]+)$/$1\/run$iteration.$2/;
$moses_ini_old = $moses_ini.".orig" if( $iteration == 0);
safesystem("mv $moses_ini $moses_ini_old");
# update moses.ini
open( INI_OLD, "<$moses_ini_old") or die "Cannot open config file $moses_ini_old";
open( INI, ">$moses_ini") or die "Cannot open config file $moses_ini";
while( my $line = <INI_OLD>) {
  if( $line =~ m/^\[(weight-.+)\]$/) {
    my $name = $FULL2ABBR{$1};
    print STDERR "Updating weight: $1, $name\n";
    print INI "$line";
    foreach( @{$lambdas{$name}}) {
      print INI "$_\n";
      print STDERR "NEW: $_\tOLD:";
      $line = <INI_OLD>;
      print STDERR $line;
    }
  } else {
    print INI $line;
  }
}
close(INI_OLD);
close(INI);

FILE_EOF

print DECODER_CMD <<"FILE_EOF";
print "Executing: $decoder_cmd";
safesystem("$decoder_cmd") or die "Failed to execute $decoder_cmd";

# update iteration number in intermediate config file
++\$iteration;
safesystem("echo \$iteration > $iteration_file");

# modify the nbest-list to conform the zmert required format
# <i> ||| <candidate_translation> ||| featVal_1 featVal_2 ... featVal_m
my \$nbest_file_orig = "$nbest_file".".orig";
safesystem( "mv $nbest_file \$nbest_file_orig");
open( NBEST_ORIG, "<\$nbest_file_orig") or die "Cannot open original nbest-list \$nbest_file_orig";
open( NBEST, ">$nbest_file") or die "Cannot open modified nbest-list $nbest_file";

my \$line_num = 0;

FILE_EOF


if( "$___EXTRACT_SEMPOS" =~ /factors/) {
  print DECODER_CMD <<"FILE_EOF";
my (undef, \$args) = split( /:/, "$___EXTRACT_SEMPOS");
my \$factor_count = $metric_num_factors;
FILE_EOF
print DECODER_CMD <<'FILE_EOF';
my @indices = split( /,/, $args);
die "Specified ".scalar @indices." factors to extract but selected metric requires $factor_count factors" 
  if( @indices != $factor_count);
while( my $line = <NBEST_ORIG>) {
  my @array = split( /\|\|\|/, $line);
  # remove feature names from the feature scores string
  $array[2] = extractScores( $array[2]);
  my @tokens = split( /\s/, $array[1]); # split sentence into words
  $array[1] = "";
  foreach my $token (@tokens) {
    next if $token eq "";
    my @factors = split( /\|/, $token);
    my $put_separator = 0;
    foreach my $index (@indices) {
      die "Cannot extract factor with index $index from '$token'" if ($index > $#factors);
      $array[1] .= '|' if ($put_separator);	# separator between factors
      $array[1] .= $factors[$index];
      $put_separator = 1;
    }
    $array[1] .= " ";	# space between words
  }
  print NBEST join( '|||', @array);
}
 
FILE_EOF

} elsif( "$___EXTRACT_SEMPOS" =~ /tmt/) {
  print DECODER_CMD <<"FILE_EOF";
# run TectoMT to analyze sentences
print STDERR "Analyzing candidates using $srunblocks_cmd\n"; 
my \$nbest_factored = "$nbest_file.factored";
open( NBEST_FACTORED, "|$srunblocks_cmd > \$nbest_factored") or die "Cannot open pipe to command $srunblocks_cmd";
FILE_EOF
print DECODER_CMD <<'FILE_EOF';
my $line_count = 0;
my @out = ();
while( my $line = <NBEST_ORIG>) {
  my @array = split( /\|\|\|/, $line);
  die "Nbest-list does not have required format (values separated by '|||')" if ($#array != 3);
  # remove feature names from the feature scores string
  $array[2] = extractScores( $array[2]);
  push( @out, \@array); # store line with scores for output
  # select only word forms
  my $sentence = "";
  foreach my $fact ( split /\s+/, $array[1]) {
    next if( $fact eq "");
    my @fact_array = split( /\|/, $fact);
    $sentence .= "$fact_array[0] ";
  }
  # analyze sentence via TectoMT using scenario
  print NBEST_FACTORED "$sentence\n";
  ++$line_count;
}
close( NBEST_ORIG);
close( NBEST_FACTORED);

open( NBEST_FACTORED, "<$nbest_factored") or die "Cannot open $nbest_factored";
my $line_count_check = 0;
while( my $line = <NBEST_FACTORED>) {
  chomp( $line);
  my $array_ref = shift( @out);
  $array_ref->[1] = $line;  
  print NBEST join( '|||', @{$array_ref});
  ++$line_count_check;
}
die "Error: Sent $line_count sentences to analyze but got only $line_count_check back" 
  if( $line_count != $line_count_check);

FILE_EOF

} elsif ($___EXTRACT_SEMPOS eq "none") {
print DECODER_CMD <<'FILE_EOF';
while( my $line = <NBEST_ORIG>) {
  my @array = split( /\|\|\|/, $line);
  # remove feature names from the feature scores string
  $array[2] = extractScores( $array[2]);
  print NBEST join( '|||', @array);
}
FILE_EOF
} else {
  die "Unknown type of factor extraction: $___EXTRACT_SEMPOS";
}

print DECODER_CMD <<'FILE_EOF';
close( NBEST);
close( NBEST_ORIG);

# END OF BODY

sub extractScores {
  my $scores = shift;
  my (%scores_hash, $name); 
  foreach my $score_or_name (split /\s+/, $scores) {
    if( $score_or_name =~ s/://) {
      $name = $score_or_name; 
    } elsif ($score_or_name =~ /\d/) {
      die "Cannot guess nbest-list first feature score name" if( not defined $name);
      $scores_hash{$name} .= "$score_or_name ";
    } else {
      die "Unknown string ($score_or_name) in nbest-list feature scores section (not a feature name or score)" 
        if( $score_or_name =~ /\S/);
    }
  }
  $scores = "";
  foreach $name (@features_order) {
    $scores .= $scores_hash{$name};
  }
  #print STDERR "REORDERED SCORES: $scores\n";
  return $scores;
}

sub safesystem {
  print STDERR "Executing: @_\n";
  system(@_);
  if ($? == -1) {
      print STDERR "Failed to execute: @_\n  $!\n";
      exit(1);
  }
  elsif ($? & 127) {
      printf STDERR "Execution of: @_\n  died with signal %d, %s coredump\n",
          ($? & 127),  ($? & 128) ? 'with' : 'without';
      exit(1);
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}
FILE_EOF

close( DECODER_CMD);

# make the decoder lauch script executable
safesystem("chmod a+x $decoder_cmd_file");

# analyze reference if necessary
if( $___EXTRACT_SEMPOS =~ /tmt/) {
  my $part = 0;
  foreach my $ref (@references) {
    my $line_count = 0;
    print STDERR "Analyzing references using $srunblocks_cmd\n"; 
    open( REF_IN, "<$ref") or die "Cannot open $ref";
    my $ref_factored = "$ref.factored.$part";
    push( @references_factored, $ref_factored);
    open( REF_FACTORED, "|$srunblocks_cmd > $ref_factored");
    while( my $line = <REF_IN>) {
      # analyze sentence via TectoMT using scenario in file $scerario_file
      print REF_FACTORED $line;
      ++$line_count;
    }
    close( REF_IN);
    close( REF_FACTORED);
    my $line_count_check = 0;
    open( REF_FACTORED, "<$ref_factored") or die "Cannot open $ref_factored";
    ++$line_count_check while( <REF_FACTORED>);
    die "Error: Sent $line_count sentences to analyze but got $line_count_check back"
     if( $line_count != $line_count_check);  
    close( REF_FACTORED);
    ++$part;
  }
  print STDERR "References analyzed\n";
} else {
  push( @references_factored, @references);
}

my $ref_stem = $references_factored[0];
$ref_stem =~ s/\d+$// if( $#references_factored); # get the file stem if we have more than one refs
$ref_stem =~ s/.*\/([^\/]+)$/..\/$1/; 

# prepare zmert configuration file
open( ZMERT_CFG, ">$zmert_cfg") or die "Cannot open $zmert_cfg";

# FILES
# print ZMERT_CFG "-dir\t$___PATH_FROM_LAUNCHDIR\n";	# working path (relative to the lauch path)
# print ZMERT_CFG "-r\t$___DEV_E\n";	# file(s) containing references
print ZMERT_CFG "-r\t$ref_stem\n";	# file(s) containing references
print ZMERT_CFG "-rps\t".scalar(@references)."\n";	# number of references per sentence
print ZMERT_CFG "-txtNrm\t0\n";	# we use our own text normalization
print ZMERT_CFG "-p\t$opt_params\n";	# file containig parameter names, initial values, ranges
print ZMERT_CFG "-fin\t$___LAMBDAS_OUT\n" if(defined $___LAMBDAS_OUT);	# file where the final weight vector is written

# MERT CONFIGURATION
print ZMERT_CFG "-m\t$___METRIC\n";	
print ZMERT_CFG "-maxIt\t$___MAX_MERT_ITER\n" if( $___MAX_MERT_ITER);	# maximum number of MERT iterations
# print ZMERT_CFG "-prevIt\t$PREV_MERT_ITER\n";	
# number of iteration before considering an early exit
# print ZMERT_CFG "-minIt\t$MIN_MERT_ITER\n";	
# number of consecutive iterations that must satisfy some early stopping 
# criterion to cause an early exit
# print ZMERT_CFG "-stopIt\t$STOP_MIN_ITER\n";	
# early exit criterion: no weight changes by more than $LAMBDA_CHANGE; 
# default value: -1 (this criterion is never investigated)
# print ZMERT_CFG "-stopSig\t$LAMBDA_CHANGE\n";
# save intermediate decoder config files (1) or decoder outputs (2) or both (3) or neither (0)
print ZMERT_CFG "-save\t$___SAVE_INTER\n";
# print ZMERT_CFG "-ipi\t$INITS_PER_ITER\n";	# number of intermediate initial points per iteration
# print ZMERT_CFG "-opi\t$ONCE_PER_ITER\n";	# modify a parameter only once per iteration;
# print ZMERT_CFG "-rand\t$RAND_INIT\n";		# choose initial points randomly
print ZMERT_CFG "-seed\t$___PREDICTABLE_SEEDS\n" if($___PREDICTABLE_SEEDS);	# initialize the random number generator

# DECODER SPECIFICATION
print ZMERT_CFG "-cmd\t$decoder_cmd_file\n";	# name of file containing commands to run the decoder
print ZMERT_CFG "-decOut\t$nbest_file\n";	# name of the n-best file produced by the decoder
# print ZMERT_CFG "-decExit\t$DECODER_EXIT_CODE\n";	# value returned by decoder after successful exit
print ZMERT_CFG "-dcfg\t$decoder_cfg_inter\n";		# name of intermediate decoder configuration file
print ZMERT_CFG "-N\t$___N_BEST_LIST_SIZE\n";	

# OUTPUT SPECIFICATION
print ZMERT_CFG "-v\t$___MERT_VERBOSE\n";	# zmert verbosity level (0-2)
print ZMERT_CFG "-decV\t$___DECODER_VERBOSE\n";	# decoder output printed (1) or ignored (0)

close( ZMERT_CFG);

my ($name, $num, $val, $min, $max);
# prepare file with parameters to optimize
open( PARAMS, ">$opt_params") or die "Cannot open file $opt_params with parameters to optimize";
my $optString;
foreach $name (keys %used_triples) {
  $num = 0;
  foreach my $triple (@{$used_triples{$name}}) {
    ($val, $min, $max) = @$triple;
    my ($minRand, $maxRand) = ($min, $max);
    # the file should describe features to optimize in the following format:
    # "featureName ||| defValue optString minVal maxVal minRandVal maxRandVal"
    #    optString can be 'Opt' or 'Fix' 
   $optString = "Opt"; 
   if( defined $___ACTIVATE_FEATURES and not $active_features{$name."_$num"}) {
     $optString = "Fix";
   } 
   print PARAMS "$name"."_$num ||| $val $optString $min $max $minRand $maxRand\n";
    ++$num;
  }
}
print PARAMS  "normalization = $___NORM\n";
close( PARAMS);

# prepare intermediate config file from which moses.ini will be updated before each launch
open( DEC_CFG, ">$decoder_cfg_inter") or die "Cannot open file $decoder_cfg_inter";
foreach $name (keys %used_triples) {
  $num = 0;
  foreach my $tri (@{$used_triples{$name}}) {
    ($val, $min, $max) = @$tri;
    print DEC_CFG $name."_$num $val\n";
    ++$num;
  }
}
close( DEC_CFG);

open( ITER, ">$iteration_file") or die "Cannot open file $iteration_file";
print ITER "1"; 
close( ITER);

# launch zmert
my $javaMaxMem = ""; # -maxMem 4000" # use at most 4000MB of memory
my $cmd = "java -cp $zmert_classpath ZMERT $javaMaxMem $zmert_cfg";

print "Zmert start at ".`date`;

if ( 0 && defined $___JOBS) {
  # NOT WORKING - this branch needs to init environment variables
  safesystem("$qsubwrapper $pass_old_sge -command='$cmd' -stderr=$mert_logfile -queue-parameter='$queue_flags'") or die "Failed to start zmert (via qsubwrapper $qsubwrapper)";

} else {
  safesystem("$cmd 2> $mert_logfile") or die "Failed to run zmert";
}

print "Zmert finished at ".`date`;

# RELEVANT ONLY FOR PLAYGROUND at UFAL, CHARLES UNIVESITY IN PRAGUE
# copy optimized moses.ini and original run1.moses.ini to the working directory
if( $___FILTER_PHRASE_TABLE) {
  my ($config_opt, $config_std, $config_base) = ($___CONFIG, $___CONFIG, "$cwd/moses.abs.ini");
  $config_std =~ s/^(.*)\/([^\/]+)$/$1\/run1.$2/;
  mergeConfigs( $config_base, $___CONFIG);
  mergeConfigs( $config_base, $config_std);
}

# chdir back to the original directory # useless, just to remind we were not there
chdir($cwd);


} # end of local scope

sub mergeConfigs {
  my ($config_base, $config_weights) = @_;
  my $config_new = $config_weights;
  $config_new =~ s/^.*\///;
  open BASE, "<$config_base" or die "Cannot open $config_base";
  open WEIGHTS, "<$config_weights" or die "Cannot open $config_weights";
  open NEW, ">$config_new" or die "Cannot open $config_new";
  my $cont = 1;
  my ($b_line, $w_line);
  while( $cont) {
    $b_line = <BASE>;
    $w_line = <WEIGHTS>;
    $cont = (defined $b_line and defined $w_line);
    if( $b_line =~ /^\[weight-/) {
      if( $w_line !~ /^\[weight-/) { die "mergeConfigs: $config_base and $config_weights do not have the same format"; }
      print NEW $w_line;
      $b_line = <BASE>; $w_line = <WEIGHTS>;
      while( $w_line =~ /\d/) {
        print NEW $w_line;
        $b_line = <BASE>; $w_line = <WEIGHTS>;
      }
      print NEW $b_line;
    } else {
      print NEW $b_line;
    }
  }
  close BASE;
  close WEIGHTS;
  close NEW;
}

sub dump_triples {
  my $triples = shift;

  foreach my $name (keys %$triples) {
    foreach my $triple (@{$triples->{$name}}) {
      my ($val, $min, $max) = @$triple;
    }
  }
}

sub safesystem {
  print STDERR "Executing: @_\n";
  system(@_);
  if ($? == -1) {
      print STDERR "Failed to execute: @_\n  $!\n";
      exit(1);
  }
  elsif ($? & 127) {
      printf STDERR "Execution of: @_\n  died with signal %d, %s coredump\n",
          ($? & 127),  ($? & 128) ? 'with' : 'without';
      exit(1);
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}

sub ensure_full_path {
    my $PATH = shift;
$PATH =~ s/\/nfsmnt//;
    return $PATH if $PATH =~ /^\//;
    my $dir = `pawd 2>/dev/null`; 
    if(!$dir){$dir = `pwd`;}
    chomp($dir);
    $PATH = $dir."/".$PATH;
    $PATH =~ s/[\r\n]//g;
    $PATH =~ s/\/\.\//\//g;
    $PATH =~ s/\/+/\//g;
    my $sanity = 0;
    while($PATH =~ /\/\.\.\// && $sanity++<10) {
        $PATH =~ s/\/+/\//g;
        $PATH =~ s/\/[^\/]+\/\.\.\//\//g;
    }
    $PATH =~ s/\/[^\/]+\/\.\.$//;
    $PATH =~ s/\/+$//;
$PATH =~ s/\/nfsmnt//;
    return $PATH;
}

sub scan_config {
  my $ini = shift;
  my $inishortname = $ini; $inishortname =~ s/^.*\///; # for error reporting
  # we get a pre-filled counts, because some lambdas are always needed (word penalty, for instance)
  # as we walk though the ini file, we record how many extra lambdas do we need
  # and finally, we report it

  # in which field (counting from zero) is the filename to check?
  my %where_is_filename = (
    "ttable-file" => 4,
    "generation-file" => 3,
    "lmodel-file" => 3,
    "distortion-file" => 3,
    "global-lexical-file" => 1,
  );
  # by default, each line of each section means one lambda, but some sections
  # explicitly state a custom number of lambdas
  my %where_is_lambda_count = (
    "ttable-file" => 3,
    "generation-file" => 2,
    "distortion-file" => 2,
  );
  
  open INI, $ini or die "Can't read $ini";
  my $section = undef;  # name of the section we are reading
  my $shortname = undef;  # the corresponding short name
  my $nr = 0;
  my $error = 0;
  my %defined_files;
  my %defined_steps;  # check the ini file for compatible mapping steps and actually defined files
  while (<INI>) {
    $nr++;
    next if /^\s*#/; # skip comments
    if (/^\[([^\]]*)\]\s*$/) {
      $section = $1;
      $shortname = $TABLECONFIG2ABBR{$section};
      next;
    }
    if (defined $section && $section eq "mapping") {
      # keep track of mapping steps used
      $defined_steps{$1}++ if /^([TG])/ || /^\d+ ([TG])/;
    }
    if (defined $section && defined $where_is_filename{$section}) {
      print "$section -> $where_is_filename{$section}\n";
      # this ini section is relevant to lambdas
      chomp;
      my @flds = split / +/;
      my $fn = $flds[$where_is_filename{$section}];
      if (defined $fn && $fn !~ /^\s+$/) {
	  print "checking weight-count for $section\n";
        # this is a filename! check it
	if ($fn !~ /^\//) {
	  $error = 1;
	  print STDERR "$inishortname:$nr:Filename not absolute: $fn\n";
	}
	if (! -s $fn && ! -s "$fn.gz" && ! -s "$fn.binphr.idx" && ! -s "$fn.binlexr.idx" ) {
	  $error = 1;
	  print STDERR "$inishortname:$nr:File does not exist or empty: $fn\n";
	}
	# remember the number of files used, to know how many lambdas do we need
        die "No short name was defined for section $section!"
          if ! defined $shortname;

        # how many lambdas does this model need?
        # either specified explicitly, or the default, i.e. one
        my $needlambdas = defined $where_is_lambda_count{$section} ? $flds[$where_is_lambda_count{$section}] : 1;

        print STDERR "Config needs $needlambdas lambdas for $section (i.e. $shortname)\n" if $verbose;
        if (!defined $___LAMBDA && (!defined $additional_triples->{$shortname} || scalar(@{$additional_triples->{$shortname}}) < $needlambdas)) {
          print STDERR "$inishortname:$nr:Your model $shortname needs $needlambdas weights but we define the default ranges for only "
            .scalar(@{$additional_triples->{$shortname}})." weights. Cannot use the default, you must supply lambdas by hand.\n";
          $error = 1;
        }
	else {
	    # note: table may use less parameters than the maximum number
	    # of triples
	    for(my $lambda=0;$lambda<$needlambdas;$lambda++) {
		my ($start, $min, $max) 
		    = @{${$additional_triples->{$shortname}}[$lambda]};
		push @{$used_triples{$shortname}}, [$start, $min, $max];
	    }
	}
        $defined_files{$shortname}++;
      }
    }
  }
  die "$inishortname: File was empty!" if !$nr;
  close INI;
  for my $pair (qw/T=tm=translation G=g=generation/) {
    my ($tg, $shortname, $label) = split /=/, $pair;
    $defined_files{$shortname} = 0 if ! defined $defined_files{$shortname};
    $defined_steps{$tg} = 0 if ! defined $defined_steps{$tg};

    if ($defined_files{$shortname} != $defined_steps{$tg}) {
      print STDERR "$inishortname: You defined $defined_files{$shortname} files for $label but use $defined_steps{$tg} in [mapping]!\n";
      $error = 1;
    }
  }

  # distance-based distortion
  if ($___ASYNC == 1)
  {
    print STDERR "ASYNC distortion & word penalty";
    my @my_array;
    for(my $i=0 ; $i < $defined_steps{"T"} ; $i++) 
    {
      push @my_array, [ 1.0, 0.0, 2.0 ];
    }
    push @{$used_triples{"d"}}, @my_array;

    @my_array = ();
    for(my $i=0 ; $i < $defined_steps{"T"} ; $i++) 
    {
      push @my_array, [ 0.5, -1.0, 1.0 ];
    }
    push @{$used_triples{"w"}}, @my_array;

    # debug print
    print "distortion:";
    my $refarray=$used_triples{"d"};
    my @vector=@$refarray;
    foreach my $subarray (@vector) {
      my @toto=@$subarray;
      print @toto,"\n";
    }
    #exit 1;
  }
  else
  { 
    print STDERR "SYNC distortion";
    push @{$used_triples{"d"}}, [1.0, 0.0, 2.0];
  }


  exit(1) if $error;
  return (\%defined_files);
}
