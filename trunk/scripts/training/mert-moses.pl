#!/usr/bin/perl -w

# Usage:
# mert-moses-parallel.perl <working-dir> <foreign> <english> <n-best-size> <decoder-parallel> <jobs> <decoder> <decoder-config> <other-decoder-params> <lambdas> <start-step>

# Notes:
# <foreign> and <english> should be raw text files, one sentence per line
# <english> can be a prefix, in which case the files are <english>0, <english>1, etc.
# <start-step> is what iteration to start at (default 1). If you add an 'a'
#   suffix the decoding for that iteration will be skipped (only cmert is run)

# Example:
# mert-moses-parallel.perl mer-work mteval02.all.src.raw mteval02.all.rfs. 100 
#   moses-parallel.pl 10 moses moses.ini "-dl 4 -b 0.1 -ttable-limit 100" 
#   "d:1,0.5-1.5 lm:1,0.5-1.5 tm:0.3,0.25-0.75;0.2,0.25-0.75;0.2,0.25-0.75;0.3,0.25-0.75;0,-0.5-0.5 w:0,-0.5-0.5"
#   
# Revision history

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

#default for initial values and ranges are:
my $default_ranges="d:0.2,0.0-1.0 lm:0.4,0.0-1.0 tm:0.1,0.0-1.0;0.1,0.0-1.0;0.1,0.0-1.0;0.1,0.0-1.0;0.0,-1.0-1.0 w:0,-1.0-1.0";

#by default no modification to the configuration file
my $default_pars="";

#path of script for filtering phrase tables
my $filtercmd="$ENV{MOSESBIN}/run-filtered-moses-parallel.perl-2006-07-27";

use strict;


my $CMERT = "$ENV{MOSESBIN}/cmert-0.5";
$ENV{PYTHONPATH} = "$CMERT/python";

my $___WORKING_DIR = $ARGV[0]; shift(@ARGV);
my $___DEV_F = $ARGV[0]; shift(@ARGV);
my $___DEV_E = $ARGV[0]; shift(@ARGV);
my $___N_BEST_LIST_SIZE = $ARGV[0]; shift(@ARGV);
my $___DECODER_PARALLEL = $ARGV[0];  shift(@ARGV);
my $___JOBS = $ARGV[0]; shift(@ARGV);
my $___DECODER = $ARGV[0];  shift(@ARGV);
my $___CONFIG = $ARGV[0];  shift(@ARGV);
my $___PARAMETERS = $ARGV[0]; shift(@ARGV);
my $___LAMBDA = $ARGV[0]; shift(@ARGV);
my $___START = $ARGV[0]; shift(@ARGV);

print STDERR "jobs:$___JOBS\n";
&full_path(\$___CONFIG);
&full_path(\$___DEV_F);
&full_path(\$___DEV_E);

 
if ($___LAMBDA eq "" ){
  $___LAMBDA=$default_ranges;
}

if ($___PARAMETERS eq "" ){
  $___PARAMETERS=$default_pars;
}
$___PARAMETERS.=" -config $___CONFIG";
$___PARAMETERS=~s/^\s+//;
$___PARAMETERS=~s/\s+$//;


# set start run, if specified
my $start_run = 1;
my $skip_decoder = 0;
if ($___START) {
  if ($___START =~ /(\d+)/) {
    $start_run = $1;
    $skip_decoder = 1;
  } 
  else {
    print "Bad start step: $___START\n";
  }
}

#store current directory and create the working directory (if needed)
my $cwd = `pwd`; chop($cwd);
safesystem("mkdir -p $___WORKING_DIR") or die;

#chdir to the working directory
chdir($___WORKING_DIR);

# Transform lambda option into decoder options
my (@LAMBDA,@MIN,@MAX,$rand,$decoder_config);
my $lambda_ranges = $___LAMBDA;
print "lambda_ranges = $lambda_ranges\n";

foreach (split(/\s+/,$lambda_ranges)) {
    my ($name,$values) = split(/:/);
    $decoder_config .= "-$name ";
    foreach my $feature (split(/;/,$values)) {
	if ($feature =~ /^(-?[\.\d]+),(-?[\.\d]+)-(-?[\.\d]+)$/) {
	    push @LAMBDA, $1;
	    push @MIN, $2;
	    push @MAX, $3;
	    $decoder_config .= "%.6f ";
	    $rand .= "$2+rand(".($3-$2)."), ";
	}
	else {
	    print "BUGGY FEATURE RANGE DEFINITION: $name => $feature\n";
	}
    } 
}
print STDERR "DECODER_CFG: $decoder_config\n";

