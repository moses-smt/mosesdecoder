#!/usr/bin/env perl

#######################
# Revision history
#
# 28 Apr 2015 first version

use warnings;
use strict;
use Getopt::Long qw(:config pass_through no_ignore_case permute);

my ($BIN,$IN,$MAX_LINES,$SETTINGS,$REVERSE,$TMP);

GetOptions('bin=s' => \$BIN,
           'i=s' => \$IN,
           'max-lines=i' => \$MAX_LINES,
           'settings=s' => \$SETTINGS,
           'r' => \$REVERSE,
           'tmp=s' => \$TMP,
          ) or exit(1);

die("ERROR - usage: fast-align-in-parts.perl -bin FAST_ALIGN_BIN -i PARALLEL_CORPUS -max-lines COUNT -settings CONFIG [-r] -tmp TMPDIR")
  unless defined($BIN) && defined($IN) && defined($SETTINGS) && defined($TMP) && defined($MAX_LINES)
      && $MAX_LINES > 0;
die("ERROR - input file does not exist: $IN") unless -e $IN;
die("ERROR - fast_align binary does not exist: $BIN") unless -e $BIN;
        
chomp(my $line_count = `cat $IN | wc -l`);

# not more than maximal number of lines -> just run it regulary
if ($MAX_LINES > $line_count) {
  my $cmd = "$BIN -i $IN $SETTINGS";
  $cmd .= " -r" if defined($REVERSE);
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
  $cmd .= " >$output_file";
  safesystem($cmd) or die;
  die("ERROR: no output produced from command $cmd") unless -e $output_file;

  # check line count
  chomp(my $input_line_count = `cat $input_file | wc -l`);
  chomp(my $output_line_count = `cat $output_file | wc -l`);
  die("ERROR: mismatched number of lines in part $1\n\t$input_line_count\t$input_file\n\t$output_line_count\t$output_file\n") unless $input_line_count == $output_line_count;
}

# join output
$cmd = "cat $TMP/aligned-*";
safesystem($cmd) or die;

$cmd = "rm -r $TMP/* ; rmdir $TMP";
safesystem($cmd);

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

