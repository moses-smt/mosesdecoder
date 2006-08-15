#!/usr/bin/perl -w
# Usage:
# mert-moses.pl <foreign> <english> <decoder-executable> <decoder-config>
# For other options see below or run 'mert-moses.pl --help'

# Notes:
# <foreign> and <english> should be raw text files, one sentence per line
# <english> can be a prefix, in which case the files are <english>0, <english>1, etc. are used

# Revision history

# 31 Jl 1006 move gzip run*.out to avoid failure wit restartings
#            adding default paths
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

# defaults for initial values and ranges are:
my $default_triples = {
  # for each _d_istortion, _l_anguage _m_odel, _t_ranslation _m_odel and _w_ord penalty, there is a list
  # of [ default value, lower bound, upper bound ]-triples. In most cases, only one triple is used,
  # but the translation model has currently 5 features
  "d" => [ [ 1.0, 0.0, 2.0 ] ],
  "lm" => [ [ 1.0, 0.0, 2.0 ] ],
  "tm" => [
            [ 0.3, 0.0, 0.5 ],
            [ 0.2, 0.0, 0.5 ],
            [ 0.3, 0.0, 0.5 ],
            [ 0.2, 0.0, 0.5 ],
            [ 0.0, -1.0, 1.0 ],
	  ],
  "g" => [
           [ 1.0, 0.0, 2.0 ],
           [ 1.0, 0.0, 2.0 ],
         ],
  "w" => [ [ 0.0, -1.0, 1.0 ] ],
};

# moses.ini file uses FULL names for lambdas, while this training script internally (and on the command line)
# uses ABBR names.
my $ABBR_FULL_MAP = "d=weight-d lm=weight-l tm=weight-t w=weight-w g=weight-generation";
my %ABBR2FULL = map {split/=/,$_,2} split /\s+/, $ABBR_FULL_MAP;
my %FULL2ABBR = map {my ($a, $b) = split/=/,$_,2; ($b, $a);} split /\s+/, $ABBR_FULL_MAP;

# We parse moses.ini to figure out how many weights do we need to optimize.
# For this, we must know the correspondence between options defining files
# for models and options assigning weights to these models.
my $TABLECONFIG_ABBR_MAP = "ttable-file=tm lmodel-file=lm distortion-file=d generation-file=g";
my %TABLECONFIG2ABBR = map {split(/=/,$_,2)} split /\s+/, $TABLECONFIG_ABBR_MAP;

# There are weights that do not correspond to any input file, they just increase the total number of lambdas we optimize
my $extra_lambdas_for_model = {
  "w" => 1,  # word penalty
  "d" => 1,  # basic distortion
};




my $minimum_required_change_in_weights = 0.00001;
    # stop if no lambda changes more than this

my $verbose = 0;
my $usage = 0; # request for --help
my $___WORKING_DIR = "mert-work";
my $___DEV_F = undef; # required, input text to decode
my $___DEV_E = undef; # required, basename of files with references
my $___DECODER = undef; # required, pathname to the decoder executable
my $___CONFIG = undef; # required, pathname to startup ini file
my $___N_BEST_LIST_SIZE = 100;
my $queue_flags = "-l ws06ossmt=true -l mem_free=0.5G -hard";  # extra parameters for parallelizer
      # the -l ws0ssmt is relevant only to JHU workshop
my $___JOBS = undef; # if parallel, number of jobs to use (undef -> serial)
my $___DECODER_FLAGS = ""; # additional parametrs to pass to the decoder
my $___LAMBDA = undef; # string specifying the seed weights and boundaries of all lambdas
my $continue = 0; # should we try to continue from the last saved step?
my $skip_decoder = 0; # and should we skip the first decoder run (assuming we got interrupted during mert)

# Parameter for effective reference length when computing BLEU score
# This is used by score-nbest-bleu.py
# Default is to use shortest reference
# Use "--average" to use average reference length
my $___AVERAGE = 0;

my $allow_unknown_lambdas = 0;
my $allow_skipping_lambdas = 0;


my $SCRIPTS_ROOTDIR = undef; # path to all tools (overriden by specific options)
my $cmertdir = undef; # path to cmert directory
my $pythonpath = undef; # path to python libraries needed by cmert
my $filtercmd = undef; # path to filter-model-given-input.pl
my $SCORENBESTCMD = undef;
my $qsubwrapper = undef;
my $moses_parallel_cmd = undef;


