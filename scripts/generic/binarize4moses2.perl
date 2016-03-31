#!/usr/bin/env perl

use strict;

use Getopt::Long;
use File::Basename;
use FindBin qw($RealBin);

my $mosesDir = "$RealBin/../..";
my $ptPath;
my $lexRoPath;
my $outPath;
my $numLexScores;
my $pruneNum = 0;

GetOptions("phrase-table=s"  => \$ptPath,
           "lex-ro=s"   => \$lexRoPath,
           "output-dir=s" => \$outPath,
           "num-lex-scores=i" => \$numLexScores,
           "prune=i" => \$pruneNum
	   ) or exit 1;

die("ERROR: please set --phrase-table") unless defined($ptPath);
die("ERROR: please set --lex-ro") unless defined($lexRoPath);
die("ERROR: please set --output-dir") unless defined($outPath);
die("ERROR: please set --num-lex-scores") unless defined($numLexScores);

my $cmd;

my $tempPath = dirname($outPath)  ."/tmp.$$";
`mkdir -p $tempPath`;

$cmd = "gzip -dc $ptPath |  $mosesDir/contrib/sigtest-filter/filter-pt -n $pruneNum | gzip -c > $tempPath/pt.gz";
system($cmd);

$cmd = "$mosesDir/bin/processLexicalTableMin  -in $lexRoPath -out $tempPath/lex-ro -T . -threads all";
system($cmd);

$cmd = "$mosesDir/bin/addLexROtoPT pt.gz $tempPath/lex-ro.minlexr  | gzip -c > $tempPath/pt.withLexRO.gz";
system($cmd);

$cmd = "$mosesDir/bin/CreateProbingPT2 --num-lex-scores $numLexScores --log-prob --input-pt $tempPath/pt.withLexRO.gz --output-dir $outPath";
system($cmd);

