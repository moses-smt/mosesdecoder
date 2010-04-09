#!/usr/bin/perl -w 

# Usage:
# zmert-moses.pl <foreign> <english> <decoder-executable> <decoder-config>
# For other options see below or run 'zmert-moses.pl --help'

# Notes:
# <foreign> and <english> should be raw text files, one sentence per line
# <english> can be a prefix, in which case the files are <english>0, <english>1, etc. are used

# Revision history

# 29 Dec 2009 Derived from mert-moses-new.pl (Kamil Kos)


use FindBin qw($Bin);
use File::Basename;
my $SCRIPTS_ROOTDIR = $Bin;
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
my $___DECODER_VERBOSE = 0; # should decoder output be included? - 0:no,1:yes
my $usage = 0; # request for --help
my $___WORKING_DIR = "mert-work";
my $___DEV_F = undef; # required, input text to decode
my $___DEV_E = undef; # required, basename of files with references
my $___DECODER = undef; # required, pathname to the decoder executable
my $___CONFIG = undef; # required, pathname to startup ini file
my $___N_BEST_LIST_SIZE = 100;
my $queue_flags = "-l mem_free=0.5G -hard";  # extra parameters for parallelizer
      # the -l ws0ssmt is relevant only to JHU workshop
my $___JOBS = undef; # if parallel, number of jobs to use (undef -> serial)
my $___DECODER_FLAGS = ""; # additional parametrs to pass to the decoder
my $___LAMBDA = undef; # string specifying the seed weights and boundaries of all lambdas
my $skip_decoder = 0; # and should we skip the first decoder run (assuming we got interrupted during mert)
my $___FILTER_PHRASE_TABLE = 1; # filter phrase table
my $___PREDICTABLE_SEEDS = 0;
my $___METRIC = "BLEU 4 shortest"; # name of metric that will be used for minimum error training, followed by metric parameters (see zmert documentation)
my $___LAMBDAS_OUT = undef; # file where final lambdas should be written

# set 1 if using with async decoder
my $___ASYNC = 0; 

# Use "--norm" to select normalization in mert
my $___NORM = "none";

# set 0 if input type is text, set 1 if input type is confusion network
my $___INPUTTYPE = 0; 

my $mertdir = undef; # path to zmert directory
my $filtercmd = undef; # path to filter-model-given-input.pl
my $qsubwrapper = undef;
my $moses_parallel_cmd = undef;
my $old_sge = 0; # assume sge<6.0
my $___CONFIG_BAK = undef; # backup pathname to startup ini file
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
  "nbest=i" => \$___N_BEST_LIST_SIZE,
  "queue-flags=s" => \$queue_flags,
  "jobs=i" => \$___JOBS,
  "decoder-flags=s" => \$___DECODER_FLAGS,
  "lambdas=s" => \$___LAMBDA,
  "metric" => \$___METRIC,
  "norm" => \$___NORM,
  "help" => \$usage,
  "verbose" => \$verbose,
  "mert-verbose" => \$___MERT_VERBOSE,
  "decoder-verbose" => \$___DECODER_VERBOSE,
  "mertdir=s" => \$mertdir,
  "lambdas-out=s" => \$___LAMBDAS_OUT,
  "rootdir=s" => \$SCRIPTS_ROOTDIR,
  "filtercmd=s" => \$filtercmd, # allow to override the default location
  "qsubwrapper=s" => \$qsubwrapper, # allow to override the default location
  "mosesparallelcmd=s" => \$moses_parallel_cmd, # allow to override the default location
  "old-sge" => \$old_sge, #passed to moses-parallel
  "filter-phrase-table!" => \$___FILTER_PHRASE_TABLE, # allow (disallow)filtering of phrase tables
  "predictable-seeds" => \$___PREDICTABLE_SEEDS, # allow (disallow) switch on/off reseeding of random restarts
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
  --norm ... Select normalization for zmert
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

";
  exit 1;
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
  %active_features = map {$_ => 1} split( ",", $___ACTIVATE_FEATURES);
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

