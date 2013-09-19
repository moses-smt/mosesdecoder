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

GetOptions("scorer=s" => \$scoreExe,
           "test=s"    => \$test_name,
           "data-dir=s"=> \$data_dir,
           "test-dir=s"=> \$test_dir,
           "results-dir=s"=> \$results_dir,
          ) or exit 1;

# output dir
unless (defined $results_dir) 
{ 
  my $ts = get_timestamp($scoreExe);
  $results_dir = "$data_dir/results/$test_name/$ts"; 
}

`mkdir -p $results_dir`;

my $outPath = "$results_dir/phrase-table.4.half.e2f";

my $cmdMain = "$scoreExe $test_dir/$test_name/extract.inv.sorted $test_dir/$test_name/lex.e2f $outPath  --Inverse --GoodTuring \n";
`$cmdMain`;

my $truthPath = "$test_dir/$test_name/truth/results.txt";
my $cmd = "diff $outPath $truthPath | wc -l";

my $numDiff = `$cmd`;

if ($numDiff == 0)
{
  #  print STDERR "FAILURE. Ran $cmdMain\n";
  print STDERR "SUCCESS\n";
  exit 0;
}
else
{
  print STDERR "FAILURE. Ran $cmdMain\n";
  exit 1;
}

sub get_timestamp {
  my ($file) = @_;
	my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
		 $atime,$mtime,$ctime,$blksize,$blocks)
								= stat($file);
  my $timestamp = strftime("%Y%m%d-%H%M%S", gmtime $mtime);
  my $timestamp2 = strftime("%Y%m%d-%H%M%S", gmtime);
  my $username = `whoami`; chomp $username;
  return "moses.v$timestamp-$username-at-$timestamp2";
}


