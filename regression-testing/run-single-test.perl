#!/usr/bin/perl -w

# $Id$

use strict;
my $script_dir; BEGIN { use Cwd qw/ abs_path /; use File::Basename; $script_dir = dirname(abs_path($0)); push @INC, $script_dir; }
use MosesRegressionTesting;
use Getopt::Long;
use File::Temp qw ( tempfile );
use POSIX qw ( strftime );
my @SIGS = qw ( SIGHUP SIGINT SIGQUIT SIGILL SIGTRAP SIGABRT SIGIOT SIGBUS SIGFPE SIGKILL SIGUSR1 SIGSEGV SIGUSR2 SIGPIPE SIGALRM SIGTERM SIGSTKFLT SIGCHLD SIGCONT SIGSTOP SIGTSTP SIGTTIN SIGTTOU SIGURG SIGXCPU SIGXFSZ SIGVTALRM SIGPROF SIGWINCH SIGIO SIGPWR SIGSYS SIGUNUSED SIGRTMIN );
my ($decoder, $test_name);

my $test_dir = "$script_dir/tests";
my $data_dir;
my $BIN_TEST = $script_dir;
my $results_dir;
my $NBEST = 0;

GetOptions("decoder=s" => \$decoder,
           "test=s"    => \$test_name,
           "data-dir=s"=> \$data_dir,
           "test-dir=s"=> \$test_dir,
           "results-dir=s"=> \$results_dir,
          ) or exit 1;

die "Please specify a decoder with --decoder\n" unless $decoder;
die "Please specify a test to run with --test\n" unless $test_name;

die "Please specify the location of the data directory with --data-dir\n" unless $data_dir;

die "Cannot locate test dir at $test_dir" unless (-d $test_dir);

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

my $conf = "$test_dir/moses.ini";
my $input = "$test_dir/to-translate.txt";

die "Cannot locate executable called $decoder\n" unless (-x $decoder);
die "Cannot find $conf\n" unless (-f $conf);
die "Cannot locate input at $input" unless (-f $input);

my $local_moses_ini = MosesRegressionTesting::get_localized_moses_ini($conf, $data_dir, $results_dir);
my ($nbestfile,$nbestsize) = MosesRegressionTesting::get_nbestlist($conf);

if (defined($nbestsize) && $nbestsize > 0){
  $NBEST=$nbestsize;
}

my $ts = get_timestamp($decoder);
my $results = "$results_dir/$ts";
mkdir($results) || die "Failed to create results directory: $results\n";

my $truth = "$test_dir/truth";
if (!-d $truth) {
  die "Could not find truth/ in $test_dir!\n";
}

print "RESULTS AVAILABLE IN: $results\n\n";

my ($o, $elapsed, $ec, $sig) = exec_moses($decoder, $local_moses_ini, $input, $results);
my $error = ($sig || $ec > 0);
if ($error) {
  open OUT, ">$results/Summary";
  print STDERR "MOSES CRASHED.\n\texit_code=$ec\n\tsignal=$sig\n";
  print OUT    "MOSES CRASHED.\n\texit_code=$ec\n\tsignal=$sig\n";
  print STDERR "FAILURE, for debugging, local moses.ini=$local_moses_ini\n";
  print OUT    "FAILURE, for debugging, local moses.ini=$local_moses_ini\n";
  close OUT;
  exit 2 if $sig;
  exit 3;
}

($o, $ec, $sig) = run_command("$test_dir/filter-stdout.pl $results/run.stdout > $results/results.txt");
warn "filter-stdout failed!" if ($ec > 0 || $sig);
($o, $ec, $sig) = run_command("$test_dir/filter-stderr.pl $results/run.stderr >> $results/results.txt");
warn "filter-stderr failed!" if ($ec > 0 || $sig);

if($NBEST > 0){
  ($o, $ec, $sig) = run_command("$test_dir/filter-nbest.pl $results/run.nbest >> $results/results.txt");
  warn "filter-nbest failed!" if ($ec > 0 || $sig);
}

open OUT, ">>$results/results.txt";
print OUT "TOTAL_WALLTIME ~ $elapsed\n";
close OUT;

run_command("gzip $results/run.stdout");
run_command("gzip $results/run.stderr");
if($NBEST > 0){
  run_command("gzip $results/run.nbest");
}

($o, $ec, $sig) = run_command("$BIN_TEST/compare-results.perl $results $truth");
print $o;
if ($ec) {
  print STDERR "FAILURE, for debugging, local moses.ini=$local_moses_ini\n";
  exit 1;
}

unlink $local_moses_ini or warn "Couldn't remove $local_moses_ini\n";
exit 0;

sub exec_moses {
  my ($decoder, $conf, $input, $results) = @_;
  my $start_time = time;
  my ($o, $ec, $sig);
  if ($NBEST > 0){
        print STDERR "Nbest output file is $results/run.nbest\n";
        print STDERR "Nbest size is $NBEST\n";
	($o, $ec, $sig) = run_command("$decoder -f $conf -i $input -n-best-list $results/run.nbest $NBEST 1> $results/run.stdout 2> $results/run.stderr");
  }
  else{
	($o, $ec, $sig) = run_command("$decoder -f $conf -i $input 1> $results/run.stdout 2> $results/run.stderr");
  }
  my $elapsed = time - $start_time;
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
	my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
		 $atime,$mtime,$ctime,$blksize,$blocks)
								= stat($file);
  my $timestamp = strftime("%Y%m%d-%H%M%S", gmtime $mtime);
  my $timestamp2 = strftime("%Y%m%d-%H%M%S", gmtime);
  my $username = `whoami`; chomp $username;
  return "moses.v$timestamp-$username-at-$timestamp2";
}

