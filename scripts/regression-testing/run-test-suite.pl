#!/usr/bin/perl

use strict;
my $script_dir; BEGIN { use Cwd qw/ abs_path /; use File::Basename; $script_dir = dirname(abs_path($0)); push @INC, $script_dir; }
use Getopt::Long;

############################################################
my @tests = qw (
  mert-moses-new
  mert-moses-new-nocase
  mert-moses-new-aggregate
  mert-moses-new-continue
);

###########################################################

use MosesScriptsRegressionTesting;
use File::Temp qw ( tempfile );
use POSIX qw ( strftime );

my $test_dir;
my $moses_scripts_dir;
my $mert_scripts_dir;
my $data_dir;
my $BIN_TEST = $script_dir;

GetOptions("data-dir=s" => \$data_dir,
           "moses-scripts-dir=s"=> \$moses_scripts_dir,
           "mert-scripts-dir=s"=> \$mert_scripts_dir,
          ) or exit 1;

$data_dir = MosesScriptsRegressionTesting::find_data_directory($BIN_TEST, $data_dir);

my $test_run = "$BIN_TEST/run-single-test.pl --data-dir=$data_dir";
$test_dir = $script_dir . "/tests";
$test_run .= " --test-dir=$test_dir" if $test_dir;
$test_run .= " --moses-scripts-dir=$moses_scripts_dir" if $moses_scripts_dir;
$test_run .= " --mert-scripts-dir=$mert_scripts_dir" if $mert_scripts_dir;

print "Data directory: $data_dir\n";
print "Scripts directory: $moses_scripts_dir\n";

print "Running tests: @tests\n\n";

print "TEST NAME               STATUS     PATH TO RESULTS\n";
my $lb = "---------------------------------------------------------------------------------------------------------\n";
print $lb;

my $fail = 0;
my @failed;
foreach my $test (@tests) {
  my $cmd = "$test_run --test=$test";
  my ($res, $output, $results_path) = do_test($cmd);
  format STDOUT =
@<<<<<<<<<<<<<<<<<<<<<< @<<<<<<<<< @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$test,                   $res,      $results_path
.
  write;
  if ($res eq 'FAIL') {
    print "$lb$output$lb";
    $fail++;
    push @failed, $test;
  } else {
# TOTAL_WALLTIME  result=BASELINE=11, TEST=12       DELTA=1        PCT CHANGE=9.09
    if ($output =~ /TOTAL_WALLTIME\s+result\s*=\s*([^\n]+)/o) {
      print "\t\tTiming statistics: $1\n";
    }
  }
}

my $total = scalar @tests;
my $fail_percentage = int(100 * $fail / $total);
my $pass_percentage = int(100 * ($total-$fail) / $total);
print "\n$pass_percentage% of the tests passed.\n";
print "$fail_percentage% of the tests failed.\n";
if ($fail_percentage>0) { print "\nPLEASE INVESTIGATE THESE FAILED TESTS: @failed\n"; }

sub do_test {
  my ($test) = @_;
  my $o = `$test 2>&1`;
  my $res = 'PASS';
  $res = 'FAIL' if ($? > 0);
  my $od = '';
  if ($o =~ /RESULTS AVAILABLE IN: (.*)$/m) {
    $od = $1;
    $o =~ s/^RESULTS AVAIL.*$//mo;
  }
  return ($res, $o, $od);
}

