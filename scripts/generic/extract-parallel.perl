#! /usr/bin/perl -w 

# example
#  ./extract-parallel.perl 8 ./coreutils-8.9/src/split "./coreutils-8.9/src/sort --batch-size=253" ./extract ./corpus.5.en ./corpus.5.ar ./align.ar-en.grow-diag-final-and ./extracted 7 --NoFileLimit orientation 

use strict;
use File::Basename;

sub RunFork($);
sub systemCheck($);
sub NumStr($);

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
for (my $i = 8; $i < $#ARGV + 1; ++$i)
{
  $makeTTable = 0 if $ARGV[$i] eq "--NoTTable";
  $otherExtractArgs .= $ARGV[$i] ." ";
}

my $cmd;
my $TMPDIR=dirname($extract)  ."/tmp.$$";
$cmd = "mkdir -p $TMPDIR";
`$cmd`;

my $totalLines = int(`cat $align | wc -l`);
my $linesPerSplit = int($totalLines / $numParallel) + 1;

print "total=$totalLines line-per-split=$linesPerSplit \n";

my @children;
my $pid;

if ($numParallel > 1)
{
	$cmd = "$splitCmd -d -l $linesPerSplit -a 5 $target $TMPDIR/target.";
	$pid = RunFork($cmd);
	push(@children, $pid);
	
	$cmd = "$splitCmd -d -l $linesPerSplit -a 5 $source $TMPDIR/source.";
	$pid = RunFork($cmd);
	push(@children, $pid);

	$cmd = "$splitCmd -d -l $linesPerSplit -a 5 $align $TMPDIR/align.";
	$pid = RunFork($cmd);
	push(@children, $pid);
	
	# wait for everything is finished
	foreach (@children) {
		waitpid($_, 0);
	}

}
else
{
  my $numStr = NumStr(0);

  $cmd = "ln -s $target $TMPDIR/target.$numStr";
	print STDERR "Executing: $cmd \n";
	`$cmd`;

  $cmd = "ln -s $source $TMPDIR/source.$numStr";
	print STDERR "Executing: $cmd \n";
	`$cmd`;

  $cmd = "ln -s $align $TMPDIR/align.$numStr";
	print STDERR "Executing: $cmd \n";
	`$cmd`;
}

# run extract
@children = ();
for (my $i = 0; $i < $numParallel; ++$i)
{
  my $pid = fork();
  
  if ($pid == 0)
  { # child
    my $numStr = NumStr($i);
    my $cmd = "$extractCmd $TMPDIR/target.$numStr $TMPDIR/source.$numStr $TMPDIR/align.$numStr $TMPDIR/extract.$numStr $otherExtractArgs --SentenceOffset ".($i*$linesPerSplit)." 2>> /dev/stderr \n";
    print STDERR $cmd;
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
my $is_osx = ($^O eq "darwin");
my $catCmd = $is_osx?"gunzip -c ":"zcat ";
my $catInvCmd = $catCmd;
my $catOCmd = $catCmd;
for (my $i = 0; $i < $numParallel; ++$i)
{
		my $numStr = NumStr($i);
		$catCmd .= "$TMPDIR/extract.$numStr.gz ";
		$catInvCmd .= "$TMPDIR/extract.$numStr.inv.gz ";
		$catOCmd .= "$TMPDIR/extract.$numStr.o.gz ";
}

$catCmd .= " | LC_ALL=C $sortCmd -T $TMPDIR | gzip -c > $extract.sorted.gz \n";
$catInvCmd .= " | LC_ALL=C $sortCmd -T $TMPDIR | gzip -c > $extract.inv.sorted.gz \n";
$catOCmd .= " | LC_ALL=C $sortCmd -T $TMPDIR | gzip -c > $extract.o.sorted.gz \n";


@children = ();
if ($makeTTable)
{
  $pid = RunFork($catCmd);
  push(@children, $pid);

  $pid = RunFork($catInvCmd);
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

# delete temporary files
$cmd = "rm -rf $TMPDIR \n";
print STDERR $cmd;
`$cmd`;

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