use strict;
use Getopt::Long;
GetOptions(
  "working-dir=s" => \$___WORKING_DIR,
  "input=s" => \$___DEV_F,
  "refs=s" => \$___DEV_E,
  "decoder=s" => \$___DECODER,
  "config=s" => \$___CONFIG,
  "nbest=i" => \$___N_BEST_LIST_SIZE,
  "queue-flags=s" => \$queue_flags,
  "jobs=i" => \$___JOBS,
  "decoder-flags=s" => \$___DECODER_FLAGS,
  "lambdas=s" => \$___LAMBDA,
  "continue" => \$continue,
  "skip-decoder" => \$skip_decoder,
  "average" => \$___AVERAGE,
  "help" => \$usage,
  "allow-unknown-lambdas" => \$allow_unknown_lambdas,
  "allow-skipping-lambdas" => \$allow_skipping_lambdas,
  "verbose" => \$verbose,
  "roodir=s" => \$SCRIPTS_ROOTDIR,
  "cmertdir=s" => \$cmertdir,
  "pythonpath=s" => \$pythonpath,
  "filtercmd=s" => \$filtercmd, # allow to override the default location
  "scorenbestcmd=s" => \$SCORENBESTCMD, # path to score-nbest.py
  "qsubwrapper=s" => \$qsubwrapper, # allow to override the default location
  "mosesparallelcmd=s" => \$moses_parallel_cmd, # allow to override the default location
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


print STDERR "After default: $queue_flags\n";

if ($usage || !defined $___DEV_F || !defined$___DEV_E || !defined$___DECODER || !defined $___CONFIG) {
  print STDERR "usage: mert-moses.pl input-text references decoder-executable decoder.ini
Options:
  --working-dir=mert-dir ... where all the files are created
  --nbest=100 ... how big nbestlist to generate
  --jobs=N  ... set this to anything to run moses in parallel
  --mosesparallelcmd=STRING ... use a different script instead of moses-parallel
  --queue-flags=STRING  ... anything you with to pass to 
              qsub, eg. '-l ws06osssmt=true'
              The default is to submit the jobs to the ws06ossmt queue, which
              makes sense only at JHU. To reset the default JHU queue
              parameters, please use \"--queue-flags=' '\" (i.e. a space between
              the quotes).
  --decoder-flags=STRING ... extra parameters for the decoder
  --lambdas=STRING  ... default values and ranges for lambdas, a complex string
         such as 'd:1,0.5-1.5 lm:1,0.5-1.5 tm:0.3,0.25-0.75;0.2,0.25-0.75;0.2,0.25-0.75;0.3,0.25-0.75;0,-0.5-0.5 w:0,-0.5-0.5'
  --allow-unknown-lambdas ... keep going even if someone supplies a new lambda
         in the lambdas option (such as 'superbmodel:1,0-1'); optimize it, too
  --continue  ... continue from the last achieved state
  --skip-decoder ... skip the decoder run for the first time, assuming that
                     we got interrupted during optimization
  --average   ... Use either average or shortest (default) reference
                  length as effective reference length
  --filtercmd=STRING  ... path to filter-model-given-input.pl
  --roodir=STRING  ... where do helpers reside (if not given explicitly)
  --cmertdir=STRING ... where is cmert installed
  --pythonpath=STRING  ... where is python executable
  --scorenbestcmd=STRING  ... path to score-nbest.py
";
  exit 1;
}

# Check validity of input parameters and set defaults if needed




if (!defined $SCRIPTS_ROOTDIR) {
  $SCRIPTS_ROOTDIR = $ENV{"SCRIPTS_ROOTDIR"};
  die "Please set SCRIPTS_ROOTDIR or specify --rootdir" if !defined $SCRIPTS_ROOTDIR;
}

print STDERR "Using SCRIPTS_ROOTDIR: $SCRIPTS_ROOTDIR\n";




# path of script for filtering phrase tables and running the decoder
$filtercmd="$SCRIPTS_ROOTDIR/training/filter-model-given-input.pl" if !defined $filtercmd;

