#!/usr/bin/perl -w

# $Id$

use strict;
my $script_dir; BEGIN { use Cwd qw/ abs_path /; use File::Basename; $script_dir = dirname(abs_path($0)); push @INC, $script_dir; }
use Getopt::Long;

############################################################
my @tests = qw (
  chart.target-syntax
  chart.hierarchical
  chart.target-syntax.ondisk
  chart.hierarchical.ondisk
  phrase.basic-surface-only
  phrase.ptable-filtering
  phrase.multi-factor
  phrase.multi-factor-drop
  phrase.confusionNet-surface-only
  phrase.confusionNet-multi-factor
  phrase.basic-surface-binptable
  phrase.multi-factor-binptable
  phrase.nbest-multi-factor
  phrase.lattice-surface
  phrase.lattice-distortion
  phrase.lexicalized-reordering
  phrase.lexicalized-reordering-cn
  phrase.consensus-decoding-surface
  phrase.continue-partial-translation
  phrase.show-weights.lex-reorder
  phrase.show-weights
  phrase.xml-markup
);

############################################################
use MosesRegressionTesting;
use File::Temp qw ( tempfile );
use POSIX qw ( strftime );

my $decoderPhrase;
my $decoderChart;
my $test_dir;
my $BIN_TEST = $script_dir;
my $data_dir;

GetOptions(	"decoder-phrase=s" => \$decoderPhrase,
			"decoder-chart=s" => \$decoderChart,
           	"data-dir=s" => \$data_dir,
          ) or exit 1;

$data_dir = MosesRegressionTesting::find_data_directory($BIN_TEST, $data_dir);

my $test_run = "$BIN_TEST/run-single-test.pl --data-dir=$data_dir";
$test_dir = $script_dir . "/tests";
$test_run .= " --test-dir=$test_dir" if $test_dir;

print "Data directory: $data_dir\n";

die "Please specify the phrase-based decoder & the chart decoder to test with --decoder-phrase=[path] --decoder-chart=[path] \n" unless ($decoderPhrase and $decoderChart);

die "Cannot locate executable called $decoderPhrase\n" unless (-x $decoderPhrase);

print "Running tests: @tests\n\n";

print "TEST NAME               STATUS     PATH TO RESULTS\n";
my $lb = "---------------------------------------------------------------------------------------------------------\n";
print $lb;

my $fail = 0;
my @failed;
foreach my $test (@tests) 
{
  my $cmd;
  my $model_type = substr($test, $[, 6);

  if ($model_type eq 'phrase')
  {
  	$cmd .= "$test_run --decoder=$decoderPhrase";
  }
  elsif ($model_type eq 'chart.')
  {
  	$cmd .= "$test_run --decoder=$decoderChart";
  }
  else 
  {
  	print "FAIL";	
  }
  
  $cmd .= " --test=$test";
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

