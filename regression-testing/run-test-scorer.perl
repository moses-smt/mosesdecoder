#!/usr/bin/perl -w

use strict;
use FindBin qw($Bin);
use MosesRegressionTesting;
use Getopt::Long;
use File::Temp qw ( tempfile );
use POSIX qw ( strftime );

my $scoreExe;
my $test_name;
my $data_dir;
my $test_dir;
my $results_dir;

GetOptions("decoder=s" => \$scoreExe,
           "test=s"    => \$test_name,
           "data-dir=s"=> \$data_dir,
           "test-dir=s"=> \$test_dir,
           "results-dir=s"=> \$results_dir,
          ) or exit 1;

my $outPath = "$test_dir/phrase-table.4.half.f2e";
my $cmdMain = "$scoreExe $test_dir/extract.sorted $test_dir/lex.f2e $outPath  --GoodTuring \n";

`$cmdMain`;

my $truthPath = "$test_dir/truth/results.txt";
my $cmd = "diff $outPath $truthPath | wc -l";

my $numDiff = 554;
$numDiff = `$cmd`;

if ($numDiff == 0)
{
  print STDERR "SUCCESS\n";
  exit 0;
}
else
{
  print STDERR "FAILURE. Ran $cmdMain\n";
  exit 1;
}
