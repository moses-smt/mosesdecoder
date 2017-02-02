#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
my $script_dir; BEGIN { use Cwd qw/ abs_path /; use File::Basename; $script_dir = dirname(abs_path($0)); push @INC, $script_dir; }
use MosesScriptsRegressionTesting;
use Getopt::Long;
use File::Temp qw ( tempfile );
use POSIX qw ( strftime );
my @SIGS = qw ( SIGHUP SIGINT SIGQUIT SIGILL SIGTRAP SIGABRT SIGIOT SIGBUS SIGFPE SIGKILL SIGUSR1 SIGSEGV SIGUSR2 SIGPIPE SIGALRM SIGTERM SIGSTKFLT SIGCHLD SIGCONT SIGSTOP SIGTSTP SIGTTIN SIGTTOU SIGURG SIGXCPU SIGXFSZ SIGVTALRM SIGPROF SIGWINCH SIGIO SIGPWR SIGSYS SIGUNUSED SIGRTMIN );
my ($decoder, $test_name);

my $test_dir = "$script_dir/tests";
my $moses_scripts_dir;
my $mert_scripts_dir;
my $data_dir;
my $BIN_TEST = $script_dir;
my $results_dir;
$decoder="$script_dir/moses-virtual";

GetOptions("test=s"    => \$test_name,
           "moses-scripts-dir=s"=> \$moses_scripts_dir,
           "mert-scripts-dir=s"=> \$mert_scripts_dir,
           "data-dir=s"=> \$data_dir,
           "test-dir=s"=> \$test_dir,
           "results-dir=s"=> \$results_dir,
          ) or exit 1;

die "Please specify a test to run with --test\n" unless $test_name;

die "Please specify the location of the data directory with --data-dir\n" unless $data_dir;

die "Please specify the location of the moses directory with --moses-scripts-dir\n" unless $moses_scripts_dir;

die "Please specify the location of the mert directory with --mert-scripts-dir\n" unless $mert_scripts_dir;

die "Cannot locate test dir at $test_dir" unless (-d $test_dir);

$moses_scripts_dir = abs_path($moses_scripts_dir);
$mert_scripts_dir = abs_path($mert_scripts_dir);
$decoder = abs_path($decoder);

$test_dir .= "/$test_name";
die "Cannot locate test dir at $test_dir" unless (-d $test_dir);

#### get place to put results
unless (defined $results_dir) { $results_dir = "$data_dir/results"; }
if (!-d $results_dir) {
  print STDERR "[WARNING] Results directory not found.\n";
  mkdir ($results_dir) || die "Failed to create $results_dir";
}
$results_dir .= "/$test_name";
if (!-d $results_dir) {
  print STDERR "[WARNING] Results directory for test=$test_name could not be found.\n";
  mkdir ($results_dir) || die "Failed to create $results_dir";
}
##########

my $ts = get_timestamp("$test_dir/command");
my $results = "$results_dir/$ts";
mkdir($results) || die "Failed to create results directory: $results\n";

my $truth = "$test_dir/truth";
if (!-d $truth) {
  die "Could not find truth/ in $test_dir!\n";
}

print "RESULTS AVAILABLE IN: $results\n\n";

my ($o, $elapsed, $ec, $sig) = exec_test($test_dir, $results);
my $error = ($sig || $ec > 0);
if ($error) {
  open OUT, ">$results/Summary";
  print STDERR "$test_name CRASHED.\n\texit_code=$ec\n\tsignal=$sig\n";
  print OUT    "$test_name CRASHED.\n\texit_code=$ec\n\tsignal=$sig\n";
  close OUT;
  exit 2 if $sig;
  exit 3;
}

($o, $ec, $sig) = run_command("$test_dir/filter-stdout $results/run.stdout > $results/results.dat");
warn "filter-stdout failed!" if ($ec > 0 || $sig);
($o, $ec, $sig) = run_command("$test_dir/filter-stderr $results/run.stderr >> $results/results.dat");
warn "filter-stderr failed!" if ($ec > 0 || $sig);

open OUT, ">> $results/results.dat";
print OUT "TOTAL_WALLTIME ~ $elapsed\n";
close OUT;

run_command("gzip $results/run.stdout");
run_command("gzip $results/run.stderr");

($o, $ec, $sig) = run_command("$BIN_TEST/compare-results.pl $results $truth");
print $o;
if ($ec) {
  print STDERR "FAILURE, for debugging see  $test_dir\n";
  exit 1;
}

exit 0;

sub exec_test {
  my ($test_dir,$results) = @_;
  my $start_time = time;
warn "Executing: sh $test_dir/command $moses_scripts_dir $mert_scripts_dir $decoder $data_dir $test_dir 1> $results/run.stdout 2> $results/run.stderr\n\n";
  my ($o, $ec, $sig) = run_command("sh $test_dir/command $moses_scripts_dir $mert_scripts_dir $decoder $data_dir $test_dir 1> $results/run.stdout 2> $results/run.stderr");
  my $elapsed = 0;
  $elapsed = time - $start_time;
  return ($o, $elapsed, $ec, $sig);
}

sub run_command {
  my ($cmd) = @_;
  my $o = `$cmd`;
  my $exit_code = $? >> 8;

  my $signal = $? & 127;
  my $core_dumped = $? & 128;
  if ($signal) { $signal = sig_name($signal); }
  return $o, $exit_code, $signal;
}

sub sig_name {
  my $sig = shift;
  return $SIGS[$sig];
}

sub get_timestamp {
  my ($file) = @_;
  my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime,$blksize,$blocks) = stat($file);
  my $timestamp = strftime("%Y%m%d-%H%M%S", gmtime $mtime);
  my $timestamp2 = strftime("%Y%m%d-%H%M%S", gmtime);
  my $username = `whoami`; chomp $username;
  return "command.v$timestamp-$username-at-$timestamp2";
}

