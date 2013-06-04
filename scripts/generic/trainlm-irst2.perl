#!/usr/bin/perl -w

# Compatible with sri LM-creating script, eg.
#    ngram-count -order 5 -interpolate -wbdiscount -unk -text corpus.txt -lm lm.txt
# To use it in the EMS, add this to the [LM] section
#    lm-training = "$moses-script-dir/generic/trainlm-irst2.perl -cores $cores -irst-dir $irst-dir"
#    settings = ""
# Also, make sure that $irst-dir is defined (in the [LM] or [GENERAL] section. 
# It should point to the root of the LM toolkit, eg
#    irst-dir = /Users/hieu/workspace/irstlm/trunk/bin
# Set smoothing method in settings, if different from modified Kneser-Ney 

use strict;
use FindBin qw($RealBin);
use Getopt::Long;

my $order = 3; # order of language model (default trigram)
my $corpusPath; # input text data
my $lmPath; # generated language model
my $cores = 2; # number of CPUs used
my $irstPath; # bin directory of IRSTLM 
my $tempPath = "tmp"; # temp dir
my $pruneSingletons = 1; # 1 = prune singletons, 0 = keep singletons
my $smoothing = "msb"; # smoothing method: wb = witten-bell, sb = kneser-ney, msb = modified-kneser-ney
my $dummy;

GetOptions("order=s"  => \$order,
           "text=s"   => \$corpusPath,
           "lm=s"     => \$lmPath,
           "cores=s"  => \$cores,
           "irst-dir=s"  => \$irstPath,
           "temp-dir=s"  => \$tempPath,
           "p=i" => \$pruneSingletons,   # irstlm parameter: prune singletons
           "s=s" => \$smoothing, # irstlm parameter: smoothing method
	   "interpolate!" => \$dummy,  #ignore
	   "kndiscount!" => \$dummy    #ignore
	   ) or exit 1;

#die("ERROR: please set order") unless defined($order);
die("ERROR: please set text") unless defined($corpusPath);
die("ERROR: please set lm") unless defined($lmPath);
die("ERROR: please set irst-dir") unless defined($irstPath);


$tempPath .= "/irstlm-build-tmp.$$";
`mkdir -p $tempPath`;

# add <s> and </s>
my $cmd = "cat $corpusPath | $irstPath/add-start-end.sh > $tempPath/setagged";
print STDERR "EXECUTING $cmd\n";
`$cmd`;

# collect n-gram counts
$cmd = "$irstPath/ngt -i=$tempPath/setagged -n=$order -b=yes -o=$tempPath/counts";
print STDERR "EXECUTING $cmd\n";
`$cmd`;

# build lm
$cmd = "$irstPath/tlm -o=$lmPath -lm=$smoothing -bo=yes -n=$order -tr=$tempPath/counts";
$cmd .= " -ps=no" unless $pruneSingletons;
print STDERR "EXECUTING $cmd\n";
`$cmd`;

$cmd = "rm -rf $tempPath";
print STDERR "EXECUTING $cmd\n";
`$cmd`;

print STDERR "FINISH.\n";
