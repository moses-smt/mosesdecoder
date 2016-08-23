#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use IPC::Open3;
use File::Temp qw/tempdir/;
use File::Path qw/rmtree/;
use Getopt::Long "GetOptions";
use Symbol;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

my $SRILM = "/home/pkoehn/moses/srilm/bin/i686-m64";
my $TEMPDIR = "/tmp";
my ($TUNING,$LM,$NAME,$GROUP,$WEIGHTS,$CONTINUE);

die("interpolate-lm.perl --tuning set --name out-lm --lm lm0,lm1,lm2,lm3 [--srilm srilm-dir --tempdir tempdir --group \"0,1 2,3\"]")
    unless &GetOptions('tuning=s' => => \$TUNING,
		       'name=s' => \$NAME,
		       'srilm=s' => \$SRILM,
		       'tempdir=s' => \$TEMPDIR,
           'continue' => \$CONTINUE,
           'group=s' => \$GROUP,
           'weights=s' => \$WEIGHTS,
		       'lm=s' => \$LM);

# check and set default to unset parameters
die("ERROR: please specify output language model name --name") unless defined($NAME);
die("ERROR: please specify tuning set with --tuning") unless defined($TUNING);
die("ERROR: please specify language models with --lm") unless defined($LM);
die("ERROR: can't read $TUNING") unless -e $TUNING;
die("ERROR: did not find srilm dir") unless -e $SRILM;
die("ERROR: cannot run ngram") unless -x $SRILM."/ngram";

my @LM = split(/,/,$LM);
my @WEIGHT;
@WEIGHT = split(/,/,$WEIGHTS) if defined($WEIGHTS);
die("ERROR: different number of weights and language models: ".scalar(@WEIGHT)." vs. ".scalar(@LM))
  if defined($WEIGHTS) && scalar(@WEIGHT) != scalar(@LM);

# establish order
my $order = 0;
foreach my $lm (@LM) {
  my $lm_order;
  $lm .= ".gz" if (! -e $lm && -e "$lm.gz");
  if ($lm =~ /gz$/) {
    open(LM,"zcat $lm|") || die("ERROR: could not find language model file '$lm'");
  }
  else {
    open(LM,$lm) || die("ERROR: could not find language model file '$lm'");
  }
  while(<LM>) {
    $lm_order = $1 if /ngram\s+(\d+)/;
    last if /1-grams/;
  }
  close(LM);
  $order = $lm_order if $order == 0;
  die("ERROR: language models have different order") if $order != $lm_order;
}
print STDERR "language models have order $order.\n";

# too many language models? group them first
if (!defined($GROUP) && scalar(@LM) > 10) {
  print STDERR "more than 10, automatically grouping language models.\n";
  my $num_groups = int(scalar(@LM)/10 + 0.99);
  my $size_groups = int(scalar(@LM)/$num_groups + 0.99);

  $GROUP = "";
  for(my $i=0;$i<$num_groups;$i++) {
    $GROUP .= " " unless $i==0;
    for(my $j=0;$j<$size_groups;$j++) {
      my $lm_i = $i*$size_groups+$j;
      next if $lm_i >= scalar(@LM);
      $GROUP .= "," unless $j==0;
      $GROUP .= $lm_i;
    }
  }
  print STDERR "groups: $GROUP\n";
}

# normal interpolation
if (!defined($GROUP)) {
  &interpolate($NAME,\@WEIGHT,@LM);
  exit;
}