# check if English dev set (reference translations) exist
my $DEV_E;
if (-e $___DEV_E) {
  $DEV_E=$___DEV_E;
}
else {
# if multiple file, get a full list of the files
    my $part = 0;
    while (-e "$___DEV_E$part") {
        $DEV_E .= " $___DEV_E$part";
        $part++;
    }
    die("no reference translations in $___DEV_E") unless $part;
}

#as weights are normalized in the next steps (by cmert)
#normalize initial LAMBDAs, too
my $totlambda=0;
grep($totlambda+=abs($_),@LAMBDA);
grep($_/=$totlambda,@LAMBDA);


# create some initial files (esp. weights and their ranges for randomization)

open(WEIGHTS,"> weights.txt");
print WEIGHTS join(" ", @LAMBDA)."\n";
close(WEIGHTS);

open(RANGES,"> ranges.txt");
print RANGES join(" ", @MIN)."\n";
print RANGES join(" ", @MAX)."\n";
close(RANGES);


#Parameter for effective reference length when computing BLEU score
#This is used by score-nbest-bleu.py
#Default is to use shortest reference
#Specify "-average" to use average reference length

my $EFF_REF_LEN = "";
if ($___PARAMETERS =~ /\-average /){
   $EFF_REF_LEN = "-a";
}
$___PARAMETERS =~ s/ \-average / /;
$___PARAMETERS =~ s/^\-average / /;


# filtering the phrase tables
print "filtering the phrase tables... ".`date`;
$___PARAMETERS =~ s/ \-f / -config /;
$___PARAMETERS =~ s/^\-f /-config /;
my $PARAMETERS;
if ($___PARAMETERS =~ /-config (\S+)/) {
    my $config = $1;
    my $filtered_parameters = $___PARAMETERS;
    $filtered_parameters =~ s/-config *(\S+)//;
    my $cmd = "$filtercmd ./filtered $___DECODER_PARALLEL $___JOBS $___DECODER $config $___DEV_F \"$filtered_parameters -norun\"";
    print $cmd."\n"; 
    safesystem("$cmd") or die;

    $PARAMETERS = $___PARAMETERS;
    $PARAMETERS =~ s/-config (\S+)/-config filtered\/moses.ini/;
    if ($PARAMETERS =~ /^(.*)-distortion-file +(\S.*) (-.+)$/ || 
	$PARAMETERS =~ /^(.*)-distortion-file +(\S.*)()$/) {
	my ($pre,$files,$post) = ($1,$2,$3);
	$PARAMETERS = "$pre -distortion-file ";
	foreach my $distortion (split(/ +/,$files)) {
	    my $out = $distortion;
	    $out =~ s/^.*\/+([^\/]+)/$1/g;
	    $PARAMETERS .= "filtered/$out";
	}
	$PARAMETERS .= $post;
    }
}

my $devbleu;
my $run=$start_run-1;
my $prev_size = -1;
while(1) {
  $run++;
  # run beamdecoder with option to output lattices
  # the end result should be (1) @NBEST_LIST, a list of lists; (2) @SCORE, a list of lists of lists

  print "run $run start at ".`date`;

  # get most recent set of weights
  open(WEIGHTS,"< weights.txt");
  while(<WEIGHTS>) {
      chop;
      @LAMBDA = split " ";
  }
  close(WEIGHTS);


  # skip if restarted
  if (!$skip_decoder) {
      print "($run) run decoder to produce n-best lists\n";
      print "LAMBDAS are @LAMBDA\n";
      &run_decoder(\@LAMBDA);
  }
  else {
      print "skipped decoder run\n";
      $skip_decoder = 0;
  }
  safesystem("gzip run*out") or die;


  # convert n-best list into a numberized format with error scores
  safesystem("gunzip run*.best*.out.gz") or die;
  my $cmd = "sort -mn -t \"|\" -k 1,1 run*.best*.out | $CMERT/score-nbest.py $EFF_REF_LEN $DEV_E ./";
  print $cmd."\n";
  safesystem("$cmd") or die;
  safesystem("gzip run*.best*.out") or die;


  # keep a count of lines in nbests lists (alltogether)
  # if it did not increase since last iteration, we are DONE
  open(IN,"cat cands.opt | awk '{n += \$2} END {print n}'|");
  my $size;
  chomp($size=<IN>);	
  close(IN);
  print "$size accumulated translations\n";
  print "prev accumulated translations was : $prev_size\n";
  if ($size <= $prev_size){
     print "Training finished at ".`date`;
     last;
  }
  $prev_size = $size;


  # run cmert
  safesystem("cat ranges.txt weights.txt > init.opt") or die;
  safesystem("rm -f weights.txt") or die;

#store actual values
#  system ("cp cands.opt run$run.cands.opt");
#  system ("cp feats.opt run$run.feats.opt");
  safesystem("cp init.opt run$run.init.opt") or die;

  my $DIM = scalar(@LAMBDA);

  safesystem("$CMERT/mert -d $DIM 2> cmert.log") or die;

  open(IN,"cat cmert.log | grep 'Best point:'|");
  chomp($_=<IN>);
  /(Best point: [\s\d\.\-]+ => )([\d\.]+)/;
  print "($run) BEST at $run: $1$2 at ".`date`;
  close (IN);
  
  safesystem ("cp cmert.log run$run.cmert.log") or die;

  $devbleu = $2;

  print "run $run end at ".`date`;

  if (! -s "weights.txt"){
      print "Optimization failed\n";
      last;
  }
}
safesystem("cp init.opt run$run.init.opt") or die;
safesystem ("cp cmert.log run$run.cmert.log") or die;

