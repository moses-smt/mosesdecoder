#!/usr/bin/perl -w

use strict;
use Date::Parse;

my $file = $ARGV[0] || die;
die unless -e $file;

my $start;
open(OUT,$file.".STDOUT");
my $start_line = <OUT>;
if ($start_line =~ /starting at (.+) on \S+$/) {
  $start = str2time($1);
}
close(OUT);
my $current = time();

&progress_run_giza($file) if $file =~ /TRAINING_run-giza/;
&progress_extract($file) if $file =~ /TRAINING_extract-phrases/;
&progress_decode($file) if $file =~ /EVALUATION_.+_decode/;

sub progress_extract {
  my ($file) = @_;

  my $dot_line = `tail -n 1 $file.STDERR`;
  chop($dot_line);
  $dot_line =~ s/^\.//g;
  my $lines_processed = length($dot_line)*10000;

  my $total = `grep ^total $file.STDOUT`;
  return unless $total =~ /^total=(\d+)/;
  my $lines_total = $1;

  my $ratio = $lines_processed/$lines_total;
  my $remaining = &generic_remaining($ratio);
  print &format_progress($ratio,$remaining);
}

sub progress_run_giza {
  my ($file) = @_;
  my $info;
  my $max_sent = 0;
  my $sent = 0;

  my ($iter_m1,$iter_hmm,$iter_m3,$iter_m4) = (5,5,5,5);
  my $ratio = "?";
  my $already = 0;
  my $added = 0;
  my $total = 1;
  my $factor;

  open(GIZA,$file.".STDOUT");
  while(<GIZA>) {
    $iter_m1 = $1 if /^model1iterations = (\d+)/;
    $iter_hmm = $1 if /^hmmiterations = (\d+)/;
    $iter_m3 = $1 if /^model3iterations = (\d+)/;
    $iter_m4 = $1 if /^model4iterations = (\d+)/;

    if (/starting at (.+) on \S+$/) {
      $info = "start";
      $total = $iter_m1/10+$iter_hmm+$iter_m3+$iter_m4*3;
    }
    elsif (/Model1 Training Started at: (.+)/) {
      $info = "m1:it1";
      $added += $2;
    }
    elsif (/Model 1 Iteration: (\d+) took: (\d+) seconds/) {
      $info = "m1:it".($1+1);
      $info = "hmm:it1" if $1 == $iter_m1;
      $added += $2;
      $already = $1;
      $factor = ($1 == $iter_m1) ? 1 : 0.1;
    }
    elsif (/Hmm Iteration: (\d+) took: (\d+) seconds/) {
      $info = "hmm:it".($1+1);
      $info = "m3:it1" if $1 == $iter_hmm;
      $added += $2;
      $already = $iter_m1/10+$1;
      $factor = 1;
    }
    elsif (/THTo3 Viterbi Iteration : (\d+) took: (\d+) seconds/) {
      $info = "m3:it2";
      $added += $2;
      $already = $iter_m1/10+$iter_hmm+1;
      $factor = 1;
    }
    elsif (/Model3 Viterbi Iteration : (\d+) took: (\d+) seconds/) {
      $info = "m3:it".($1+1);
      $info = "m4:it1" if $1 == $iter_m3;
      $added += $2;
      $already = $iter_m1/10+$iter_hmm+$1;
      $factor = ($1 == $iter_m3) ? 3 : 1;
    }
    elsif (/T3To4 Viterbi Iteration : (\d+) took: (\d+) seconds/) {
      $info = "m4:it2";
      $added += $2;
      $already = $iter_m1/10+$iter_hmm+$iter_m3+3;
      $factor = 3;
    }
    elsif (/Model4 Viterbi Iteration : (\d+) took: (\d+) seconds/) {
      $info = "m4:it".($1-$iter_m3+1);
      $added += $2;
      $already = $iter_m1/10+$iter_hmm+$iter_m3+3*($1-$iter_m3);
      $factor = 3;
    }
    elsif (/\[sent:(\d+)\]/) {
      $sent = $1;
      $max_sent = $1 if $1 > $max_sent;
    }
  }
  close(GIZA);

  if ($sent > 0) {
    $already += $sent/$max_sent * $factor;
  }
  else {
    $already += (($current-$start-$added)/($current-$start)-1);
  }

  return $info unless $already > 0;
  $ratio = $already/$total;
  my $remaining = &generic_remaining($ratio);
  print $info."<BR>".&format_progress($ratio,$remaining);
}

sub progress_decode {
  my ($file) = @_;
  open(FILE,$file);
  my ($input_file,$output_file);
  while(<FILE>) {
    $input_file = $1 if /\< *(\S+)/;
    $output_file = $1 if /\> *(\S+)/;
  }
  close(FILE);
  return unless defined($input_file);
  return unless defined($output_file);

  return unless $file =~ /^(.+)\/steps\/\d+\/EVAL/;
  my $base_dir = $1;

  return unless $input_file =~ /(\/evaluation\/[^\/]+)$/;
  $input_file = $base_dir.$1;
  return unless $output_file =~ /(\/evaluation\/[^\/]+)$/;
  $output_file = $base_dir.$1;
  return unless -e $input_file && -e $output_file;

  my $total = int(`cat $input_file | wc -l`);
  my $already = int(`cat $output_file | wc -l`);
  return unless $already;

  my $ratio = $already/$total;
  my $remaining = &generic_remaining($ratio);
  print &format_progress($ratio,$remaining);
}

sub generic_remaining {
  my ($ratio) = @_;
  return ($current-$start)*(1/$ratio-1);
}

sub format_progress {
  my ($ratio,$remaining) = @_;
  return "" if $ratio eq "?";
  $ratio = .99 if $ratio >= 1;
  $remaining = 60 if $remaining < 60;
  if ($remaining >= 36000) {
    return sprintf("%d%s %dh left\n",$ratio*100,'%',$remaining/3600);
  }
  if ($remaining >= 3600) {
    return sprintf("%d%s %.1fh left\n",$ratio*100,'%',$remaining/3600);
  }
  return sprintf("%d%s %dm left\n",$ratio*100,'%',$remaining/60);
}


