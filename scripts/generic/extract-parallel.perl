#!/usr/bin/env perl 
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# example
#  ./extract-parallel.perl 8 ./coreutils-8.9/src/split "./coreutils-8.9/src/sort --batch-size=253" ./extract ./corpus.5.en ./corpus.5.ar ./align.ar-en.grow-diag-final-and ./extracted 7 --NoFileLimit orientation --GZOutput

use warnings;
use strict;
use File::Basename;

sub RunFork($);
sub systemCheck($);
sub NumStr($);
sub DigitStr($);
sub CharStr($);
sub GetSplitVersion($);

my $alph = "abcdefghijklmnopqrstuvwxyz";
my @alph = (split(//,$alph));

print "Started ".localtime() ."\n";

my $numParallel= $ARGV[0];
$numParallel = 1 if $numParallel < 1;

my $splitCmd= $ARGV[1];
my $sortCmd= $ARGV[2];
my $extractCmd= $ARGV[3];

my $target = $ARGV[4]; # 1st arg of extract argument
my $source = $ARGV[5]; # 2nd arg of extract argument
my $align = $ARGV[6]; # 3rd arg of extract argument
my $extract = $ARGV[7]; # 4th arg of extract argument

my $makeTTable = 1; # whether to build the ttable extract files
my $otherExtractArgs= "";
my $weights = "";
my $baselineExtract;
my $glueFile;
my $phraseOrientation = 0;
my $phraseOrientationPriorsFile;
my $splitCmdOption = "";

my $GZIP_EXEC;
if(`which pigz 2> /dev/null`) {
  $GZIP_EXEC = 'pigz';
}
else {
  $GZIP_EXEC = 'gzip';
}
print STDERR "using $GZIP_EXEC \n";

my $isBSDSplit = GetSplitVersion($splitCmd);
print STDERR "isBSDSplit=$isBSDSplit \n";

if ($isBSDSplit == 0) {
	$splitCmdOption .= "-d";
}

my $gzOut = 0; 

for (my $i = 8; $i < $#ARGV + 1; ++$i)
{
  $makeTTable = 0 if $ARGV[$i] eq "--NoTTable";
  if ($ARGV[$i] eq '--BaselineExtract') {
    $baselineExtract = $ARGV[++$i];
    next;
  }
  if ($ARGV[$i] eq '--InstanceWeights') {
    $weights = $ARGV[++$i];
    next;
  }
  if ($ARGV[$i] eq '--GlueGrammar') {
    $glueFile = $ARGV[++$i];
    next;
  }
  $phraseOrientation = 1 if $ARGV[$i] eq "--PhraseOrientation";
  if ($ARGV[$i] eq '--PhraseOrientationPriors') {
    $phraseOrientationPriorsFile = $ARGV[++$i];
    next;
  }
  if ($ARGV[$i] eq '--GZOutput') {
  	$gzOut = 1;
  }

  $otherExtractArgs .= $ARGV[$i] ." ";
}

die("Need to specify --GZOutput for parallel extract") if ($gzOut == 0);

my $cmd;
my $TMPDIR=dirname($extract)  ."/tmp.$$";
$cmd = "mkdir -p $TMPDIR; ls -l $TMPDIR";
print STDERR "Executing: $cmd \n";
`$cmd`;

my $totalLines = int(`cat $align | wc -l`);
my $linesPerSplit = int($totalLines / $numParallel) + 1;

print "total=$totalLines line-per-split=$linesPerSplit \n";

my @children;
my $pid;

if ($numParallel > 1)
{
	$cmd = "$splitCmd $splitCmdOption -l $linesPerSplit -a 7 $target $TMPDIR/target.";
	$pid = RunFork($cmd);
	push(@children, $pid);

	$cmd = "$splitCmd $splitCmdOption -l $linesPerSplit -a 7 $source $TMPDIR/source.";
	$pid = RunFork($cmd);
	push(@children, $pid);

	$cmd = "$splitCmd $splitCmdOption -l $linesPerSplit -a 7 $align $TMPDIR/align.";
	$pid = RunFork($cmd);
	push(@children, $pid);

  if ($weights) {
    $cmd = "$splitCmd $splitCmdOption -l $linesPerSplit -a 7 $weights $TMPDIR/weights.";
    $pid = RunFork($cmd);
    push(@children, $pid);
  }

	# wait for everything is finished
	foreach (@children) {
		waitpid($_, 0);
	}

}
else
{
  my $numStr = NumStr(0);

  $cmd = "ln -s $target $TMPDIR/target.$numStr";
	`$cmd`;

  $cmd = "ln -s $source $TMPDIR/source.$numStr";
	`$cmd`;

  $cmd = "ln -s $align $TMPDIR/align.$numStr";
	`$cmd`;

  if ($weights) {
    $cmd = "ln -s $weights $TMPDIR/weights.$numStr";
    `$cmd`;
  }
}

# run extract
@children = ();
for (my $i = 0; $i < $numParallel; ++$i)
{
  my $pid = fork();

  if ($pid == 0)
  { # child
    my $numStr = NumStr($i);
    my $weightsCmd = "";
    if ($weights) {
      $weightsCmd = "--InstanceWeights $TMPDIR/weights.$numStr";
    }

    my $glueArg = "";
    if (defined($glueFile)) {
      $glueArg = "--GlueGrammar $TMPDIR/glue.$numStr";
    }
    #print STDERR "glueArg=$glueArg \n";

    my $cmd = "$extractCmd $TMPDIR/target.$numStr $TMPDIR/source.$numStr $TMPDIR/align.$numStr $TMPDIR/extract.$numStr $glueArg $otherExtractArgs $weightsCmd --SentenceOffset ".($i*$linesPerSplit)." 2>> /dev/stderr \n";
    `$cmd`;

    exit();
  }
  else
  { # parent
  	push(@children, $pid);
  }
}

# wait for everything is finished
foreach (@children) {
	waitpid($_, 0);
}

# merge
my $catCmd = "gunzip -c ";
my $catInvCmd = $catCmd;
my $catOCmd = $catCmd;
my $catContextCmd = $catCmd;
my $catContextInvCmd = $catCmd;

for (my $i = 0; $i < $numParallel; ++$i)
{
		my $numStr = NumStr($i);
		$catCmd .= "$TMPDIR/extract.$numStr.gz ";
		$catInvCmd .= "$TMPDIR/extract.$numStr.inv.gz ";
		$catOCmd .= "$TMPDIR/extract.$numStr.o.gz ";
		$catContextCmd .= "$TMPDIR/extract.$numStr.context ";
		$catContextInvCmd .= "$TMPDIR/extract.$numStr.context.inv ";
}
if (defined($baselineExtract)) {
		my $sorted = -e "$baselineExtract.sorted.gz" ? ".sorted" : "";
		$catCmd .= "$baselineExtract$sorted.gz ";
		$catInvCmd .= "$baselineExtract.inv$sorted.gz ";
		$catOCmd .= "$baselineExtract.o$sorted.gz ";
}

$catCmd .= " | LC_ALL=C $sortCmd -T $TMPDIR 2>> /dev/stderr | $GZIP_EXEC -c > $extract.sorted.gz 2>> /dev/stderr \n";
$catInvCmd .= " | LC_ALL=C $sortCmd -T $TMPDIR 2>> /dev/stderr | $GZIP_EXEC -c > $extract.inv.sorted.gz 2>> /dev/stderr \n";
$catOCmd .= " | LC_ALL=C $sortCmd -T $TMPDIR 2>> /dev/stderr | $GZIP_EXEC -c > $extract.o.sorted.gz 2>> /dev/stderr \n";
$catContextCmd .= " | LC_ALL=C $sortCmd -T $TMPDIR 2>> /dev/stderr | uniq | $GZIP_EXEC -c > $extract.context.sorted.gz 2>> /dev/stderr \n";
$catContextInvCmd .= " | LC_ALL=C $sortCmd -T $TMPDIR 2>> /dev/stderr | uniq | $GZIP_EXEC -c > $extract.context.inv.sorted.gz 2>> /dev/stderr \n";


@children = ();
if ($makeTTable)
{
  print STDERR "merging extract / extract.inv\n";
  $pid = RunFork($catCmd);
  push(@children, $pid);

  $pid = RunFork($catInvCmd);
  push(@children, $pid);
}
else {
  print STDERR "skipping extract, doing only extract.o\n";
}

if ($otherExtractArgs =~ /--FlexibilityScore/) {
  $pid = RunFork($catContextCmd);
  push(@children, $pid);

  $pid = RunFork($catContextInvCmd);
  push(@children, $pid);
  }

my $numStr = NumStr(0);
if (-e "$TMPDIR/extract.$numStr.o.gz")
{
	$pid = RunFork($catOCmd);
	push(@children, $pid);
}

# wait for all sorting to finish
foreach (@children) {
	waitpid($_, 0);
}

# merge glue rules
if (defined($glueFile)) {
  my $cmd = "cat $TMPDIR/glue.* | LC_ALL=C sort | uniq > $glueFile";
  print STDERR "Merging glue rules: $cmd \n";
  print STDERR `$cmd`;
}

# merge phrase orientation priors (GHKM extraction)
if ($phraseOrientation && defined($phraseOrientationPriorsFile)) {
  print STDERR "Merging phrase orientation priors\n";

  my @orientationPriorsCountFiles = glob("$TMPDIR/*.phraseOrientationPriors");
  my %priorCounts;

  foreach my $filenamePhraseOrientationPriors (@orientationPriorsCountFiles) {
    if (-f $filenamePhraseOrientationPriors) {
      open my $infilePhraseOrientationPriors, '<', $filenamePhraseOrientationPriors or die "cannot open $filenamePhraseOrientationPriors: $!";
      while (my $line = <$infilePhraseOrientationPriors>) {
        print $line;
        my ($key, $value) = split / /, $line;
        $priorCounts{$key} += $value;
      }
      close $infilePhraseOrientationPriors;
    }
  }

  open my $outPhraseOrientationPriors, '>', $phraseOrientationPriorsFile or die "cannot open $phraseOrientationPriorsFile: $!";
  foreach my $key (sort keys %priorCounts) {
    print $outPhraseOrientationPriors $key." ".$priorCounts{$key}."\n";
  }
  close($outPhraseOrientationPriors);
}

# delete temporary files
$cmd = "rm -rf $TMPDIR \n";
systemCheck($cmd);

print STDERR "Finished ".localtime() ."\n";

# -----------------------------------------
# -----------------------------------------

sub RunFork($)
{
  my $cmd = shift;

  my $pid = fork();

  if ($pid == 0)
  { # child
    print STDERR $cmd;
    systemCheck($cmd);
    exit();
  }
  return $pid;
}

sub systemCheck($)
{
  my $cmd = shift;
  my $retVal = system($cmd);
  if ($retVal != 0)
  {
    exit(1);
  }
}

sub DigitStr($)
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

sub CharStr($)
{
    my $i = shift;
    my $charStr;
    my @bit=();

    while ($i>0){
        push @bit, $i%26;
        $i=int($i/26);
    }
    my $offset=scalar(@bit);
    my $h;
    for ($h=6;$h>=$offset;--$h) { $charStr.="a"; }
    for ($h=$offset-1;$h>=0;--$h) { $charStr.="$alph[$bit[$h]]"; }
    return $charStr;
}

sub NumStr($)
{
    my $i = shift;
    if ($isBSDSplit){
        return CharStr($i);
    }else{
        return DigitStr($i);
    }
}

sub GetSplitVersion($)
{
	my $splitCmd = shift;
	my $retVal = system("$splitCmd --help > /dev/null");
	if ($retVal != 0) {
		return 1;
	}
	else {
		return 0;
	}
}

