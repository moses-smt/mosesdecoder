#!/usr/bin/perl -w

# Compatible with sri LM-creating script, eg.
#    ngram-count -order 5 -interpolate -wbdiscount -unk -text corpus.txt -lm lm.txt
# To use it in the EMS, add this to the [LM] section
#    lm-training = "$moses-script-dir/generic/trainlm-irst.perl -cores $cores -irst-dir $irst-dir"
#    settings = ""
# Also, make sure that $irst-dir is defined (in the [LM] or [GENERAL] section. 
# It should point to the root of the LM toolkit, eg
#    irst-dir = /Users/hieu/workspace/irstlm/trunk
# And make sure that $cores is defined, eg $cores = 8

use strict;
use FindBin qw($Bin);
use Getopt::Long;

my $order;
my $corpusPath;
my $lmPath;
my $cores;
my $irstPath;

GetOptions("order=s"  => \$order,
           "text=s"   => \$corpusPath,
           "lm=s"     => \$lmPath,
           "cores=s"  => \$cores,
           "irst-dir=s"  => \$irstPath,
	   ) or exit 1;

my $ext = ($corpusPath =~ m/([^.]+)$/)[0];
print "extension is $ext\n";

mkdir 'temp';

my $cmd;
if ($ext eq "gz")
{
    $cmd = "zcat $corpusPath | $irstPath/bin/add-start-end.sh | gzip -c > temp/monolingual.setagged.gz";
}
else
{
    $cmd = "cat $corpusPath | $irstPath/bin/add-start-end.sh | gzip -c > temp/monolingual.setagged.gz";
}
print STDERR "EXECUTING $cmd\n";
`$cmd`;

$cmd = "IRSTLM=$irstPath $irstPath/bin/build-lm.sh -t stat4 -i \"gunzip -c temp/monolingual.setagged.gz\" -n $order -p -o temp/iarpa.gz -k $cores";
print STDERR "EXECUTING $cmd\n";
`$cmd`;

$ext = ($lmPath =~ m/([^.]+)$/)[0];
print "extension is $ext\n";

if ($ext eq "gz")
{
    $cmd = "$irstPath/bin/compile-lm temp/iarpa.gz --text yes /dev/stdout | gzip -c > $lmPath";
}
else
{
    $cmd = "$irstPath/bin/compile-lm temp/iarpa.gz --text yes $lmPath";
}

print STDERR "EXECUTING $cmd\n";
`$cmd`;

$cmd = "rm -rf temp stat4";
print STDERR "EXECUTING $cmd\n";
`$cmd`;

print STDERR "FINISH.\n";