$qsubwrapper="$SCRIPTS_ROOTDIR/generic/qsub-wrapper.pl" if !defined $qsubwrapper;

$moses_parallel_cmd = "$SCRIPTS_ROOTDIR/generic/moses-parallel.pl"
  if !defined $moses_parallel_cmd;

$cmertdir = "$SCRIPTS_ROOTDIR/training/cmert-0.5" if !defined $cmertdir;
my $cmertcmd="$cmertdir/mert";

$SCORENBESTCMD = "$cmertdir/score-nbest.py" if ! defined $SCORENBESTCMD;

$pythonpath = "$cmertdir/python" if !defined $pythonpath;

$ENV{PYTHONPATH} = $pythonpath; # other scripts need to know


die "Not executable: $filtercmd" if ! -x $filtercmd;
die "Not executable: $cmertcmd" if ! -x $cmertcmd;
die "Not executable: $moses_parallel_cmd" if defined $___JOBS && ! -x $moses_parallel_cmd;
die "Not executable: $qsubwrapper" if defined $___JOBS && ! -x $qsubwrapper;
die "Not a dir: $pythonpath" if ! -d $pythonpath;
die "Not executable: $___DECODER" if ! -x $___DECODER;

my $input_abs = ensure_full_path($___DEV_F);
die "File not found: $___DEV_F (interpreted as $input_abs)."
  if ! -e $input_abs;
$___DEV_F = $input_abs;


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
my %copy_of_extra_lambdas_for_model = %$extra_lambdas_for_model;
my ($lambdas_per_model, $models_used) = scan_config($___CONFIG, \%copy_of_extra_lambdas_for_model);


# Parse the lambda config string and convert it to a nice structure in the same format as $default_triples
my $use_triples = undef;
if (defined $___LAMBDA) {
  # interpreting lambdas from command line
  foreach (split(/\s+/,$___LAMBDA)) {
      my ($name,$values) = split(/:/);
      die "Malformed setting: '$_', expected name:values\n" if !defined $name || !defined $values;
      foreach my $startminmax (split/;/,$values) {
	  if ($startminmax =~ /^(-?[\.\d]+),(-?[\.\d]+)-(-?[\.\d]+)$/) {
	      my $start = $1;
	      my $min = $2;
	      my $max = $3;
              push @{$use_triples->{$name}}, [$start, $min, $max];
	  }
	  else {
	      die "Malformed feature range definition: $name => $startminmax\n";
	  }
      } 
  }
} else {
  # no lambdas supplied, use the default ones, but do not forget to repeat them accordingly
  # first for or inherent models
  foreach my $name (keys %$extra_lambdas_for_model) {
    foreach (1..$extra_lambdas_for_model->{$name}) {
      die "No default weights defined for -$name"
        if !defined $default_triples->{$name};
      # XXX here was a deadly bug: we need a deep copy of the default values
      my @copy = ();
      foreach my $triple (@{$default_triples->{$name}}) {
        my @copy_triple = @$triple;
        push @copy, [ @copy_triple ];
      }
      push @{$use_triples->{$name}}, @copy;
    }
  }
  # and then for all models used
  foreach my $name (keys %$models_used) {
    foreach (1..$models_used->{$name}) {
      die "No default weights defined for -$name"
        if !defined $default_triples->{$name};
      # XXX here was a deadly bug: we need a deep copy of the default values
      my @copy = ();
      foreach my $triple (@{$default_triples->{$name}}) {
        my @copy_triple = @$triple;
        push @copy, [ @copy_triple ];
      }
      push @{$use_triples->{$name}}, @copy;
    }
  }
}

# moses should use our config
if ($___DECODER_FLAGS =~ /(^|\s)-(config|f) /
|| $___DECODER_FLAGS =~ /(^|\s)-(ttable-file|t) /
|| $___DECODER_FLAGS =~ /(^|\s)-(distortion-file) /
|| $___DECODER_FLAGS =~ /(^|\s)-(generation-file) /
|| $___DECODER_FLAGS =~ /(^|\s)-(lmodel-file) /
) {
  die "It is forbidden to supply any of -config, -ttable-file, -distortion-file, -generation-file or -lmodel-file in the --decoder-flags.\nPlease use only the --config option to give the config file that lists all the supplementary files.";
}

