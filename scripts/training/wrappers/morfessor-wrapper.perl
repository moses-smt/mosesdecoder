#!/usr/bin/env perl 

use warnings;
use strict;
use Getopt::Long "GetOptions";

my $MORF_DIR;
my $MODEL;

GetOptions('morfessor-dir=s' => \$MORF_DIR,
           'model=s' => \$MODEL);

die("Must provide --model=s argument") if (!defined($MODEL));

my $cmd = "";

if (defined($MORF_DIR)) {
  $cmd .= "PYTHONPATH=$MORF_DIR  $MORF_DIR/scripts/";
}

my $TMP_FILE = "/tmp/morf.$$";
$cmd .= "morfessor-segment "
       ."-L $MODEL "
       ."--output-format \"{analysis} \" "
       ."--output-format-separator \" \" "
       ."--output-newlines "
       ."/dev/stdin "
       ."| sed 's/ \$//' > $TMP_FILE";
print STDERR "Executing: $cmd\n";
`$cmd`;


open(FILE, $TMP_FILE) or die("Can't open file $TMP_FILE");
while (my $line = <FILE>) {
    print "$line";
}
close(FILE);
