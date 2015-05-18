#!/usr/bin/env perl

use warnings;
use strict;
use Getopt::Long "GetOptions";

my $MORF_DIR;
my $MODEL;
my $TMPDIR = ".";

GetOptions("morfessor-dir=s" => \$MORF_DIR,
           "model=s" => \$MODEL,
           "tmpdir=s" => \$TMPDIR);

die("Must provide --model=s argument") if (!defined($MODEL));

my $cmd = "";

if (defined($MORF_DIR)) {
  $cmd .= "PYTHONPATH=$MORF_DIR  $MORF_DIR/scripts/";
}

my $TMPFILE = "$TMPDIR/morf.$$";
$cmd .= "morfessor-segment "
       ."-L $MODEL "
       ."--output-format \"{analysis} \" "
       ."--output-format-separator \" \" "
       ."--output-newlines "
       ."/dev/stdin "
       ."| sed 's/ \$//' > $TMPFILE";
print STDERR "Executing: $cmd\n";
`$cmd`;


open(FILE, $TMPFILE) or die("Can't open file $TMPFILE");
while (my $line = <FILE>) {
    print "$line";
}
close(FILE);

unlink($TMPFILE);
