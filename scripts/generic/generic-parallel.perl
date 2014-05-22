#!/usr/bin/perl -w

use strict;
use utf8;

binmode STDOUT, ":utf8";
binmode STDERR, ":utf8";

sub NumStr($);

my $NUM_SPLIT_LINES = $ARGV[0];

my $cmd = "";
for (my $i = 1; $i < $#ARGV; ++$i)
{
  $cmd .= $ARGV[$i] ." ";
}

my $TMPDIR = "/tmp/tmp.$$";
mkdir $TMPDIR;
print STDERR "TMPDIR=$TMPDIR \n";

# split input file
open (INPUT_ALL, "> $TMPDIR/input.all");
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

# execute in parallel
my $i = 0;
my $filePath = "$TMPDIR/x" .NumStr($i);
print STDERR "filePath=$filePath \n";
while (-f $filePath) 
{
  print STDERR "filePath=$filePath \n";
  print EXEC "$cmd < $filePath > $filePath.out\n";

  ++$i;
  $filePath = "$TMPDIR/x" .NumStr($i);
}
close (EXEC);

$cmd2 = "parallel < $TMPDIR/exec";
`$cmd2`;

# concatenate
$i = 1;
my $firstPath = "$TMPDIR/x" .NumStr(0) .".out";
$filePath = "$TMPDIR/x" .NumStr($i) .".out";
print STDERR "filePath=$filePath \n";
while (-f $filePath) 
{
  print STDERR "filePath=$filePath \n";
  $cmd = "cat $filePath >> $firstPath";
  `$cmd`;

  ++$i;
  $filePath = "$TMPDIR/x" .NumStr($i) .".out";
}

# output
open (OUTPUT_ALL, "$firstPath");
while (my $line = <OUTPUT_ALL>)
{ 
  chomp($line);
  print "$line\n";
}

$cmd = "rm -rf $TMPDIR/";
`$cmd`;

###########################################
sub NumStr($)
{
    my $i = shift;
    my $numStr;
    if ($i < 10) {
        $numStr = "0000$i";
    }
    elsif ($i < 100) {
        $numStr = "000$i";
    }
    elsif ($i < 1000) {
        $numStr = "00$i";
    }
    elsif ($i < 10000) {
        $numStr = "0$i";
    }
    else {
        $numStr = $i;
    }
    return $numStr;
}

