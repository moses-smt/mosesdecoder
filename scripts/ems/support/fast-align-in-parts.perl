#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

#######################
# Revision history
#
# 28 Apr 2015 first version

use warnings;
use strict;
use Getopt::Long qw(:config pass_through no_ignore_case permute);

my ($BIN,$IN,$OUT,$MAX_LINES,$SETTINGS,$REVERSE,$SAVE_MODEL,$TMP);

GetOptions('bin=s' => \$BIN,
           'i=s' => \$IN,
           'o=s' => \$OUT,
           'max-lines=i' => \$MAX_LINES,
           'settings=s' => \$SETTINGS,
           'save-model=s' => \$SAVE_MODEL,
           'r' => \$REVERSE,
           'tmp=s' => \$TMP,
          ) or exit(1);

die("ERROR - usage: fast-align-in-parts.perl -bin FAST_ALIGN_BIN -i PARALLEL_CORPUS -max-lines COUNT -settings CONFIG [-r] -tmp TMPDIR [-save-model MODEL] -o ALIGNMENTS")
  unless defined($BIN) && defined($IN) && defined($SETTINGS) && defined($TMP) 
      && defined($MAX_LINES) && defined($OUT)
      && $MAX_LINES > 0;
die("ERROR - input file does not exist: $IN") unless -e $IN;
die("ERROR - fast_align binary does not exist: $BIN") unless -e $BIN;

$SAVE_MODEL = defined($SAVE_MODEL) && $SAVE_MODEL && $SAVE_MODEL ne 'no';
chomp(my $line_count = `cat $IN | wc -l`);

# not more than maximal number of lines -> just run it regulary
if ($MAX_LINES > $line_count) {
  my $cmd = "$BIN -i $IN $SETTINGS >$OUT";
  $cmd .= " -r" if defined($REVERSE);
  $cmd .= " -p $OUT.parameters 2> $OUT.log" if $SAVE_MODEL;
  safesystem($cmd) or die;
  exit(0);
}

my $cmd = "mkdir -p $TMP";
safesystem($cmd) or die;

# split input
$cmd = "split -a 2 -l $MAX_LINES $IN $TMP/prepared-";
safesystem($cmd) or die;

# process
my @INPUT_FILES = `ls $TMP/prepared-*`;
chop(@INPUT_FILES);
foreach my $input_file (@INPUT_FILES) {
  # create output file name
  die("ERROR") unless $input_file =~ /prepared-(..)$/;
  my $output_file = "$TMP/aligned-$1";

  # process part
  my $cmd = "$BIN -i $input_file $SETTINGS";
  $cmd .= " -r" if defined($REVERSE);
  $cmd .= " -p $output_file.parameters 2> $output_file.log" if $SAVE_MODEL;
  $cmd .= " >$output_file";
  safesystem($cmd) or die;
  die("ERROR: no output produced from command $cmd") unless -e $output_file;

  # check line count
  chomp(my $input_line_count = `cat $input_file | wc -l`);
  chomp(my $output_line_count = `cat $output_file | wc -l`);
  die("ERROR: mismatched number of lines in part $1\n\t$input_line_count\t$input_file\n\t$output_line_count\t$output_file\n") unless $input_line_count == $output_line_count;
}

# join output
$cmd = "cat $TMP/aligned-?? > $OUT";
safesystem($cmd) or die;

# join model
&join_model(scalar @INPUT_FILES) if $SAVE_MODEL;
&join_log(scalar @INPUT_FILES) if $SAVE_MODEL;

$cmd = "rm $TMP/* ; rmdir $TMP";
safesystem($cmd);

sub join_model {
  my ($count) = @_;
  open(CONCAT,"cat $TMP/aligned-*.parameters | LC_ALL=C sort -T $TMP -S 10%|");
  open(JOINED,">$OUT.parameters");
  my ($last_f,$last_e,$f,$e,$score,$merged_score);
  while(<CONCAT>) {
    ($f,$e,$score) = split;
    if (!defined($last_f) || $f ne $last_f || $e ne $last_e) {
      printf JOINED "%s %s %f\n",$last_f,$last_e,log($merged_score) if defined($last_f);
      $last_f = $f;
      $last_e = $e;
      $merged_score = 0;
    } 
    $merged_score += exp($score)/$count;
  }
  printf JOINED "%s %s %f\n",$f,$e,log($merged_score);
  close(CONCAT);
  close(JOINED);
}

sub merge_entry {
  my ($count,$f,$e,@SCORE) = @_;
  my $score = 0;
  foreach (@SCORE) {
    $score += exp($_)/$count;
  }
  $score = log($score);
  print JOINED "$f $e $score\n";
}

sub join_log {
  my ($count) = @_;
  open(CONCAT,"cat $TMP/aligned-*.log |");
  my ($length,$tension,$tension_count) = (0,0,0);
  while(<CONCAT>) {
    $length += $1 if /expected target length = source length \* ([\d\.]+)/;
    $tension += $1 if /final tension: ([\d\.]+)/ and (++$tension_count % 3 == 0);
  }
  close(CONCAT);
  $length /= $count;
  $tension /= $count;
  open(JOINED,">$OUT.log");
  print JOINED "expected target length = source length * $length\n";
  print JOINED "     final tension: $tension\n";
  close(JOINED);
}

sub safesystem {
  print STDERR "Executing: @_\n";
  system(@_);
  if ($? == -1) {
    print STDERR "Failed to execute: @_\n  $!\n";
    exit(1);
  }
  elsif ($? & 127) {
    printf STDERR "Execution of: @_\n  died with signal %d, %s coredump\n",
      ($? & 127),  ($? & 128) ? 'with' : 'without';
    exit 1;
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}

