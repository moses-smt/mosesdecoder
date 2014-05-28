#!/usr/bin/perl -w

# $Id$
# after filter-mode-given-input.pl, process the feature list

# original code by Philipp Koehn
# changes by Ondrej Bojar
# adapted for hierarchical models by Phil Williams

use strict;

use FindBin qw($Bin);
use Getopt::Long;



my $SCRIPTS_ROOTDIR;
if (defined($ENV{"SCRIPTS_ROOTDIR"})) {
    $SCRIPTS_ROOTDIR = $ENV{"SCRIPTS_ROOTDIR"};
} else {
    $SCRIPTS_ROOTDIR = $Bin;
    if ($SCRIPTS_ROOTDIR eq '') {
        $SCRIPTS_ROOTDIR = dirname(__FILE__);
    }
    $SCRIPTS_ROOTDIR =~ s/\/training$//;
    $ENV{"SCRIPTS_ROOTDIR"} = $SCRIPTS_ROOTDIR;
}


# moses.ini file uses FULL names for lambdas, while this training script
# internally (and on the command line) uses ABBR names.
my @ABBR_FULL_MAP = qw(d=weight-d lm=weight-l tm=weight-t w=weight-w
  g=weight-generation lex=weight-lex I=weight-i);
my %ABBR2FULL = map {split/=/,$_,2} @ABBR_FULL_MAP;
my %FULL2ABBR = map {my ($a, $b) = split/=/,$_,2; ($b, $a);} @ABBR_FULL_MAP;



my $verbose = 0;
my $usage = 0; # request for --help




##!# # consider phrases in input up to $MAX_LENGTH
##!# # in other words, all phrase-tables will be truncated at least to 10 words per
##!# # phrase.
##!# my $MAX_LENGTH = 10;

# utilities
##!# my $ZCAT = "gzip -cd";

# get optional parameters
##!# my $opt_hierarchical = 0;
##!# my $binarizer = undef;
##!# my $opt_min_non_initial_rule_count = undef;
##!# my $opt_gzip = 1; # gzip output files (so far only phrase-based ttable until someone tests remaining models and formats)

my $___RANGES = undef;
my $___ACTIVATE_FEATURES = undef; # comma-separated (or blank-separated) list of features to work on 
                                  # if undef work on all features
                                  # (others are fixed to the starting values)
my $___DECODER_FLAGS = "";        # additional parametrs to pass to the decoder                                

my $devbleu = undef;
my $___WORKING_DIR = undef;
my $___DEV_F = undef;
my $run = undef;                  # either first or final
my $runid_final = undef;
my $runid_finalplus=0;
my $sparse_weights_file = undef;


# set 0 if input type is text, set 1 if input type is confusion network
my $___INPUTTYPE = 0;

my $___DECODER = undef;           # required, pathname to the decoder executable
my $___CONFIG = undef;            # required, pathname to startup ini file


GetOptions(
  "activate-features=s" => \$___ACTIVATE_FEATURES, #comma-separated (or blank-separated) list of features to work on (others are fixed to the starting values)
  "range=s@" => \$___RANGES,
  "decoder-flags=s" => \$___DECODER_FLAGS,
  "inputtype=i" => \$___INPUTTYPE,
  "devbleu=s" => \$devbleu,
  "sparse_weight_file=s" => \$sparse_weights_file,
  "working-dir=s" => \$___WORKING_DIR,
) or exit(1);

##!# GetOptions(
##!#    "gzip!" => \$opt_gzip,
##!#    "Hierarchical" => \$opt_hierarchical,
##!#    "Binarizer=s" => \$binarizer,
##!#    "MinNonInitialRuleCount=i" => \$opt_min_non_initial_rule_count
##!# ) or exit(1);


# the ?? required parameters can be supplied on the command line directly
# or using the --options
if (scalar @ARGV == 4) {
  # required parameters: options
  $___DEV_F = shift;
  $___DECODER = shift;
  $___CONFIG = shift;
  $run = shift;         # first or final
}

if ($usage || !defined $___DECODER || !defined $___CONFIG) {
  print STDERR "usage: $0 \$___DECODER \$___CONFIG(decoder.ini)
Options:
  --activate-features=STRING  ... comma-separated list of features to optimize,
                                  others are fixed to the starting values
                                  default: optimize all features
                                  example: tm_0,tm_4,d_0
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
  --decoder-flags=STRING ... extra parameters for the decoder
  --inputtype=[0|1|2]    ... Handle different input types: (0 for text,
                             1 for confusion network, 2 for lattices,
                             default is 0)
";
  exit 1;
}



##!# # get command line parameters
##!# my $dir = shift;
##!# my $config = shift;
##!# my $input = shift;

##!# $dir = ensure_full_path($dir);

############################################################
############################################################
############################################################

# main

# we run moses to check validity of moses.ini and to obtain all the feature
# names