# as weights are normalized in the next steps (by cmert)
# normalize initial LAMBDAs, too
my $need_to_normalize = 1;

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

  # make a backup copy of startup ini file
  $___CONFIG_BAK = $___CONFIG;
  # the decoder should now use the filtered model
  $___CONFIG = "filtered/moses.ini";
}
else{
  # do not filter phrase tables (useful if binary phrase tables are available)
  # use the original configuration file
  $___CONFIG_BAK = $___CONFIG;
}

my $PARAMETERS;
$PARAMETERS = $___DECODER_FLAGS;

my $nbest_file = "zmert.best$___N_BEST_LIST_SIZE.out";

# Run zmert to optimize lambdas
# We need to prepare:
#	1) zmert configuration file (zmert_cfg.txt)
#	2) parameters we want to optimize (params.txt)
#	3) decoder configuration file (decoder_cfg_inter.txt)
#	4) decoder launch script (decoder_cmd) - must be executable

print "Zmert start at ".`date`;

my $zmert_cfg = ensure_full_path("zmert_cfg.txt");
my $opt_params = ensure_full_path("params.txt");
my $decoder_cfg_inter = ensure_full_path("decoder_cfg_inter.txt");
my $decoder_cmd_file = ensure_full_path("decoder_cmd");

my $LAMBDAS_FILE = ensure_full_path("finalWeights.txt");

# prepare zmert configuration file
open( ZMERT_CFG, ">$zmert_cfg") or die "Cannot open $zmert_cfg";

# FILES
print ZMERT_CFG "-dir $___WORKING_DIR\n";	# working path (relative to the lauch path)
print ZMERT_CFG "-r $___DEV_E\n";	# file(s) containing references
print ZMERT_CFG "-rps ".scalar(@references)."\n";	# number of references per sentence
# print ZMERT_CFG "-txtNrm 1\n";	# how should text be normalized
print ZMERT_CFG "-p $opt_params\n";	# file containig parameter names, initial values, ranges
print ZMERT_CFG "-fin $___LAMBDAS_OUT\n" if(defined $___LAMBDAS_OUT);	# file where the final weight vector is written

# MERT CONFIGURATION
print ZMERT_CFG "-m $___METRIC\n";	
# print ZMERT_CFG "-maxIt $MAX_MERT_ITER\n";	# maximum number of MERT iterations
# print ZMERT_CFG "-prevIt $PREV_MERT_ITER\n";	
# number of iteration before considering an early exit
# print ZMERT_CFG "-minIt $MIN_MERT_ITER\n";	
# number of consecutive iterations that must satisfy some early stopping 
# criterion to cause an early exit
# print ZMERT_CFG "-stopIt $STOP_MIN_ITER\n";	
# early exit criterion: no weight changes by more than $LAMBDA_CHANGE; 
# default value: -1 (this criterion is never investigated)
# print ZMERT_CFG "-stopSig $LAMBDA_CHANGE\n";
# save intermediate decoder config files (1) or decoder outputs (2) or both (3) or neither (0)
# print ZMERT_CFG "-save $SAVE_INTER\n";
# print ZMERT_CFG "-ipi $INITS_PER_ITER\n";	# number of intermediate initial points per iteration
# print ZMERT_CFG "-opi $ONCE_PER_ITER\n";	# modify a parameter only once per iteration;
# print ZMERT_CFG "-rand $RAND_INIT\n";		# choose initial points randomly
print ZMERT_CFG "-seed $___PREDICTABLE_SEEDS\n" if($___PREDICTABLE_SEEDS);	# initialize the random number generator

# DECODER SPECIFICATION
print ZMERT_CFG "-cmd $decoder_cmd_file\n";	# name of file containing commands to run the decoder
print ZMERT_CFG "-decOut $nbest_file\n"	# name of the n-best file produced by the decoder
# print ZMERT_CFG "-decExit $DECODER_EXIT_CODE\n";	# value returned by decoder after successful exit
print ZMERT_CFG "-dcfg $decoder_cfg_inter\n";		# name of intermediate decoder configuration file
print ZMERT_CFG "-N $___N_BEST_LIST_SIZE\n";	

# OUTPUT SPECIFICATION
print ZMERT_CFG "-v $___MERT_VERBOSE\n" if($___MERT_VERBOSE != 1);	# zmert verbosity level (0-2)
print ZMERT_CFG "-decV $___DECODER_VERBOSE\n" if($___DECODER_VERBOSE);	# decoder output printed (1) or ignored (0)

