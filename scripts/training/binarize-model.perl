#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

#
# Binarize a Moses model
#

use warnings;
use strict;

use Getopt::Long "GetOptions";
use FindBin qw($RealBin);

$ENV{"LC_ALL"} = "C";
my $SCRIPTS_ROOTDIR = $RealBin;
if ($SCRIPTS_ROOTDIR eq '') {
    $SCRIPTS_ROOTDIR = dirname(__FILE__);
}
$SCRIPTS_ROOTDIR =~ s/\/training$//;

my ($binarizer, $input_config, $output_config);
my $opt_hierarchical = 0;
$binarizer = "$SCRIPTS_ROOTDIR/../bin/processPhraseTable";
my $min_score = undef;
GetOptions(
  "Hierarchical" => \$opt_hierarchical,
  "Binarizer=s" => \$binarizer,
    "MinScore=s" => \$min_score,
) or exit(1);

$input_config = shift;
$output_config = shift;

if (!defined $input_config || !defined $output_config) {
  print STDERR "usage: binarize-model.perl input-config output-config [-Binarizer binarizer] [-MinScore score-def]\n";
  exit 1;
}

my $hierarchical = "";
$hierarchical = "-Hierarchical" if $opt_hierarchical;
my $targetdir = "$output_config.tables";

my $cmd = "$RealBin/filter-model-given-input.pl  $targetdir $input_config /dev/null $hierarchical -nofilter -Binarizer \"$binarizer\" ";
$cmd .= "-MinScore $min_score" if defined($min_score);
safesystem($cmd) || die "binarising failed";
safesystem("rm -f $output_config; ln -s $targetdir/moses.ini $output_config") || die "failed to link new ini file";

#FIXME: Why isn't this in a module?
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
      exit(1);
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}


