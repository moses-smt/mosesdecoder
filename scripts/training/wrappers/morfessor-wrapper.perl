#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use Getopt::Long "GetOptions";

my $MORF_DIR;
my $MODEL;
my $TMP_DIR = ".";

GetOptions("morfessor-dir=s" => \$MORF_DIR,
           "model=s" => \$MODEL,
           "tmpdir=s" => \$TMP_DIR);

die("Must provide --model=s argument") if (!defined($MODEL));

my $cmd = "";

my $ESC_FILE = "$TMP_DIR/morf.esc.$$";

$cmd = "cat /dev/stdin | sed s/^#/HASH/ > $ESC_FILE";
print STDERR "Executing: $cmd\n";
`$cmd`;

$cmd = "";
if (defined($MORF_DIR)) {
  $cmd .= "PYTHONPATH=$MORF_DIR  $MORF_DIR/scripts/";
}

my $OUT_FILE = "$TMP_DIR/morf.out.$$";
$cmd .= "morfessor-segment "
       ."-L $MODEL "
       ."--output-format \"{analysis} \" "
       ."--output-format-separator \" \" "
       ."--output-newlines "
       ."$ESC_FILE "
       ."| sed 's/ \$//' | sed s/^HASH/#/ > $OUT_FILE";
print STDERR "Executing: $cmd\n";
`$cmd`;


open(FILE, $OUT_FILE) or die("Can't open file $OUT_FILE");
while (my $line = <FILE>) {
    print "$line";
}
close(FILE);

unlink($OUT_FILE);
unlink($ESC_FILE);