if (($run eq "first")){
   my $featlist = get_featlist_from_moses($___CONFIG,$___CONFIG,"first");
   $featlist = insert_ranges_to_featlist($featlist, $___RANGES);
   create_config($___CONFIG,"$___WORKING_DIR/run1.moses.ini",$featlist,1,(defined$devbleu?$devbleu:"--not-estimated--"),$sparse_weights_file);
} else {        # $run eq "final"
   chomp ($runid_final = `cat $___WORKING_DIR/finished_step.txt | tail -n 1`);
   $runid_finalplus = $runid_final + 1; 
   `mv run${runid_finalplus}.moses.ini run_final.moses.ini`;
   chomp ($devbleu = `cat $___WORKING_DIR/run_final.moses.ini | tail -n +3 | head -n 1 | gawk '{print \$3}'`);
   my $featlist = get_featlist_from_moses($___CONFIG,"$___WORKING_DIR/run_final.moses.ini","final");
   $featlist = insert_ranges_to_featlist($featlist, $___RANGES);
   create_config($___CONFIG,"$___WORKING_DIR/moses.ini",$featlist,$runid_finalplus,$devbleu,$sparse_weights_file);
}

##COPIED## Mark which features are disabled:
##COPIED#if (defined $___ACTIVATE_FEATURES) {
##COPIED#  my %enabled = map { ($_, 1) } split /[, ]+/, $___ACTIVATE_FEATURES;
##COPIED#  my %cnt;
##COPIED#  for(my $i=0; $i<scalar(@{$featlist->{"names"}}); $i++) {
##COPIED#    my $name = $featlist->{"names"}->[$i];
##COPIED#    $cnt{$name} = 0 if !defined $cnt{$name};
##COPIED#    $featlist->{"enabled"}->[$i] = $enabled{$name."_".$cnt{$name}};
##COPIED#    $cnt{$name}++;
##COPIED#  }
##COPIED#} else {
##COPIED#  # all enabled
##COPIED#  for(my $i=0; $i<scalar(@{$featlist->{"names"}}); $i++) {
##COPIED#    $featlist->{"enabled"}->[$i] = 1;
##COPIED#  }
##COPIED#}
##COPIED#
##COPIED#print STDERR "MERT starting values and ranges for random generation:\n";
##COPIED#for(my $i=0; $i<scalar(@{$featlist->{"names"}}); $i++) {
##COPIED#  my $name = $featlist->{"names"}->[$i];
##COPIED#  my $val = $featlist->{"values"}->[$i];
##COPIED#  my $min = $featlist->{"mins"}->[$i];
##COPIED#  my $max = $featlist->{"maxs"}->[$i];
##COPIED#  my $enabled = $featlist->{"enabled"}->[$i];
##COPIED#  printf STDERR "  %5s = %7.3f", $name, $val;
##COPIED#  if ($enabled) {
##COPIED#    printf STDERR " (%5.2f .. %5.2f)\n", $min, $max;
##COPIED#  } else {
##COPIED#    print STDERR " --- inactive, not optimized ---\n";
##COPIED#  }
##COPIED#}





sub get_featlist_from_moses {
  # run moses with the given config file and return the list of features and
  # their initial values
  my $configfn = shift;
  my $config_score = shift;
  my $run = shift;

  my $featlistfn = "";
  if ($run eq 'first') {
    $featlistfn = "./features.list";   # given feature list
  } elsif ($run eq "final") {
    $featlistfn = "./features.list.run_final"; 
  }
  if (-e $featlistfn) {
    print STDERR "Using cached features list: $featlistfn\n";
  } else {
    print STDERR "Asking moses for feature names and values from $config_score\n";
    my $cmd = "$___DECODER $___DECODER_FLAGS -config $config_score -inputtype $___INPUTTYPE -show-weights > $featlistfn";
    print STDERR "$cmd\n";   #DEBUG
    safesystem($cmd) or die "Failed to run moses with the config $config_score";
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
    /^(.+) (\S+) (\S+)$/ || die("invalid feature: $_");
    my ($longname, $feature, $value) = ($1,$2,$3);
    next if $value eq "sparse";
    push @errs, "$featlistfn:$nr:Bad initial value of $feature: $value\n"
      if $value !~ /^[+-]?[0-9.e]+$/;
    push @errs, "$featlistfn:$nr:Unknown feature '$feature', please add it to \@ABBR_FULL_MAP\n"
      if !defined $ABBR2FULL{$feature};
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
        die "Unrecognized name '$name' in --range=$range"
          if !defined $ABBR2FULL{$name};
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
  for(my $i=0; $i<scalar(@{$featlist->{"names"}}); $i++) {
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



sub create_config {
    my $infn = shift; # source config
    my $outfn = shift; # where to save the config
    my $featlist = shift; # the lambdas we should write
    my $iteration = shift;  # just for verbosity
    my $bleu_achieved = shift; # just for verbosity
    my $sparse_weights_file = shift; # only defined when optimizing sparse features

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

    # First delete all weights params from the input, we're overwriting them.
    # Delete both short and long-named version.
    for(my $i=0; $i<scalar(@{$featlist->{"names"}}); $i++) {
      my $name = $featlist->{"names"}->[$i];
      delete($P{$name});
      delete($P{$ABBR2FULL{$name}});
    }

    # Convert weights to elements in P
    for(my $i=0; $i<scalar(@{$featlist->{"names"}}); $i++) {
      my $name = $featlist->{"names"}->[$i];
      my $val = $featlist->{"values"}->[$i];
      $name = defined $ABBR2FULL{$name} ? $ABBR2FULL{$name} : $name;
        # ensure long name
      push @{$P{$name}}, $val;
    }

    if (defined($sparse_weights_file)) {
      push @{$P{"weights-file"}}, $___WORKING_DIR."/".$sparse_weights_file;
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


