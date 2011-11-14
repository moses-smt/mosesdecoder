#!/usr/bin/perl -w

# $Id$

use strict;
use FindBin qw($Bin);

my $script_dir; BEGIN { use Cwd qw/ abs_path /; use File::Basename; $script_dir = dirname(abs_path($0)); push @INC, $script_dir; }
use Getopt::Long;

############################################################
my @tests = qw (
  score.phrase-based
  score.phrase-based-inv
  score.phrase-based-with-alignment
  score.phrase-based-with-alignment-inv
  score.hierarchical
  score.hierarchical-inv
  mert.basic
  mert.pro
  mert.extractor-txt
  mert.extractor-bin
  chart.target-syntax
  chart.target-syntax.ondisk
  chart.hierarchical
  chart.hierarchical-withsrilm
  chart.hierarchical.ondisk
  phrase.basic-surface-only
  phrase.basic-surface-only-withirstlm
  phrase.basic-surface-only-withirstlm-binlm
  phrase.basic-lm-oov
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
  phrase.lexicalized-reordering-bin
  phrase.lexicalized-reordering-cn
  phrase.consensus-decoding-surface
  phrase.continue-partial-translation
  phrase.show-weights.lex-reorder
  phrase.show-weights
  phrase.xml-markup
);
  #phrase.basic-lm-oov-withkenlm
  #phrase.basic-surface-only-withkenlm
  #phrase.basic-surface-only-withkenlm.bin
  #chart.hierarchical-withkenlm

############################################################
use MosesRegressionTesting;
use File::Temp qw ( tempfile );
use POSIX qw ( strftime );

my $decoderPhrase = "$Bin/../moses-cmd/src/moses";
my $decoderChart = "$Bin/../moses-chart-cmd/src/moses_chart";
my $scoreExe = "$Bin/../scripts/training/phrase-extract/score";
my $kenlmBinarizer = "$Bin/../kenlm/build_binary";
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
  	$cmd .= "$BIN_TEST/run-single-test.perl $test_run --decoder=$decoderPhrase";
  }
  elsif ($model_type eq 'chart.')
  {
  	$cmd .= "$BIN_TEST/run-single-test.perl $test_run --decoder=$decoderChart";
  }
  elsif ($model_type eq 'score.')
  {
    $cmd .= "$BIN_TEST/run-test-scorer.perl $test_run --scorer=$scoreExe";
  }
  elsif ($test =~ /^mert/)
  {
    $cmd .= "$BIN_TEST/run-test-mert.perl $test_run";
  }
  elsif ($test =~ /^kenlmbin/)
  {
  	$cmd .= "$BIN_TEST/run-kenlm-binarizer.perl --binarizer=$kenlmBinarizer";
  }
  else 
  {
  	print "FAIL";	
  }
  
  $cmd .= " --test=$test";
  
print STDERR "cmd = $cmd\n";

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
exit 2 if $fail > 0;

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

