#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use File::Temp qw/ tempfile tempdir /;

if (scalar(@ARGV) != 7) {
  print STDERR "Usage: run-wade.perl wade-script filtered-dir input reference alignment output wade-output\n";
  exit 1;
}

my ($wade_script, $filtered_dir, $input, $reference, $alignment, $output, $wade_output) = @ARGV;
my ($cfh, $cfile) = tempfile();

#unless ($phrase_table =~ /.*\.gz/){ $phrase_table .= ".gz";}

print $cfh "phrase-table=$filtered_dir/phrase-table.0-0.1.1.gz\n";
print $cfh "fr_test=$input\n";
print $cfh "en_test=$reference\n";
print $cfh "fren_align_test=$alignment\n";
print $cfh "translation_details=$output.details\n";

print $cfh "candidatealign_out=$wade_output.candalign\n";
print $cfh "annotatedalign_out=$wade_output.annotalign\n";
print $cfh "transspans_out=$wade_output.transspans\n";
print $cfh "wade_output=$wade_output\n";
close $cfh;

print $cfile . "\n";

system("python $wade_script $cfile");
