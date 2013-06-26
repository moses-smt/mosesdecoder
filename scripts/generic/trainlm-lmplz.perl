#!/usr/bin/perl -w

# Compatible with sri LM-creating script, eg.
#    ngram-count -order 5 -interpolate -wbdiscount -unk -text corpus.txt -lm lm.txt
# To use it in the EMS, add this to the [LM] section
#    lm-training = "$moses-script-dir/generic/trainlm-lmplz.perl -lmplz $lmplz"
#    settings = "-T $working-dir/tmp -S 10G"
# Also, make sure that $lmplz is defined (in the [LM] or [GENERAL] section. 
# It should point to the binary file
#    lmplz = /home/waziz/workspace/github/moses/bin/lmplz

use strict;
use FindBin qw($RealBin);
use Getopt::Long qw/GetOptionsFromArray/;
#use Getopt::Long;
Getopt::Long::Configure("pass_through", "no_ignore_case");

my $order = 3; # order of language model (default trigram)
my $corpus; # input text data
my $lm; # generated language model
my $lmplz; # bin directory of IRSTLM 
my $help = 0;

my @optconfig = (
    "-order=s"  => \$order,
    "-text=s"   => \$corpus,
    "-lm=s"     => \$lm,
    "-lmplz=s"  => \$lmplz,
);

GetOptionsFromArray(\@ARGV, @optconfig);
die("ERROR: please set text") unless defined($corpus);
die("ERROR: please set lm") unless defined($lm);
die("ERROR: please set lmplz") unless defined($lmplz);

my $settings = join(' ', @ARGV);
my $cmd = "$lmplz --order $order $settings < $corpus > $lm";

print STDERR "EXECUTING $cmd\n";
`$cmd`;
