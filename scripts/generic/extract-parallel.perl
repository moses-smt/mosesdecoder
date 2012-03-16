#! /usr/bin/perl

# example
#  ./extract-parallel.perl 8 ./coreutils-8.9/src/split "./coreutils-8.9/src/sort --batch-size=253" ./extract ./corpus.5.en ./corpus.5.ar ./align.ar-en.grow-diag-final-and ./extracted 7 --NoFileLimit orientation 

use strict;
use File::Basename;

sub NumStr($);

	   print "Started ".localtime() ."\n";

	   my $numParallel= $ARGV[0];
	   my $splitCmd= $ARGV[1];
	   my $sortCmd= $ARGV[2];
	   my $extractCmd= $ARGV[3];

	   my $target = $ARGV[4]; # 1st arg of extract argument
	   my $source = $ARGV[5]; # 2nd arg of extract argument
	   my $align = $ARGV[6]; # 3rd arg of extract argument
	   my $extract = $ARGV[7]; # 4th arg of extract argument

	   my $otherExtractArgs= "";
	   for (my $i = 8; $i < $#ARGV + 1; ++$i)
{
    $otherExtractArgs .= $ARGV[$i] ." ";
}

my $TMPDIR=dirname($extract)  ."/tmp.$$";
mkdir $TMPDIR;

my $totalLines = int(`wc -l $align`);
my $linesPerSplit = int($totalLines / $numParallel) + 1;

print "total=$totalLines line-per-split=$linesPerSplit \n";

my $cmd = "$splitCmd -d -l $linesPerSplit -a 5 $target $TMPDIR/target.";
print STDERR "Executing: $cmd \n";
`$cmd`;

$cmd = "$splitCmd -d -l $linesPerSplit -a 5 $source $TMPDIR/source.";
print STDERR "Executing: $cmd \n";
`$cmd`;

$cmd = "$splitCmd -d -l $linesPerSplit -a 5 $align $TMPDIR/align.";
print STDERR "Executing: $cmd \n";
`$cmd`;

# run extract
my $isParent = 1;
my @childs;
for (my $i = 0; $i < $numParallel; ++$i)
{
  my $pid = fork();
  
  if ($pid == 0)
  { # child
    $isParent = 0;
    my $numStr = NumStr($i);
    my $cmd = "$extractCmd $TMPDIR/target.$numStr $TMPDIR/source.$numStr $TMPDIR/align.$numStr $TMPDIR/extract.$numStr $otherExtractArgs \n";
    print STDERR $cmd;
    `$cmd`;

    exit();
  }
  else
  { # parent
  	push(@childs, $pid);
  }
}

# wait for everything is finished
if ($isParent)
{
  foreach (@childs) {
  	waitpid($_, 0);
  }
}
else
{
    die "shouldn't be here";
}

# merge
if ($numParallel > 1)
{
  my $extractCmd = "cat ";
  my $extractInvCmd = "cat ";
  my $extractOrderingCmd = "cat ";
  for (my $i = 0; $i < $numParallel; ++$i)
  {
      my $numStr = NumStr($i);
      $extractCmd .= "$TMPDIR/extract.$numStr ";
      $extractInvCmd .= "$TMPDIR/extract.$numStr.inv ";
      $extractOrderingCmd .= "$TMPDIR/extract.$numStr.o ";
  }

  $extractCmd .= "> $extract \n";
  $extractInvCmd .= "> $extract.inv \n";
  $extractOrderingCmd .= "> $extract.o \n";
  print STDERR $extractCmd;
  print STDERR $extractInvCmd;
  print STDERR $extractOrderingCmd;

  systemCheck($extractCmd);
  systemCheck($extractInvCmd);

  my $numStr = NumStr(0);
  if (-e "$TMPDIR/extract.$numStr.o")
  {
    systemCheck($extractOrderingCmd);
  }
}
else
{
  rename "$TMPDIR/extract.0", "$extract";
  rename "$TMPDIR/extract.0.inv", "$extract.inv";


  my $numStr = NumStr(0);
  if (-e "$TMPDIR/extract.$numStr.o")
  {
    rename "$TMPDIR/extract.0.o", "$extract.o";
  }
}


$cmd = "rm -rf $TMPDIR \n";
print STDERR $cmd;
`$cmd`;

print STDERR "Finished ".localtime() ."\n";

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