# walk through all lambdas the user wishes to optimize and check
# if the number of lambdas matches
foreach my $name (keys %$use_triples) {
  my $expected_lambdas = $lambdas_per_model->{$name};
  $expected_lambdas = 0 if !defined $expected_lambdas;
  my $got_lambdas = defined $use_triples->{$name} ? scalar @{$use_triples->{$name}}  : 0;
  if ($got_lambdas != $expected_lambdas) {
    if ($allow_unknown_lambdas && $expected_lambdas == 0) {
      print STDERR "Allowing to optimize $name, although I have no idea what it is.\n";
    } else {
      print STDERR "Wrong number of lambdas for $name. Expected (given the config file): $expected_lambdas, got: $got_lambdas.
Use --allow-unknown-lambdas to optimize lambdas that you are just introducing
and I cannot validate against the models mentioned in moses.ini.\n";
      exit 1;
    }
  }
}

# as weights are normalized in the next steps (by cmert)
# normalize initial LAMBDAs, too
my $need_to_normalize = 1;



my @order_of_lambdas_from_decoder = ();
# this will store the labels of scores coming out of the decoder (and hence the order of lambdas coming out of mert)
# we will use the array to interpret the lambdas
# the array gets filled with labels only after first nbestlist was generated




#store current directory and create the working directory (if needed)
my $cwd = `pwd`; chop($cwd);
safesystem("mkdir -p $___WORKING_DIR") or die "Can't mkdir $___WORKING_DIR";

