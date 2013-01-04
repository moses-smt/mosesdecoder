#!/usr/bin/perl -w

# Compatible with sri LM-creating script, eg.
#    ngram-count -order 5 -interpolate -wbdiscount -unk -text corpus.txt -lm lm.txt
# To use it in the EMS, add this to the [LM] section
#    lm-training = "$moses-script-dir/generic/trainlm-irst.perl -cores $cores -irst-dir $irst-dir"
#    settings = ""
# Also, make sure that $irst-dir is defined (in the [LM] or [GENERAL] section. 
# It should point to the root of the LM toolkit, eg
#    irst-dir = /Users/hieu/workspace/irstlm/trunk/bin
# And make sure that $cores is defined, eg $cores = 8
# And make sure the $settings variable is empty. This script doesn't understand some of the sri args like -unk and will complain.

use strict;
use FindBin qw($RealBin);
use Getopt::Long;

my $order = 3;
my $corpusPath;
my $lmPath;
my $cores = 2;
my $irstPath;
my $tempPath = "tmp";
my $p = 1;
my $s;
my $temp;

GetOptions("order=s"  => \$order,
           "text=s"   => \$corpusPath,
           "lm=s"     => \$lmPath,
           "cores=s"  => \$cores,
           "irst-dir=s"  => \$irstPath,
           "temp-dir=s"  => \$tempPath,
           "p=i" => \$p,   # irstlm parameter: delete singletons
           "s=s" => \$s, # irstlm parameter: smoothing method
	   "interpolate!" => \$temp,  #ignore
	   "kndiscount!" => \$temp    #ignore
	   ) or exit 1;

#die("ERROR: please set order") unless defined($order);
die("ERROR: please set text") unless defined($corpusPath);
die("ERROR: please set lm") unless defined($lmPath);
die("ERROR: please set irst-dir") unless defined($irstPath);

my $ext = ($corpusPath =~ m/([^.]+)$/)[0];
print "extension is $ext\n";

$tempPath .= "/irstlm-build-tmp.$$";
`mkdir -p $tempPath`;

my $cmd;
if ($ext eq "gz")
{
    $cmd = "zcat $corpusPath | $irstPath/add-start-end.sh | gzip -c > $tempPath/monolingual.setagged.gz";
}
else
{
    $cmd = "cat $corpusPath | $irstPath/add-start-end.sh | gzip -c > $tempPath/monolingual.setagged.gz";
}
print STDERR "EXECUTING $cmd\n";
`$cmd`;

$cmd = "IRSTLM=$irstPath/.. $irstPath/build-lm.sh -t $tempPath/stat4 -i \"gunzip -c $tempPath/monolingual.setagged.gz\" -n $order -o $tempPath/iarpa.gz -k $cores";
$cmd .= " -p" if $p;
$cmd .= " -s $s" if defined($s);
print STDERR "EXECUTING $cmd\n";
`$cmd`;

$ext = ($lmPath =~ m/([^.]+)$/)[0];
print "extension is $ext\n";

if ($ext eq "gz")
{
    $cmd = "$irstPath/compile-lm --text $tempPath/iarpa.gz /dev/stdout | gzip -c > $lmPath";
}
else
{
    $cmd = "$irstPath/compile-lm --text $tempPath/iarpa.gz $lmPath";
}

print STDERR "EXECUTING $cmd\n";
`$cmd`;

$cmd = "rm -rf $tempPath";
print STDERR "EXECUTING $cmd\n";
`$cmd`;

print STDERR "FINISH.\n";
