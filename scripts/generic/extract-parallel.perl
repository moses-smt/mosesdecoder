#! /usr/bin/perl

# example
#  ../extract-parallel.perl ../extract-rules target.txt source.txt align.txt extracted "" 4 gsplit "gsort --batch-size=253"

use strict;
use File::Basename;

sub NumStr($);

print "Started ".localtime() ."\n";

my $extractCmd	= $ARGV[0];
my $target			= $ARGV[1];
my $source			= $ARGV[2];
my $align				= $ARGV[3];
my $extract			= $ARGV[4];
my $extractArgs	= $ARGV[5];
my $numParallel	= $ARGV[6];
my $splitCmd		= $ARGV[7];
my $sortCmd			= $ARGV[8];

my $TMPDIR=dirname($extract)  ."/tmp.$$";
mkdir $TMPDIR;

my $totalLines = int(`wc -l $align`);
my $linesPerSplit = int($totalLines / $numParallel) + 1;

print "total=$totalLines line-per-split=$linesPerSplit \n";

my $cmd = "$splitCmd -d -l $linesPerSplit -a 5 $target $TMPDIR/target.";
`$cmd`;

$cmd = "$splitCmd -d -l $linesPerSplit -a 5 $source $TMPDIR/source.";
`$cmd`;

$cmd = "$splitCmd -d -l $linesPerSplit -a 5 $align $TMPDIR/align.";
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
		my $cmd = "$extractCmd $TMPDIR/target.$numStr $TMPDIR/source.$numStr $TMPDIR/align.$numStr $TMPDIR/extract.$numStr $extractArgs \n";
		print $cmd;
		`$cmd`;
		
		$cmd = "LC_ALL=C $sortCmd -T $TMPDIR $TMPDIR/extract.$numStr > $TMPDIR/extract.$numStr.sorted \n";
		print $cmd;
		`$cmd`;
		
		$cmd = "LC_ALL=C $sortCmd -T $TMPDIR $TMPDIR/extract.$numStr.inv > $TMPDIR/extract.$numStr.inv.sorted \n";
		print $cmd;
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
my $extractCmd = "LC_ALL=C $sortCmd -m ";
my $extractInvCmd = "LC_ALL=C $sortCmd -m ";
for (my $i = 0; $i < $numParallel; ++$i)
{
		my $numStr = NumStr($i);
		$extractCmd 		.= "$TMPDIR/extract.$numStr.sorted ";
		$extractInvCmd 	.= "$TMPDIR/extract.$numStr.inv.sorted ";
}

$extractCmd .= "> $extract.sorted \n";
$extractInvCmd .= "> $extract.inv.sorted \n";
print $extractCmd;
print $extractInvCmd;
`$extractCmd`;
`$extractInvCmd`;

$cmd = "rm -rf $TMPDIR";
`$cmd`;

print "Finished ".localtime() ."\n";


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