{
# open local scope

#chdir to the working directory
chdir($___WORKING_DIR) or die "Can't chdir to $___WORKING_DIR";




# set start run
my $start_run = 1;

if ($continue) {
  # need to load last best values
  print STDERR "Trying to continue an interrupted optimization.\n";
  open IN, "finished_step.txt" or die "Failed to find the step number, failed to read finished_step.txt";
  my $step = <IN>;
  chomp $step;
  $step++;
  close IN;

  if (! -e "run$step.best$___N_BEST_LIST_SIZE.out.gz") {
    # allow stepping one extra iteration back
    $step--;
    die "Can't start from step $step, because run$step.best$___N_BEST_LIST_SIZE.out.gz was not found!"
      if ! -e "run$step.best$___N_BEST_LIST_SIZE.out.gz";
  }

  $start_run = $step +1;

  print STDERR "Reading last cached lambda values (result from step $step)\n";
  @order_of_lambdas_from_decoder = get_order_of_scores_from_nbestlist("gunzip -c < run$step.best$___N_BEST_LIST_SIZE.out.gz |");

  open IN, "weights.txt" or die "Can't read weights.txt";
  my $newweights = <IN>;
  chomp $newweights;
  close IN;
  my @newweights = split /\s+/, $newweights;

  # dump_triples($use_triples);
  $use_triples = store_new_lambda_values($use_triples, \@order_of_lambdas_from_decoder, \@newweights);
  # dump_triples($use_triples);
}



# filter the phrase tables, use --decoder-flags
print "filtering the phrase tables... ".`date`;
my $cmd = "$filtercmd ./filtered $___CONFIG $___DEV_F";
if (defined $___JOBS) {
  safesystem("$qsubwrapper -command='$cmd' -queue-parameter=\"$queue_flags\"" ) or die "Failed to submit filtering of tables to the queue (via $qsubwrapper)";
} else {
  safesystem($cmd) or die "Failed to filter the tables.";
}


# the decoder should now use the filtered model
my $PARAMETERS;
$PARAMETERS = $___DECODER_FLAGS . " -config filtered/moses.ini";

my $devbleu = undef;
my $bestpoint = undef;
my $run=$start_run-1;
my $prev_size = -1;
while(1) {
  $run++;
  # run beamdecoder with option to output nbestlists
  # the end result should be (1) @NBEST_LIST, a list of lists; (2) @SCORE, a list of lists of lists

  print "run $run start at ".`date`;

  # In case something dies later, we might wish to have a copy
  create_config($___CONFIG, "./run$run.moses.ini", $use_triples, $run, (defined$devbleu?$devbleu:"--not-estimated--"));


  # skip if the user wanted
  if (!$skip_decoder) {
      print "($run) run decoder to produce n-best lists\n";
      @order_of_lambdas_from_decoder = run_decoder($use_triples, $PARAMETERS, $run, \@order_of_lambdas_from_decoder, $need_to_normalize);
      $need_to_normalize = 0;
      safesystem("gzip -f run*out") or die "Failed to gzip run*out";
  }
  else {
      print "skipped decoder run\n";
      if (0 == scalar @order_of_lambdas_from_decoder) {
        @order_of_lambdas_from_decoder = get_order_of_scores_from_nbestlist("gunzip -dc run*.best*.out.gz | head -1 |");
      }
      $skip_decoder = 0;
      $need_to_normalize = 0;
  }

  my $EFF_REF_LEN = "";
  if ($___AVERAGE) {
     $EFF_REF_LEN = "-a";
  }

  # To be sure that scoring script produses these fresh:
  safesystem("rm -f cands.opt feats.opt") or die;
  
  # convert n-best list into a numberized format with error scores

  print STDERR "Scoring the nbestlist.\n";
  my $cmd = "export PYTHONPATH=$pythonpath ; gunzip -dc run*.best*.out.gz | sort -n -t \"|\" -k 1,1 | $SCORENBESTCMD $EFF_REF_LEN ".join(" ", @references)." ./";
  if (defined $___JOBS) {
    safesystem("$qsubwrapper -command='$cmd' -queue-parameter=\"$queue_flags\"") or die "Failed to submit scoring nbestlist to queue (via $qsubwrapper)";
  } else {
    safesystem($cmd) or die "Failed to score nbestlist";
  }


  print STDERR "Hoping that scoring succeeded. We'll see if we can read the output files now.\n";


  # keep a count of lines in nbests lists (alltogether)
  # if it did not increase since last iteration, we are DONE
  open(IN,"cands.opt") or die "Can't read cands.opt";
  my $size=0;
  while (<IN>) {
    chomp;
    my @flds = split / /;
    $size += $flds[1];
  }
  close(IN);
  print "$size accumulated translations\n";
  print "prev accumulated translations was : $prev_size\n";
  if ($size <= $prev_size){
     print STDERR "No new hypotheses in nbest list. Stopping.\n";
     last;
  }
  $prev_size = $size;


  # run cmert
  # cmert reads in the file init.opt containing three lines:
  #  minimum values
  #  maximum values
  #  current values
  # We need to prepare the files and **the order of the lambdas must
  # correspond to the order @order_of_lambdas_from_decoder

  my @MIN = ();   # lower bounds
  my @MAX = ();   # upper bounds
  my @CURR = ();   # the starting values
  my @NAME = ();  # to which model does the lambda belong
  
  # walk in order of @order_of_lambdas_from_decoder and collect the min,max,val
  my %visited = ();
  foreach my $name (@order_of_lambdas_from_decoder) {
    next if $visited{$name};
    $visited{$name} = 1;
    die "The decoder produced also some '$name' scores, but we do not know the ranges for them, no way to optimize them\n"
      if !defined $use_triples->{$name};
    foreach my $feature (@{$use_triples->{$name}}) {
      my ($val, $min, $max) = @$feature;
      push @CURR, $val;
      push @MIN, $min;
      push @MAX, $max;
      push @NAME, $name;
    }
  }

  open(OUT,"> init.opt") or die "Can't write init.opt (WD now $___WORKING_DIR)";
  print OUT join(" ", @MIN)."\n";
  print OUT join(" ", @MAX)."\n";
  print OUT join(" ", @CURR)."\n";
  close(OUT);

  #just for brevity
  open(OUT,"> names.txt") or die "Can't write names.txt (WD now $___WORKING_DIR)";
  print OUT join(" ", @NAME)."\n";
  close(OUT);

  # make a backup copy labelled with this run number
  safesystem("cp init.opt run$run.init.opt") or die;

  my $DIM = scalar(@CURR); # number of lambdas
  $cmd="$cmertcmd -d $DIM";
 
  print STDERR "Starting cmert.\n";
  if (defined $___JOBS) {
    safesystem("$qsubwrapper -command='$cmd' -stderr=cmert.log -queue-parameter=\"$queue_flags\"") or die "Failed to start cmert (via qsubwrapper $qsubwrapper)";
  } else {
    safesystem("$cmd 2> cmert.log") or die "Failed to run cmert";
  }
  die "Optimization failed, file weights.txt does not exist or is empty"
    if ! -s "weights.txt";
  # backup copies
  safesystem ("cp cmert.log run$run.cmert.log") or die;
  safesystem ("cp weights.txt run$run.weights.txt") or die; # this one is needed for restarts, too
  print "run $run end at ".`date`;

  $bestpoint = undef;
  $devbleu = undef;
  open(IN,"cmert.log") or die "Can't open cmert.log";
  while (<IN>) {
    if (/Best point:\s*([\s\d\.\-]+?)\s*=> ([\d\.]+)/) {
      $bestpoint = $1;
      $devbleu = $2;
      last;
    }
  }
  close IN;
  die "Failed to parse cmert.log, missed Best point there."
    if !defined $bestpoint || !defined $devbleu;
  print "($run) BEST at $run: $bestpoint => $devbleu at ".`date`;

  my @newweights = split /\s+/, $bestpoint;

  # update my cache of lambda values
  $use_triples = store_new_lambda_values($use_triples, \@order_of_lambdas_from_decoder, \@newweights);

  ## additional stopping criterion: weights have not changed
  my $shouldstop = 1;
  for(my $i=0; $i<@CURR; $i++) {
    die "Lost weight! cmert reported fewer weights (@newweights) than we gave it (@CURR)"
      if !defined $newweights[$i];
    if (abs($CURR[$i] - $newweights[$i]) >= $minimum_required_change_in_weights) {
      $shouldstop = 0;
      last;
    }
  }

  open F, "> finished_step.txt" or die "Can't mark finished step";
  print F $run."\n";
  close F;


  if ($shouldstop) {
    print STDERR "None of the weights changed more than $minimum_required_change_in_weights. Stopping.\n";
    last;
  }

}
print "Training finished at ".`date`;

safesystem("cp init.opt run$run.init.opt") or die;
safesystem ("cp cmert.log run$run.cmert.log") or die;

create_config($___CONFIG, "./moses.ini", $use_triples, $run, $devbleu);

# just to be sure that we have the really last finished step marked
open F, "> finished_step.txt" or die "Can't mark finished step";
print F $run."\n";
close F;


#chdir back to the original directory # useless, just to remind we were not there
chdir($cwd);

} # end of local scope


