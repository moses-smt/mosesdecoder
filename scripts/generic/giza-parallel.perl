#! /usr/bin/perl

# example
# ~/giza-parallel.perl 10 split ~/workspace/sourceforge/trunk/scripts/training/train-model.perl ar en train align

use strict;
use File::Basename;

sub NumStr($);

print "Started ".localtime() ."\n";

my $numParallel		= $ARGV[0];
my $splitCmd		= $ARGV[1];
my $trainCmd		= $ARGV[2];
my $inputExt		= $ARGV[3];
my $outputExt		= $ARGV[4];
my $corpus			= $ARGV[5];
my $align			= $ARGV[6];

my $TMPDIR=dirname($align)  ."/tmp.$$";
mkdir $TMPDIR;

my $scriptDir=dirname($trainCmd) ."/..";

# split corpus file
my $totalLines = int(`wc -l $corpus.$inputExt`);
my $linesPerSplit = int($totalLines / $numParallel) + 1;

my $cmd = "$splitCmd -d -l $linesPerSplit -a 5 $corpus.$inputExt $TMPDIR/source.";
`$cmd`;

$cmd = "$splitCmd -d -l $linesPerSplit -a 5 $corpus.$outputExt $TMPDIR/target.";
`$cmd`;

for (my $i = 0; $i < $numParallel; ++$i)
{
  my $numStr = NumStr($i);
  rename("$TMPDIR/source.$numStr", "$TMPDIR/$numStr.source");
  rename("$TMPDIR/target.$numStr", "$TMPDIR/$numStr.target");
}

#fork & run giza & friends
my $isParent = 1;
my @childs;
for (my $i = 0; $i < $numParallel; ++$i)
{
  my $pid = fork();
	
	if ($pid == 0)
	{ # child
	  $isParent = 0;

    my $numStr = NumStr($i);
    my $cmd = "$trainCmd -dont-zip -last-step 1 -scripts-root-dir $scriptDir -f source -e target -alignment grow-diag-final-and -parts 3 -reordering msd-bidirectional-fe -corpus $TMPDIR/$numStr -corpus-dir $TMPDIR/prepared.$numStr \n";
    print $cmd;
    `$cmd`;

    $cmd = "$trainCmd -dont-zip -first-step 2 -last-step 2 -scripts-root-dir $scriptDir -f source -e target -alignment grow-diag-final-and -parts 3 -reordering msd-bidirectional-fe -corpus-dir $TMPDIR/prepared.$numStr -giza-e2f $TMPDIR/giza.$numStr -direction 2 \n";
    print $cmd;
    `$cmd`;

    $cmd = "$trainCmd -dont-zip -first-step 2 -last-step 2 -scripts-root-dir $scriptDir -f source -e target -alignment grow-diag-final-and -parts 3 -reordering msd-bidirectional-fe -corpus-dir $TMPDIR/prepared.$numStr -giza-f2e $TMPDIR/giza-inverse.$numStr -direction 1 \n";
    print $cmd;
    `$cmd`;

    $cmd = "$trainCmd -dont-zip -first-step 3 -last-step 3 -scripts-root-dir $scriptDir -f source -e target -alignment grow-diag-final-and -parts 3 -reordering msd-bidirectional-fe -giza-e2f $TMPDIR/giza.$numStr -giza-f2e $TMPDIR/giza-inverse.$numStr -alignment-file $TMPDIR/aligned.$numStr -alignment grow-diag-final-and \n";
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

# cat all aligned files together. Voila
my $cmd = "cat ";
for (my $i = 0; $i < $numParallel; ++$i)
{
		my $numStr = NumStr($i);
		$cmd 		.= "$TMPDIR/aligned.$numStr.grow-diag-final-and ";
}
$cmd .= " > $align \n";
print $cmd;
`$cmd`;

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

