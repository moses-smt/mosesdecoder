#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use utf8;

binmode STDIN, ":utf8";
binmode STDOUT, ":utf8";
binmode STDERR, ":utf8";

sub NumStr($);

my $NUM_SPLIT_LINES = $ARGV[0];

my $TMPDIR = $ARGV[1];
$TMPDIR = "$TMPDIR/tmp.$$";
mkdir $TMPDIR;
print STDERR "TMPDIR=$TMPDIR \n";

my $cmd = "";
for (my $i = 2; $i < scalar(@ARGV); ++$i)
{
  $cmd .= $ARGV[$i] ." ";
}

# split input file
open (INPUT_ALL, "> $TMPDIR/input.all");
binmode INPUT_ALL, ":utf8";
while (my $line = <STDIN>)
{
  chomp($line);
  print INPUT_ALL $line."\n";
}
close(INPUT_ALL);

my $cmd2 = "split -l $NUM_SPLIT_LINES -a 5 -d  $TMPDIR/input.all $TMPDIR/x";
`$cmd2`;

# create exec file
open (EXEC, "> $TMPDIR/exec");
binmode EXEC, ":utf8";

# execute in parallel
print STDERR "executing\n";

my $i = 0;
my $filePath = "$TMPDIR/x" .NumStr($i);
while (-f $filePath)
{
  print EXEC "$cmd < $filePath > $filePath.out\n";

  ++$i;
  $filePath = "$TMPDIR/x" .NumStr($i);
}
close (EXEC);

$cmd2 = "parallel < $TMPDIR/exec";
`$cmd2`;

# concatenate
print STDERR "concatenating\n";

$i = 1;
my $firstPath = "$TMPDIR/x" .NumStr(0) .".out";
$filePath = "$TMPDIR/x" .NumStr($i) .".out";
while (-f $filePath)
{
  $cmd = "cat $filePath >> $firstPath";
  `$cmd`;

  ++$i;
  $filePath = "$TMPDIR/x" .NumStr($i) .".out";
}

# output
open (OUTPUT_ALL, "$firstPath");
binmode OUTPUT_ALL, ":utf8";
while (my $line = <OUTPUT_ALL>)
{
  chomp($line);
  print "$line\n";
}
close(OUTPUT_ALL);

$cmd = "rm -rf $TMPDIR/";
`$cmd`;

###########################################
sub NumStr($)
{
    my $i = shift;
    my $numStr;
    if ($i < 10) {
	$numStr = "000000$i";
    }
    elsif ($i < 100) {
	$numStr = "00000$i";
    }
    elsif ($i < 1000) {
	$numStr = "0000$i";
    }
    elsif ($i < 10000) {
	$numStr = "000$i";
    }
    elsif ($i < 100000) {
	$numStr = "00$i";
    }
    elsif ($i < 1000000) {
	$numStr = "0$i";
    }
    else {
	$numStr = $i;
    }
    return $numStr;
}

