#! /usr/bin/perl -w 

# example
# ./sigtest-parallel.pl 8 /home/fraser/src/stable/contrib/sigtest-filter/filter-pt "-l a+e -n 30" ./phrase-table-nonfiltered.1.gz /export/ws12/damt/src/SALM/Bin/Linux/Index/IndexSA.O32  /mnt/data/ws12/damt/experiments/fraser_PB_thousand_NEW_sigtest/training/corpus.1.fr /mnt/data/ws12/damt/experiments/fraser_PB_thousand_NEW_sigtest/training/corpus.1.en ./phrase-table.gz

use strict;
use File::Basename;

sub RunFork($);
sub systemCheck($);
sub GetSourcePhrase($);
sub NumStr($);

#FB : constant modified because of RAM consumption
my $SIGTEST_SPLIT_LINES = 125000;

print "Started ".localtime() ."\n";

my $numParallel	= shift;
die "$0 called incorrectly, error\n" unless defined($numParallel);
$numParallel = 1 if $numParallel < 1;
my $filterCmd = shift;
my $filterArgs = shift;
my $ptFile = shift;
my $indexCmd = shift;
my $fCorpus = shift;
my $eCorpus = shift;
my $outFile = shift;
die "$0 called incorrectly, error\n" unless defined($outFile);

# Currently hardcoded to take gzipped phrase table (and output gzipped too, see below)
if (-f $ptFile) {
    die "FATAL: error, expected $ptFile not to exist, and $ptFile.gz to exist";
}

$ptFile = "$ptFile.gz";
if (not -f "$ptFile") {
    die "FATAL: error, expected $ptFile to exist";
}

my $TMPDIR=dirname($ptFile)."/tmp.$$";
mkdir $TMPDIR;

my $cmd;

$cmd = "$indexCmd $fCorpus";
print STDERR "$cmd \n";
systemCheck($cmd);

$cmd = "$indexCmd $eCorpus";
print STDERR "$cmd \n";
systemCheck($cmd);

my $fileCount = 0;
if ($numParallel <= 1)
{ # don't do parallel. Just link the phrase file into place
  $cmd = "ln -s $ptFile $TMPDIR/unfiltered.0.gz";
  print STDERR "$cmd \n";
  systemCheck($cmd);
  
  $fileCount = 1;
}
else
{	# cut up phrase table into smaller mini-phrase tables
        my $totalLines;
	if ($ptFile =~ /\.gz$/) {
	    $totalLines = int(`zcat $ptFile | wc -l`);
	    open(IN, "gunzip -c $ptFile |") || die "can't open pipe to $ptFile";
	}
	else {
	    $totalLines = int(`cat $ptFile | wc -l`);
	    open(IN, $ptFile) || die "can't open $ptFile";
	}

	## This does not scale due to a RAM issue with filter-p, so using hardcoded upper limit of 2M lines
	my $linesPerSplit = int($totalLines/$numParallel)+1;
	$linesPerSplit = $SIGTEST_SPLIT_LINES if $SIGTEST_SPLIT_LINES < $linesPerSplit;

	my $filePath  = "$TMPDIR/unfiltered.$fileCount.gz";
	open (OUT, "| gzip -c > $filePath") or die "error starting gzip $!";
	
	my $lineCount = 0;
	my $line;
	my $prevSourcePhrase = "";
	while ($line=<IN>) 
	{
		chomp($line);
		++$lineCount;
	
		if ($lineCount > $linesPerSplit)
		{ # over line limit. Cut off at next source phrase change
			my $sourcePhrase = GetSourcePhrase($line);
			
			if ($prevSourcePhrase eq "")
			{ # start comparing
				$prevSourcePhrase = $sourcePhrase;
			}
			elsif ($sourcePhrase eq $prevSourcePhrase)
			{ # can't cut off yet. Do nothing      
			}
			else
			{ # cut off, open next min-unfiltered file & write to that instead
				close OUT;
	
				$prevSourcePhrase = "";
				$lineCount = 0;
				++$fileCount;
				my $filePath  = "$TMPDIR/unfiltered.$fileCount.gz";
				open (OUT, "| gzip -c > $filePath") or die "error starting gzip $!";
			}
		}
		else
		{ # keep on writing to current mini-unfiltered file
		}
	
		print OUT "$line\n";
	
	}
	close OUT;
	++$fileCount;
}


# create run scripts
my @runFiles = (0..($numParallel-1));
for (my $i = 0; $i < $numParallel; ++$i)
{
  my $path = "$TMPDIR/run.$i.sh";
  open(my $fh, ">", $path) or die "cannot open $path: $!";
  $runFiles[$i] = $fh;
}

# write filtering of unfiltered mini-phrasetables to run scripts
for (my $i = 0; $i < $fileCount; ++$i)
{
  my $numStr = NumStr($i);

  my $fileInd = $i % $numParallel;
  my $fh = $runFiles[$fileInd];
  my $cmd = "zcat $TMPDIR/unfiltered.$i.gz | $filterCmd $filterArgs -f $fCorpus -e $eCorpus | gzip -c > $TMPDIR/filtered.$i.gz\n";
  print $fh $cmd;
}

# close run script files
for (my $i = 0; $i < $numParallel; ++$i)
{
  close($runFiles[$i]);
  my $path = "$TMPDIR/run.$i.sh";
  systemCheck("chmod +x $path");
}

# run each score script in parallel
my @children;
for (my $i = 0; $i < $numParallel; ++$i)
{
  my $cmd = "$TMPDIR/run.$i.sh";
	my $pid = RunFork($cmd);
	push(@children, $pid);
}

# wait for everything is finished
foreach (@children) {
	waitpid($_, 0);
}

# merge
$cmd = "zcat ";
for (my $i = 0; $i < $fileCount; ++$i)
{
  $cmd .= "$TMPDIR/filtered.$i.gz ";
}
$cmd .= "| gzip -c > $outFile.gz";
print STDERR $cmd;
systemCheck($cmd);

$cmd = "rm -rf $TMPDIR \n";
print STDERR "SKIPPED: ", $cmd;
#systemCheck($cmd);

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

sub GetSourcePhrase($)
{
  my $line = shift;
  my $pos = index($line, "|||");
  my $sourcePhrase = substr($line, 0, $pos);
  return $sourcePhrase;
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


