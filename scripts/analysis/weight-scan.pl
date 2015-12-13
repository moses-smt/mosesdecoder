#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# runs Moses many times changing the values of one weight, all others fixed
# nbest lists are always produced to allow for comparison of real and
# 'projected' BLEU (BLEU estimated from n-best lists collected at a neighouring
# node)
# usage: weight-scan.pl <input> <moses> <moses.ini> tm_2 --range=0.0,0.1,1.0

use strict;
use warnings;
use Getopt::Long;
use FindBin qw($RealBin);
use File::Basename;
use File::Path;
my $SCRIPTS_ROOTDIR = $RealBin;
$SCRIPTS_ROOTDIR =~ s/\/training$//;
$SCRIPTS_ROOTDIR = $ENV{"SCRIPTS_ROOTDIR"} if defined($ENV{"SCRIPTS_ROOTDIR"});

my $prec = 3; # precision of weightvalue within filename
my $jobs = 0;
my $workdir = "weight-scan";
my $range = "0.0,0.1,1.0";
my $input_type = 0;
my $normalize = 0; # normalize
my $nbestsize = 100;
my $decoderflags = "";
my $moses_parallel_cmd = "$SCRIPTS_ROOTDIR/generic/moses-parallel.pl";
my $qsubwrapper="$SCRIPTS_ROOTDIR/generic/qsub-wrapper.pl";
my $queue_flags = "-hard";  # extra parameters for parallelizer
GetOptions(
  "jobs=i" => \$jobs,
  "range=s" => \$range,
  "working-dir=s" => \$workdir,
  "normalize!" => \$normalize,
  "nbest=i" => \$nbestsize,
  "decoderflags=s" => \$decoderflags,
) or exit 1;

my $inf = shift;
my $decoder = shift;
my $config = shift;
my $weightspec = shift;

if (!defined $inf || ! defined $decoder || !defined $config || !defined $weightspec) {
  print STDERR "usage: $0 <input> <moses> <moses.ini> tm_2 --range=0.0,0.1,1.0
Options:
  --working-dir=weight-scan
  --jobs=0
  --range=0.0,0.1,1.0
";
  exit 1;
}

print STDERR "Using SCRIPTS_ROOTDIR: $SCRIPTS_ROOTDIR\n";

die "Not executable: $moses_parallel_cmd" if defined $jobs && ! -x $moses_parallel_cmd;
die "Not executable: $qsubwrapper" if defined $jobs && ! -x $qsubwrapper;
die "Not executable: $decoder" if ! -x $decoder;

my $inf_abs = ensure_full_path($inf);
die "File not found: $inf (interpreted as $inf_abs)."
  if ! -e $inf_abs;
$inf = $inf_abs;

my $decoder_abs = ensure_full_path($decoder);
die "File not executable: $decoder (interpreted as $decoder_abs)."
  if ! -x $decoder_abs;
$decoder = $decoder_abs;

my $config_abs = ensure_full_path($config);
die "File not found: $config (interpreted as $config_abs)."
  if ! -e $config_abs;
$config = $config_abs;


my ($startvalue, $step, $stopvalue) = split /,/, $range;
die "Bad range: $range; expected start,step,stop"
  if !defined $startvalue || !defined $step || !defined $stopvalue;


my $featlist = get_featlist_from_moses($config);

# $weightidx is within features of the name $weightname
# $weightindex is global
my ($weightname, $weightidx) = split /_/, $weightspec;
my $weightindex;

# scan the weights, find the one we'll test and remember values of all of the
# given name
my $only_one_expected = 0;
if (!defined $weightidx) {
  $only_one_expected = 1;
  $weightidx = 0;
}
my @weightvalues = ();
my $idx = 0;
for(my $i=0; $i<scalar(@{$featlist->{"names"}}); $i++) {
  my $name = $featlist->{"names"}->[$i];
  if ($name eq $weightname) {
    push @weightvalues, $featlist->{"values"}->[$i];
    $weightindex = $i if $idx == $weightidx; # remember the global index of the weight
    $idx++;
  }
}

die "You specified only '$weightspec' but there are $idx features of the given name.\nUse e.g.: ${weightspec}_0\n"
  if $only_one_expected && $idx > 1;
die "Failed to find weights of the name '$weightname' in moses config."
  if !defined $weightindex;



