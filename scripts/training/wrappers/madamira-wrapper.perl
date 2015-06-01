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

my $FACTORS_STR;
my @FACTORS;

GetOptions(
  "tmpdir=s" => \$TMPDIR,
  "keep-tmp" => \$KEEP_TMP,
  "mada-dir=s" => \$MADA_DIR,
  "factors=s" => \$FACTORS_STR,
  "config=s" => \$CONFIG
    ) or die("ERROR: unknown options");

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

$cmd = "cat $TMPDIR/out/x*.mada > $infile.mada";
print STDERR "Executing: $cmd\n";
`$cmd`;

# get stuff out of mada output
open(MADA_OUT,"<$infile.mada");
#binmode(MADA_OUT, ":utf8");
while(my $line = <MADA_OUT>) {
    chomp($line);
  #print STDERR "line=$line \n";

    if (index($line, "SENTENCE BREAK") == 0) {
    # new sentence
    #print STDERR "BREAK\n";
	print "\n";
    }
    elsif (index($line, ";;WORD") == 0) {
        # word
	my $word = substr($line, 7, length($line) - 8);
        #print STDERR "FOund $word\n";

	for (my $i = 0; $i < 4; ++$i) {
	    $line = <MADA_OUT>;
	}

	my $factors = GetFactors($line, \@FACTORS);
	$word .= $factors;

	print "$word ";
    }
    else {
      #print STDERR "NADA\n";
    }
}
close (MADA_OUT);


if ($KEEP_TMP == 0) {
#    `rm -rf $TMPDIR`;
}


###########################
sub GetFactors
{
    my $line = shift;
    my $factorsRef = shift;
    my @factors = @{$factorsRef};

    # all factors
    my %allFactors;
    my @toks = split(" ", $line);
    for (my $i = 1; $i < scalar(@toks); ++$i) {
	#print " tok=" .$toks[$i];

        my ($key, $value) = split(":", $toks[$i]);
	$allFactors{$key} = $value;
    }

    my $ret = "";
    my $factorType;
    foreach $factorType(@factors) {
	#print "factorType=$factorType ";
	my $value = $allFactors{$factorType};

	$ret .= "|$value";
    }

    return $ret;
}