&create_config();

#chdir back to the original directory
chdir($cwd);

sub run_decoder {
    my ($LAMBDA) = @_;
    my $filename_template = "run%d.best$___N_BEST_LIST_SIZE.out";
    my $filename = sprintf($filename_template, $run);
    
    print "params = $PARAMETERS\n";
    print "decoder_config = " .sprintf($decoder_config,@{$LAMBDA}) ."\n";

    # run the decoder
    my $decoder_cmd = "$___DECODER_PARALLEL $PARAMETERS ".sprintf($decoder_config,@{$LAMBDA})." -n-best-file $filename -n-best-size $___N_BEST_LIST_SIZE -i $___DEV_F -jobs $___JOBS -decoder $___DECODER > run$run.out";
    print "$decoder_cmd\n";
    
    safesystem("$decoder_cmd") or die;
}

sub create_config {
    my %ABBR;
    $ABBR{'dl'} = 'distortion-limit';
    $ABBR{'d'}	= 'weight-d';
    $ABBR{'lm'}	= 'weight-l';
    $ABBR{'tm'}	= 'weight-t';
    $ABBR{'w'}	= 'weight-w';
    $ABBR{'g'}	= 'weight-generation';

    my %P;
    # parameters specified at the command line
    {
	my $parameter;
	print "PARAM IS |$___PARAMETERS|\n";
	foreach (split(/ /,$___PARAMETERS)) {
	    print "$_ :::\n";
	    if (/^\-([^\d].*)$/) {
		$parameter = $1;
		$parameter = $ABBR{$parameter} if defined($ABBR{$parameter});
		print "\tis parameter $parameter\n";
	    }
	    else {
		push @{$P{$parameter}},$_;
	    }
	}
    }

    # tuned parameters
    if ($___LAMBDA =~ /lm/) {
	my $l=0;
	foreach (split(/ /,$lambda_ranges)) {
	    my ($name,$values) = split(/:/);
	    $name = $ABBR{$name} if defined($ABBR{$name});
#	    print "NAME:$name l is $P{$name}\n";
	    foreach my $feature (split(/;/,$values)) {
		push @{$P{$name}},$LAMBDA[$l++];
	    }
	}
    }
    else {
	for (my $i=0; $i<=6; $i++) {
	    push @{$P{"weight-d"}},$LAMBDA[$i];
	}
	push @{$P{"weight-l"}},$LAMBDA[7];
	for (my $i=8; $i<=$#LAMBDA-1; $i++) {
	    push @{$P{"weight-t"}},$LAMBDA[$i];
	}
	push @{$P{"weight-w"}},$LAMBDA[$#LAMBDA]; 
    }

    # create new moses.ini decoder config file
    open(INI,$P{"config"}[0]);
    delete($P{"config"});
    print "OUT: > moses.ini\n";
    open(OUT,"> moses.ini");
    print OUT "# MERT optimized configuration\n";
    print OUT "# decoder $___DECODER\n";
    print OUT "# $devbleu on dev $___DEV_F\n";
    print OUT "# $run iterations\n";
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
	$parameter = $ABBR{$parameter} if defined($ABBR{$parameter});
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

    foreach my $parameter (keys %P) {
	print OUT "\n[$parameter]\n";
	foreach (@{$P{$parameter}}) {
	    print OUT $_."\n";
	}
    }

    close(INI);
    close(OUT);
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
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
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

