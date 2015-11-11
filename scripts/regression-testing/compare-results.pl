#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
my ($results, $truth) = @ARGV;

my ($report, $pass, $fail) = compare_results("$results/results.dat", "$truth/results.dat");
open OUT, ">$results/Summary";
print OUT $report;
print $report;
close OUT;

if ($fail > 0) {
  print <<EOT;

There were failures in this test run.  Please analyze the results carefully.

EOT
  exit 1;
}
exit 0;

sub compare_results {
  my ($testf, $truthf) = @_;
  my $test = read_results($testf);
  my $truth = read_results($truthf);

  my $ct1 = delete $truth->{'COMPARISON_TYPE'};
  my $ct2 = delete $test->{'COMPARISON_TYPE'};

  my $pass = 0;
  my $fail = 0;
  my $report = '';
  foreach my $k (sort keys %$truth) {
    $report .= "test-name=$k\tresult=";
    if (!exists $test->{$k}) {
      $report .= "missing from test results\n";
      $fail++;
      next;
    }

    my $truthv = $truth->{$k} || '';
    my $testv = delete $test->{$k} || '';

    if ($ct1->{$k} eq '=') {
      if ($truthv eq $testv) {
        $report .= "pass\n";
        $pass++;
      } else {
        $report .= "fail\n\tTRUTH=$truthv\n\t TEST=$testv\n";
        $fail++;
      }
    } else { # numeric difference
      $testv=$testv?$testv:0;
      $truthv=$truthv?$truthv:0;
      my $diff = $testv - $truthv;
      if ($diff == 0) { $report .= "identical\n"; next; }
      $report .= "BASELINE=$truthv, TEST=$testv\t  DELTA=$diff";
      if ($truthv != 0) {
        my $pct = $diff/$truthv;
        my $t = sprintf "\t PCT CHANGE=%4.2f", $pct*100;
        $report .= $t;
      }
      $report .= "\n";
    }
  }
  foreach my $k (sort keys %$test) {
    $fail++;
    $report .= "test-name=$k\tfound in TEST but not in TRUTH.\n";
  }
  $report .= "\nTESTS PASSED=$pass\nTESTS FAILED=$fail\n";
  return $report, $pass, $fail;
}

sub read_results {
  my ($file) = @_;
  open IN, "<$file" or die "Could not open $file!";
  my %res;
  while (my $l = <IN>) {
    if ($l =~ /^([A-Za-z0-9_]+)\s*([=~])\s*(.+)$/) {
      my ($key, $comparison_type, $value) = ($1, $2, $3);
      $res{$key} = $value;
      $res{'COMPARISON_TYPE'}->{$key}=$comparison_type;
    }
  }
  close IN;
  return \%res;
}