close( ZMERT_CFG);


# prepare file with parameters to optimize
open( PARAMS, ">$opt_params") or die "Cannot open file $opt_params with parameters to optimize";
my $optString;
foreach my $name (keys %used_triples) {
  my $num = 0;
  foreach my $triple (@{$used_triples{$name}}) {
    my ($val, $min, $max) = @$triple;
    my ($minRand, $maxRand) = ($min, $max);
    # the file should describe features to optimize in the following format:
    # "featureName ||| defValue optString minVal maxVal minRandVal maxRandVal"
    #    optString can be 'Opt' or 'Fix' 
   $optString = "Opt"; 
   if( defined $___ACTIVATE_FEATURES and not $active_features{$name}) {
     $optString = "Fix";
   } 
   print PARAMS "$name_$num \|\|\| $val $optString $min $max $minRand $maxRand\n";
    ++$num;
  }
}
print PARAMS  "normalization = $___NORM\n";
close( PARAMS);

# prepare intermediate config file from which moses.ini will be updated before each launch
open( DEC_CFG, ">$decoder_cfg_inter") or die "Cannot open file $decoder_cfg_inter";
foreach my $name (keys %used_triples) {
  my $num = 0;
  foreach my $triple (@{$used_triples{$name}}) {
    my ($val, $min, $max) = @$triple;
    print DEC_CFG "$name_$num $val\n";
    ++$num;
  }
}
close( DEC_CFG);

# prepare script that will launch moses from template
# it will include an update script that will adjust feature weights according to
# the last zmert iteration (they are stored in file $decoder_cfg_inter)

# prepare lauch command with all parameters
my $decoder_cmd;
if (defined $___JOBS) {
  $decoder_cmd = "$moses_parallel_cmd $pass_old_sge -config $___CONFIG -inputtype $___INPUTTYPE -qsub-prefix zmert -queue-parameters \\\\\"$queue_flags\\\\\" -decoder-parameters \\\\\"$parameters \$dec_params_updated\\\\\" -n-best-file $nbest_file -n-best-size $___N_BEST_LIST_SIZE -input-file $___DEV_F -jobs $___JOBS -decoder $___DECODER > moses.out";
} else {
  $decoder_cmd = "$___DECODER $parameters -config $___CONFIG -inputtype $___INPUTTYPE \$dec_params_updated -n-best-list $nbest_file $___N_BEST_LIST_SIZE -i $___DEV_F > moses.out";
}

my $template = "$SCRIPTS_ROOTDIR/trainng/zmert-decoder.pl.template";

safesystem("sed 's/___DECODER_CFG_INTER___/$decoder_cfg_inter/;s/___DECODER_CMD___/$decoder_cmd/' $template > $decoder_cmd_file");
# make the decoder lauch script executable
safesystem("chmod a+x $decoder_cmd_file");

# launch zmert
my $javaMaxMem = "" # -maxMem 4000" # use at most 4000MB of memory
my $cmd = "java -cp $zmert_classpath ZMERT $javaMaxMem $zmert_cfg";

if (defined $___JOBS) {
  safesystem("$qsubwrapper $pass_old_sge -command='$cmd' -stderr=$mert_logfile -queue-parameter=\"$queue_flags\"") or die "Failed to start zmert (via qsubwrapper $qsubwrapper)";
} else {
  safesystem("$cmd 2> $mert_logfile") or die "Failed to run zmert";
}

print "Zmert finished at ".`date`;

#chdir back to the original directory # useless, just to remind we were not there
chdir($cwd);

} # end of local scope

sub dump_triples {
  my $triples = shift;

  foreach my $name (keys %$triples) {
    foreach my $triple (@{$triples->{$name}}) {
      my ($val, $min, $max) = @$triple;
      print STDERR "Triples:  $name\t$val\t$min\t$max    ($triple)\n";
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
    "ttable-file" => 3,
    "generation-file" => 3,
    "lmodel-file" => 3,
    "distortion-file" => 3,
    "global-lexical-file" => 1,
  );
  # by default, each line of each section means one lambda, but some sections
  # explicitly state a custom number of lambdas
  my %where_is_lambda_count = (
    "ttable-file" => 2,
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

