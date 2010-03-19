#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

my $SRILM = "/home/pkoehn/moses/srilm/bin/i686-m64";
my ($TUNING,$LM,$NAME);

die("interpolate-lm.perl --tuning set --name out-lm --lm lm1,lm2,lm3 [--srilm srtilm-dir]")
    unless &GetOptions('tuning=s' => => \$TUNING,
		       'name=s' => \$NAME,
		       'srilm=s' => \$SRILM,
		       'lm=s' => \$LM);

# check and set default to unset parameters
die("ERROR: please specify output language model name --name") unless defined($NAME);
die("ERROR: please specify tuning set with --tuning") unless defined($TUNING); 
die("ERROR: please specify language models with --lm") unless defined($LM); 
die("ERROR: did not find srilm dir") unless -e $SRILM;

my @LM = split(/,/,$LM);

# establish order
my $order = 0;
foreach my $lm (@LM) {
  my $lm_order;
  $lm .= ".gz" if (! -e $lm && -e "$lm.gz");
  if ($lm =~ /gz$/) {
    open(LM,"zcat $lm|") || die("could not find language model file '$lm'");
  }
  else {
    open(LM,$lm) || die("could not find language model file '$lm'");
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

# compute perplexity
my $i = 0;
foreach my $lm (@LM) {
  print STDERR "compute perplexity for $lm\n";
  `$SRILM/ngram -unk -order $order -lm $lm -ppl $TUNING -debug 2 > /tmp/iplm.$$.$i`;
  print STDERR `tail -n 2 /tmp/iplm.$$.$i`;
  $i++;
}

# compute lambdas
print STDERR "compute lambdas...\n";
my $cmd = "$SRILM/compute-best-mix";
for(my $i=0;$i<scalar(@LM);$i++) {
  $cmd .= " /tmp/iplm.$$.$i";
}
print STDERR $cmd."\n";
my $mix = `$cmd`;
`rm /tmp/iplm.$$.*`;
$mix =~ /best lambda \(([\d\. ]+)\)/ || die("computing lambdas failed: $mix");
my @LAMBDA = split(/ /,$1);

# create new language models
print STDERR "create new language model...";
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
print STDERR $cmd."\n";
`$cmd`;

print STDERR "done.\n";