# group language models into sub-interpolated models
my %ALREADY;
my $g = 0;
my @SUB_NAME;
foreach my $subgroup (split(/ /,$GROUP)) {
  my @SUB_LM;
  foreach my $lm_i (split(/,/,$subgroup)) {
    die("ERROR: LM id $lm_i in group definition out of range") if $lm_i >= scalar(@LM);
    push @SUB_LM,$LM[$lm_i];
    $ALREADY{$lm_i} = 1;
  }
  #if (scalar @SUB_NAME == 0 && scalar keys %ALREADY == scalar @LM) {
  #  print STDERR "WARNING: grouped all language models into one, perform normal interpolation\n";
  #  &interpolate($NAME,@LM);
  #  exit;
  #}
  my $name = $NAME.".group-".chr(97+($g++));
  push @SUB_NAME,$name;
  print STDERR "\n=== BUILDING SUB LM $name from\n\t".join("\n\t",@SUB_LM)."\n===\n\n";
  &interpolate($name, undef, @SUB_LM) unless $CONTINUE && -e $name;
}
for(my $lm_i=0; $lm_i < scalar(@LM); $lm_i++) {
  next if defined($ALREADY{$lm_i});
  push @SUB_NAME, $LM[$lm_i];
}
print STDERR "\n=== BUILDING FINAL LM ===\n\n";
&interpolate($NAME, undef, @SUB_NAME);

# main interpolation function
sub interpolate {
  my ($name,$WEIGHT,@LM) = @_;

  die("cannot interpolate more than 10 language models at once: ",join(",",@LM))
    if scalar(@LM) > 10;

  my $tmp = tempdir(DIR=>$TEMPDIR);
  my @LAMBDA;

  # if weights are specified, use them
  if (defined($WEIGHT) && scalar(@$WEIGHT) == scalar(@LM)) {
    @LAMBDA = @$WEIGHT;
  }
  # no specified weights -> compute them
  else {
    # compute perplexity
    my $i = 0;
    foreach my $lm (@LM) {
      print STDERR "compute perplexity for $lm\n";
      safesystem("$SRILM/ngram -unk -order $order -lm $lm -ppl $TUNING -debug 2 > $tmp/iplm.$$.$i") or die "Failed to compute perplexity for $lm\n";
      print STDERR `tail -n 2 $tmp/iplm.$$.$i`;
      $i++;
    }

    # compute lambdas
    print STDERR "computing lambdas...\n";
    my $cmd = "$SRILM/compute-best-mix";
    for(my $i=0;$i<scalar(@LM);$i++) {
      $cmd .= " $tmp/iplm.$$.$i";
    }
    my ($mixout, $mixerr, $mixexitcode) = saferun3($cmd);
    die "Failed to mix models: $mixerr" if $mixexitcode != 0;
    my $mix = $mixout;
    `rm $tmp/iplm.$$.*`;
    $mix =~ /best lambda \(([\d\. e-]+)\)/ || die("ERROR: computing lambdas failed: $mix");
    @LAMBDA = split(/ /,$1);
  }

  # create new language model
  print STDERR "creating new language model...\n";
  my $i = 0;
  my $cmd = "$SRILM/ngram -unk -order $order -write-lm $name";
  foreach my $lm (@LM) {
    $cmd .= " -lm " if $i==0;
    $cmd .= " -mix-lm " if $i==1;
    $cmd .= " -mix-lm$i " if $i>1;
    $cmd .= $lm;
    $cmd .= " -lambda " if $i==0;
    $cmd .= " -mix-lambda$i " if $i>1;
    $cmd .= $LAMBDA[$i] if $i!=1;
    $i++;
  }
  safesystem($cmd) or die "Failed.";

  rmtree($tmp); # remove the temp dir
  print STDERR "done.\n";
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

sub saferun3 {
  print STDERR "Executing: @_\n";
  my $wtr = gensym();
  my $rdr = gensym();
  my $err = gensym();
  my $pid = open3($wtr, $rdr, $err, @_);
  close($wtr);
  my $gotout = "";
  $gotout .= $_ while (<$rdr>);
  close $rdr;
  my $goterr = "";
  if (defined $err) {
    $goterr .= $_ while (<$err>);
    close $err;
  }
  waitpid($pid, 0);
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
    return ( $gotout, $goterr, $exitcode );
  }
}
