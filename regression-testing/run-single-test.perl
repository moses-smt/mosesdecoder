#!/usr/bin/env perl

# $Id$

use Encode;
use utf8;
use warnings;
use strict;
my $script_dir; BEGIN { use Cwd qw/ abs_path /; use File::Basename; $script_dir = dirname(abs_path($0)); push @INC, $script_dir; }
use MosesRegressionTesting;
use Getopt::Long;
use File::Temp qw ( tempfile );
use POSIX qw ( strftime );
use POSIX ":sys_wait_h";
my @SIGS = qw ( SIGHUP SIGINT SIGQUIT SIGILL SIGTRAP SIGABRT SIGIOT SIGBUS SIGFPE SIGKILL SIGUSR1 SIGSEGV SIGUSR2 SIGPIPE SIGALRM SIGTERM SIGSTKFLT SIGCHLD SIGCONT SIGSTOP SIGTSTP SIGTTIN SIGTTOU SIGURG SIGXCPU SIGXFSZ SIGVTALRM SIGPROF SIGWINCH SIGIO SIGPWR SIGSYS SIGUNUSED SIGRTMIN );
my ($decoder, $test_name);

my $test_dir = "$script_dir/tests";
my $data_dir;
my $BIN_TEST = $script_dir;
my $results_dir;
my $NBEST = 0;
my $run_server_test = 0;
my $serverport = int(rand(9999)) + 10001;
my $url = "http://localhost:$serverport/RPC2";
my $startupTest = 0;
GetOptions("decoder=s" => \$decoder,
           "test=s"    => \$test_name,
           "data-dir=s"=> \$data_dir,
           "test-dir=s"=> \$test_dir,
           "results-dir=s"=> \$results_dir,
           "server"=> \$run_server_test,
           "startuptest"=> \$startupTest
          ) or exit 1;

if($run_server_test)
{
  eval {
    require XMLRPC::Lite;
    import XMLRPC::Lite;
  };
  if ($@) {
    die "Error: XMLRPC::Lite not installed, moses server regression tests will not be run. $@";
  }
  exit(0) if($startupTest);
}

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

if (defined($nbestsize) && $nbestsize > 0) {
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
my ($o, $elapsed, $ec, $sig);
if($run_server_test) {
  ($o, $elapsed, $ec, $sig) = exec_moses_server($decoder, $local_moses_ini, $input, $results);
}
else {
  ($o, $elapsed, $ec, $sig) = exec_moses($decoder, $local_moses_ini, $input, $results);
}
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
  my $cmd;
  if ($NBEST > 0){
        print STDERR "Nbest output file is $results/run.nbest\n";
        print STDERR "Nbest size is $NBEST\n";
	$cmd = "$decoder -f $conf -i $input -n-best-list $results/run.nbest $NBEST 1> $results/run.stdout 2> $results/run.stderr";
  }
  else{
      $cmd = "$decoder -f $conf -i $input 1> $results/run.stdout 2> $results/run.stderr";
  }

  open  CMD, ">$results/cmd_line";
  print CMD "$cmd\n";
  close CMD;

  ($o, $ec, $sig) = run_command($cmd);
  my $elapsed = time - $start_time;
  return ($o, $elapsed, $ec, $sig);
}

sub exec_moses_server {
  my ($decoder, $conf, $input, $results) = @_;
  my $start_time = time;
  my ($o, $ec, $sig);
  $ec = 0; $sig = 0; $o = 0;
  my $pid = fork();
  if (not defined $pid) {
      warn "resources not avilable to fork Moses server\n";
      $ec = 1; # to generate error
  } elsif ($pid == 0) {
      setpgrp(0, 0);
      warn "Starting Moses server on port $serverport ...\n";
      my $cmd = "$decoder --server --server-port $serverport -f $conf -verbose 2 --server-log $results/run.stderr.server 2> $results/run.stderr ";
      open  CMD, ">$results/cmd_line";
      print CMD "$cmd\n";
      close CMD;
      ($o, $ec, $sig) = run_command($cmd);
      exit;
      # this should not be reached unless the server fails to start
  }
  while( 1==1 ) # wait until the server is listening for requests
  {
      sleep 5;
      my $res = waitpid($pid, WNOHANG);
      die "Moses crashed or aborted! Check $results/run.stderr for error messages.\n" if ($res);
      my $str = `grep "Listening on port $serverport" $results/run.stderr`;
      last if($str =~ /Listening/);
  }
  my $proxy = XMLRPC::Lite->proxy($url);
  warn "Opening file $input to write to $results\n";
  open(TEXTIN, "$input") or die "Can not open the input file to translate with Moses server\n";
  binmode TEXTIN, ':utf8';
  open(TEXTOUT, ">$results/run.stdout");
  binmode TEXTOUT, ':utf8';
  while(<TEXTIN>)
  {
    chop;
    my $encoded = SOAP::Data->type(string => $_); # NOTE: assuming properly encoded UTF-8 input: check tests before adding them!
    my %param = ("text" => $encoded);
    my $result = $proxy->call("translate",\%param)->result;
    print TEXTOUT $result->{'text'} . "\n";
  }
  close(TEXTIN);
  close(TEXTOUT);
  my $elapsed = time - $start_time;
  print STDERR "Finished translating file $input\n";
  if(waitpid($pid, WNOHANG) <= 0)
  {
    warn "Killing process group $pid of the $decoder --server ... \n";
    kill 9, -$pid;
  }
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