sub store_new_lambda_values {
  # given new lambda values (in given order), replace the 'val' element in our triples
  my $triples = shift;
  my $names = shift;
  my $values = shift;

  my %idx = ();
  foreach my $i (0..scalar(@$values)-1) {
    my $name = $names->[$i];
    die "Missed name for lambda $values->[$i] (in @$values; names: @$names)"
      if !defined $name;
    if (!defined $idx{$name}) {
      $idx{$name} = 0;
    } else {
      $idx{$name}++;
    }
    die "We did not optimize '$name', but moses returned it back to us"
      if !defined $triples->{$name};
    die "Moses gave us too many lambdas for '$name', we had ".scalar(@{$triples->{$name}})
      ." but we got at least ".$idx{$name}+1
      if !defined $triples->{$name}->[$idx{$name}];

    # set the corresponding field in triples
    # print STDERR "Storing $i-th score as $name: $idx{$name}: $values->[$i]\n";
    $triples->{$name}->[$idx{$name}]->[0] = $values->[$i];
  }
  return $triples;
}

sub dump_triples {
  my $triples = shift;

  foreach my $name (keys %$triples) {
    foreach my $triple (@{$triples->{$name}}) {
      my ($val, $min, $max) = @$triple;
      print STDERR "Triples:  $name\t$val\t$min\t$max    ($triple)\n";
    }
  }
}


