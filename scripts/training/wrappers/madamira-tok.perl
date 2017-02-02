#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use File::Temp qw/tempfile/;
use Getopt::Long "GetOptions";
use File::Basename;
use FindBin qw($RealBin);
use Cwd 'abs_path';

sub GetFactors;


my $TMPDIR = "tmp";
my $KEEP_TMP = 0;
my $MADA_DIR;
my $CONFIG;
my $SCHEME;
my $USE_PARALLEL = 1;

my $FACTORS_STR;
my @FACTORS;

GetOptions(
  "tmpdir=s" => \$TMPDIR,
  "keep-tmp" => \$KEEP_TMP,
  "mada-dir=s" => \$MADA_DIR,
  "factors=s" => \$FACTORS_STR,
  "config=s" => \$CONFIG,
  "scheme=s" => \$SCHEME,
  "use-parallel=i" => \$USE_PARALLEL
    ) or die("ERROR: unknown options");

die("must have -scheme arg") unless defined($SCHEME);

if (!defined($CONFIG)) {
  $CONFIG = "$MADA_DIR/samples/sampleConfigFile.xml";
}

$TMPDIR = abs_path($TMPDIR);
print STDERR "TMPDIR=$TMPDIR \n";

if (defined($FACTORS_STR)) {
    @FACTORS = split(",", $FACTORS_STR);
}

#binmode(STDIN, ":utf8");
#binmode(STDOUT, ":utf8");

$TMPDIR = "$TMPDIR/madamira.$$";
`mkdir -p $TMPDIR`;
`mkdir -p $TMPDIR/split`;
`mkdir -p $TMPDIR/out`;

my $infile = "$TMPDIR/input";
print STDERR $infile."\n";

open(TMP,">$infile");
while(<STDIN>) {
    print TMP $_;
}
close(TMP);

my $cmd;

if ($USE_PARALLEL) {
  # split input file
  my $SPLIT_EXEC = `gsplit --help 2>/dev/null`;
  if($SPLIT_EXEC) {
    $SPLIT_EXEC = 'gsplit';
  }
  else {
    $SPLIT_EXEC = 'split';
  }

  $cmd = "$SPLIT_EXEC -l 10000 -a 7 -d  $TMPDIR/input $TMPDIR/split/x";
  `$cmd`;

  $cmd = "cd $MADA_DIR && parallel --jobs 4 java -Xmx2500m -Xms2500m -XX:NewRatio=3 -jar $MADA_DIR/MADAMIRA.jar -rawinput {} -rawoutdir  $TMPDIR/out -rawconfig $CONFIG ::: $TMPDIR/split/x*";
  print STDERR "Executing: $cmd\n";
  `$cmd`;

  $cmd = "cat $TMPDIR/out/x*.$SCHEME.tok > $infile.mada";
  print STDERR "Executing: $cmd\n";
  `$cmd`;
}
else {
  $cmd = "cd $MADA_DIR && java -Xmx2500m -Xms2500m -XX:NewRatio=3 -jar $MADA_DIR/MADAMIRA.jar -rawinput $infile -rawoutdir $TMPDIR/out -rawconfig $CONFIG";
  print STDERR "Executing: $cmd\n";
  `$cmd`;

  $cmd = "cat $TMPDIR/out/input.$SCHEME.tok > $infile.mada";
  print STDERR "Executing: $cmd\n";
  `$cmd`;
}

# get stuff out of mada output
open(MADA_OUT,"<$infile.mada");
#binmode(MADA_OUT, ":utf8");
while(my $line = <MADA_OUT>) {
  chomp($line);
	print "$line\n";
}
close (MADA_OUT);


if ($KEEP_TMP == 0) {
#    `rm -rf $TMPDIR`;
}

