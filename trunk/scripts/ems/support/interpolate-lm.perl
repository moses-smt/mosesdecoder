#!/usr/bin/perl -w

use strict;
use IPC::Open3;
use File::Temp qw/tempdir/;
use File::Path qw/rmtree/;
use Getopt::Long "GetOptions";

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

my $SRILM = "/home/pkoehn/moses/srilm/bin/i686-m64";
my $TEMPDIR = "/tmp";
my ($TUNING,$LM,$NAME);

die("interpolate-lm.perl --tuning set --name out-lm --lm lm1,lm2,lm3 [--srilm srtilm-dir --tempdir tempdir]")
    unless &GetOptions('tuning=s' => => \$TUNING,
		       'name=s' => \$NAME,
		       'srilm=s' => \$SRILM,
		       'tempdir=s' => \$TEMPDIR,
		       'lm=s' => \$LM);

# check and set default to unset parameters
die("ERROR: please specify output language model name --name") unless defined($NAME);
die("ERROR: please specify tuning set with --tuning") unless defined($TUNING); 
die("ERROR: please specify language models with --lm") unless defined($LM); 
die("ERROR: can't read $TUNING") unless -e $TUNING;
die("ERROR: did not find srilm dir") unless -e $SRILM;
die("ERROR: cannot run ngram") unless -x $SRILM."/ngram";

my @LM = split(/,/,$LM);

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
    $lm_order = $1 if /ngram (\d+)/;
    last if /1-grams/;
  }
  close(LM);
  $order = $lm_order if $order == 0;
  die("ERROR: language models have different order") if $order != $lm_order;
}
print STDERR "language models have order $order.\n";

my $tmp = tempdir(DIR=>$TEMPDIR);

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
$mix =~ /best lambda \(([\d\. ]+)\)/ || die("ERROR: computing lambdas failed: $mix");
my @LAMBDA = split(/ /,$1);

# create new language models
print STDERR "creating new language model...\n";
$i = 0;
$cmd = "$SRILM/ngram -unk -order $order -write-lm $NAME";
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
  my($wtr, $rdr, $err);
  my $pid = open3($wtr, $rdr, $err, @_);
  close($wtr);
  waitpid($pid, 0);
  my $gotout = "";
  $gotout .= $_ while (<$rdr>);
  close $rdr;
  my $goterr = "";
  if (defined $err) {
    $goterr .= $_ while (<$err>);
    close $err;
  }
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