sub run_decoder {
    my ($triples, $parameters, $run, $output_order_of_lambdas, $need_to_normalize) = @_;
    my $filename_template = "run%d.best$___N_BEST_LIST_SIZE.out";
    my $filename = sprintf($filename_template, $run);
    
    print "params = $parameters\n";
    # prepare the decoder config:
    my $decoder_config = "";
    my @vals = ();
    foreach my $name (keys %$triples) {
      $decoder_config .= "-$name ";
      foreach my $triple (@{$triples->{$name}}) {
        my ($val, $min, $max) = @$triple;
        $decoder_config .= "%.6f ";
        push @vals, $val;
      }
    }
    if ($need_to_normalize) {
      print STDERR "Normalizing lambdas: @vals\n";
      my $totlambda=0;
      grep($totlambda+=abs($_),@vals);
      grep($_/=$totlambda,@vals);
    }
    print STDERR "DECODER_CFG = $decoder_config\n";
    print STDERR "     values = @vals\n";
    $decoder_config = sprintf($decoder_config, @vals);
    print "decoder_config = $decoder_config\n";

    # run the decoder
    my $decoder_cmd;
    if (defined $___JOBS) {
      $decoder_cmd = "$moses_parallel_cmd -qsub-prefix mert$run -queue-parameters \"$queue_flags\" $parameters $decoder_config -n-best-file $filename -n-best-size $___N_BEST_LIST_SIZE -input-file $___DEV_F -jobs $___JOBS -decoder $___DECODER > run$run.out";
    } else {
      $decoder_cmd = "$___DECODER $parameters $decoder_config -n-best-list $filename $___N_BEST_LIST_SIZE -i $___DEV_F > run$run.out";
    }

    safesystem($decoder_cmd) or die "The decoder died.";

    if (0 == scalar @$output_order_of_lambdas) {
      # we have to peek at the nbestlist
      return get_order_of_scores_from_nbestlist($filename);
    } else {
      # we have checked the nbestlist already, we trust the order of output scores does not change
      return @$output_order_of_lambdas;
    }
}

sub get_order_of_scores_from_nbestlist {
  # read the first line and interpret the ||| label: num num num label2: num ||| column in nbestlist
  # return the score labels in order
  my $fname_or_source = shift;
  print STDERR "Peeking at the beginning of nbestlist to get order of scores: $fname_or_source\n";
  open IN, $fname_or_source or die "Failed to get order of scores from nbestlist '$fname_or_source'";
  my $line = <IN>;
  close IN;
  die "Line empty in nbestlist '$fname_or_source'" if !defined $line;
  my ($sent, $hypo, $scores, $total) = split /\|\|\|/, $line;
  $scores =~ s/^\s*|\s*$//g;
  die "No scores in line: $line" if $scores eq "";

  my @order = ();
  my $label = undef;
  foreach my $tok (split /\s+/, $scores) {
    if ($tok =~ /^([a-z][0-9a-z]*):/i) {
      $label = $1;
    } elsif ($tok =~ /^-?[-0-9.e]+$/) {
      # a score found, remember it
      die "Found a score but no label before it! Bad nbestlist '$fname_or_source'!"
        if !defined $label;
      push @order, $label;
    } else {
      die "Not a label, not a score '$tok'. Failed to parse the scores string: '$scores' of nbestlist '$fname_or_source'";
    }
  }
  print STDERR "The decoder returns the scores in this order: @order\n";
  return @order;
}