#store current directory and create the working directory (if needed)
my $cwd = `pawd 2>/dev/null`;
if(!$cwd){$cwd = `pwd`;}
chomp($cwd);

mkpath($workdir);
{
# open local scope

#chdir to the working directory
chdir($workdir) or die "Can't chdir to $workdir";

## MAIN LOOP
for(my $weightvalue = $startvalue; $weightvalue <= $stopvalue; $weightvalue+=$step) {
  my $nbestout = run_decoder($featlist, $weightvalue);
}


#chdir back to the original directory # useless, just to remind we were not there
chdir($cwd);
} # end of local scope

sub run_decoder {
    my ($featlist, $weightvalue) = @_;
    my $filebase = sprintf("%${prec}f", $weightvalue);
    my $nbestfilename = "best$nbestsize.$filebase";
    my $filename = "out.$filebase";

    # user-supplied parameters
    print STDERR "params = $decoderflags\n";

    # parameters to set all model weights (to override moses.ini)
    my @vals = @{$featlist->{"values"}};
    $vals[$weightindex] = $weightvalue; # set the one we're scanning
    if ($normalize) {
      print STDERR "Normalizing lambdas: @vals\n";
      my $totlambda=0;
      grep($totlambda+=abs($_),@vals);
      grep($_/=$totlambda,@vals);
    }
    # moses now does not seem accept "-tm X -tm Y" but needs "-tm X Y"
    my %model_weights;
    for(my $i=0; $i<scalar(@{$featlist->{"names"}}); $i++) {
      my $name = $featlist->{"names"}->[$i];
      $model_weights{$name} = "-$name" if !defined $model_weights{$name};
      $model_weights{$name} .= sprintf " %.6f", $vals[$i];
    }
    my $decoder_config = join(" ", values %model_weights);
    print STDERR "DECODER_CFG = $decoder_config\n";

    # write the weights for future use
    open OUTF, ">weights.$filebase" or die "Can't write weights";
    print OUTF join(" ", map { sprintf("%.6f", $_) } @vals)."\n";
    close OUTF;

    # run the decoder
    my $nBest_cmd = "-n-best-size $nbestsize";
    my $decoder_cmd;

    if ($jobs) {
      $decoder_cmd = "$moses_parallel_cmd -config $config -inputtype $input_type -qsub-prefix scan$weightvalue -queue-parameters \"$queue_flags\" -decoder-parameters \"$decoderflags $decoder_config\" -n-best-list \"$nbestfilename $nbestsize\" -input-file $inf -jobs $jobs -decoder $decoder > $filename";
    } else {
      $decoder_cmd = "$decoder $decoderflags  -config $config -inputtype $input_type $decoder_config -n-best-list $nbestfilename $nbestsize -input-file $inf > $filename";
    }

    safesystem($decoder_cmd) or die "The decoder died. CONFIG WAS $decoder_config \n";

    return $nbestfilename;
}

sub get_featlist_from_moses {
  # run moses with the given config file and return the list of features and
  # their initial values
  my $configfn = shift;
  my $featlistfn = "./features.list";
  if (-e $featlistfn) {
    print STDERR "Using cached features list: $featlistfn\n";
  } else {
    print STDERR "Asking moses for feature names and values from $configfn\n";
    my $cmd = "$decoder $decoderflags -config $configfn  -inputtype $input_type -show-weights > $featlistfn";
    safesystem($cmd) or die "Failed to run moses with the config $configfn";
  }

  # read feature list
  my @names = ();
  my @startvalues = ();
  open(INI,$featlistfn) or die "Can't read $featlistfn";
  my $nr = 0;
  my @errs = ();
  while (<INI>) {
    $nr++;
    chomp;
    my ($longname, $feature, $value) = split / /;
    push @errs, "$featlistfn:$nr:Bad initial value of $feature: $value\n"
      if $value !~ /^[+-]?[0-9.e]+$/;
    #push @errs, "$featlistfn:$nr:Unknown feature '$feature', please add it to \@ABBR_FULL_MAP\n"
    #  if !defined $ABBR2FULL{$feature};
    push @names, $feature;
    push @startvalues, $value;
  }
  close INI;
  if (scalar @errs) {
    print STDERR join("", @errs);
    exit 1;
  }
  return {"names"=>\@names, "values"=>\@startvalues};
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
