#!/usr/bin/env perl

use warnings;
use strict;

BEGIN {
use Cwd qw/ abs_path /;
use File::Basename;
my $script_dir = dirname(abs_path($0));
print STDERR  "script_dir=$script_dir\n";
push @INC, $script_dir;
}

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

my $outPath = "$results_dir/pt.half";

my $scorerArgs = `cat $test_dir/$test_name/args.txt`;
$_ = $scorerArgs;
s/(\$\w+)/$1/eeg;
$scorerArgs = $_;

my $cmdMain = "$scoreExe $scorerArgs \n";

open  CMD, ">$results_dir/cmd_line";
print CMD "$cmdMain";
close CMD;

`$cmdMain`;

my $truthPath = "$test_dir/$test_name/truth/results.txt";


if (-e $outPath)
{
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
}
else
{
  print STDERR "FAILURE. Output does not exists. Ran $cmdMain\n";
  exit 1;
}

###################################
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