sub create_config {
    my $infn = shift; # source config
    my $outfn = shift; # where to save the config
    my $triples = shift; # the lambdas we should write
    my $iteration = shift;  # just for verbosity
    my $bleu_achieved = shift; # just for verbosity

    my %P; # the hash of all parameters we wish to override

    # first convert the command line parameters to the hash
    { # ensure local scope of vars
	my $parameter=undef;
	print "Parsing --decoder-flags: |$___DECODER_FLAGS|\n";
        $___DECODER_FLAGS =~ s/^\s*|\s*$//;
        $___DECODER_FLAGS =~ s/\s+/ /;
	foreach (split(/ /,$___DECODER_FLAGS)) {
	    if (/^\-([^\d].*)$/) {
		$parameter = $1;
		$parameter = $ABBR2FULL{$parameter} if defined($ABBR2FULL{$parameter});
	    }
	    else {
                die "Found value with no -paramname before it: $_"
                  if !defined $parameter;
		push @{$P{$parameter}},$_;
	    }
	}
    }

    # Convert weights to elements in P
    foreach my $abbr (keys %$triples) {
      # First delete all weights params from the input, in short or long-named version
      delete($P{$abbr});
      delete($P{$ABBR2FULL{$abbr}});
      # Then feed P with the current values
      foreach my $feature (@{$use_triples->{$abbr}}) {
        my ($val, $min, $max) = @$feature;
        my $name = defined $ABBR2FULL{$abbr} ? $ABBR2FULL{$abbr} : $abbr;
        push @{$P{$name}}, $val;
      }
    }

    # create new moses.ini decoder config file by cloning and overriding the original one
    open(INI,$infn) or die "Can't read $infn";
    delete($P{"config"}); # never output 
    print "Saving new config to: $outfn\n";
    open(OUT,"> $outfn") or die "Can't write $outfn";
    print OUT "# MERT optimized configuration\n";
    print OUT "# decoder $___DECODER\n";
    print OUT "# BLEU $bleu_achieved on dev $___DEV_F\n";
    print OUT "# We were before running iteration $iteration\n";
    print OUT "# finished ".`date`;
    my $line = <INI>;
    while(1) {
	last unless $line;

	# skip until hit [parameter]
	if ($line !~ /^\[(.+)\]\s*$/) { 
	    $line = <INI>;
	    print OUT $line if $line =~ /^\#/ || $line =~ /^\s+$/;
	    next;
	}

	# parameter name
	my $parameter = $1;
	$parameter = $ABBR2FULL{$parameter} if defined($ABBR2FULL{$parameter});
	print OUT "[$parameter]\n";

	# change parameter, if new values
	if (defined($P{$parameter})) {
	    # write new values
	    foreach (@{$P{$parameter}}) {
		print OUT $_."\n";
	    }
	    delete($P{$parameter});
	    # skip until new parameter, only write comments
	    while($line = <INI>) {
		print OUT $line if $line =~ /^\#/ || $line =~ /^\s+$/;
		last if $line =~ /^\[/;
		last unless $line;
	    }
	    next;
	}
	
	# unchanged parameter, write old
	while($line = <INI>) {
	    last if $line =~ /^\[/;
	    print OUT $line;
	}
    }

    # write all additional parameters
    foreach my $parameter (keys %P) {
	print OUT "\n[$parameter]\n";
	foreach (@{$P{$parameter}}) {
	    print OUT $_."\n";
	}
    }

    close(INI);
    close(OUT);
    print STDERR "Saved: $outfn\n";
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
    return $PATH if $PATH =~ /^\//;
    $PATH = `pwd`."/".$PATH;
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
    return $PATH;
}




sub scan_config {
  my $ini = shift;
  my $inishortname = $ini; $inishortname =~ s/^.*\///; # for error reporting
  my $lambda_counts = shift;
  # we get a pre-filled counts, because some lambdas are always needed (word penalty, for instance)
  # as we walk though the ini file, we record how many extra lambdas do we need
  # and finally, we report it

  # in which field (counting from zero) is the filename to check?
  my %where_is_filename = (
    "ttable-file" => 3,
    "generation-file" => 3,
    "lmodel-file" => 3,
    "distortion-file" => 0,
  );
  # by default, each line of each section means one lambda, but some sections
  # explicitly state a custom number of lambdas
  my %where_is_lambda_count = (
    "ttable-file" => 2,
    "generation-file" => 2,
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
      $defined_steps{$1}++ if /^([TG])/;
    }
    if (defined $section && defined $where_is_filename{$section}) {
      # this ini section is relevant to lambdas
      chomp;
      my @flds = split / +/;
      my $fn = $flds[$where_is_filename{$section}];
      if (defined $fn && $fn !~ /^\s+$/) {
        # this is a filename! check it
	if ($fn !~ /^\//) {
	  $error = 1;
	  print STDERR "$inishortname:$nr:Filename not absolute: $fn\n";
	}
	if (! -s $fn) {
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
	$lambda_counts->{$shortname}+=$needlambdas;
        if (!defined $___LAMBDA && (!defined $default_triples->{$shortname} || scalar(@{$default_triples->{$shortname}}) != $needlambdas)) {
          print STDERR "$inishortname:$nr:Your model $shortname needs $needlambdas weights but we define the default ranges for "
            .scalar(@{$default_triples->{$shortname}})." weights. Cannot use the default, you must supply lambdas by hand.\n";
          $error = 1;
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
  exit(1) if $error;
  return ($lambda_counts, \%defined_files);
}

