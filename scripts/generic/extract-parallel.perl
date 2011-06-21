#! /usr/bin/perl

# do not use. not yet complete
use strict;

sub NumStr($);

my $extractCmd	= $ARGV[0];
my $target			= $ARGV[1];
my $source			= $ARGV[2];
my $align				= $ARGV[3];
my $extractArgs	= $ARGV[4];
my $numParallel	= $ARGV[5];
my $splitCmd		= $ARGV[6];
my $sortCmd			= $ARGV[7];

my $TMPDIR="./tmp";
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
		
		$cmd = "LC_ALL=C sort -T $TMPDIR $TMPDIR/extract.$numStr > $TMPDIR/extract.$numStr.sorted \n";
		print $cmd;
		`$cmd`;
		
		$cmd = "LC_ALL=C sort -T $TMPDIR $TMPDIR/extract.$numStr.inv > $TMPDIR/extract.$numStr.inv.sorted \n";
		print $cmd;
		`$cmd`;
		
		last;
	}
	else
	{ # parent
		push(@childs, $pid);	
	}
}

print "second\n";

if ($isParent)
{
  foreach (@childs) {
    waitpid($_, 0);
  }
}

print "third\n";


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