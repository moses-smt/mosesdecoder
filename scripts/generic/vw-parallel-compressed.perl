#! /usr/bin/perl -w 

# example
# 1st arg: num_cores
# 2nd arg: training file (built by extract-psd)
# 3rd arg: cache prefix (used with vw --cache_file)
# 4th arg: model output file (used with vw -f)
# 5th arg: path to VW
# vw-parallel.perl 8 train_file vw_cache vw_model /home/fraser/src/vowpal_wabbit/ --passes 10 --csoaa_ldf m --hash all --noconstant -b 22 -q st


use strict;
use File::Basename;

sub RunFork($);
sub systemCheck($);
sub GetSourcePhrase($);
sub NumStr($);

print "Started ".localtime() ."\n";

my ($numParallel, $trainFile, $cacheFile, $modelFile, $vwPath, @args) = @ARGV;
die "Error in args. Usage: vw-parallel.perl cores train_file cache_prefix model_file vw_path vw_args" if ! defined $vwPath;

my $vwCmd = "$vwPath/bin/vw " . join(" ", @args);

$numParallel = 1 if $numParallel < 1;

my $TMPDIR="tmp.$$";
mkdir $TMPDIR;

my $fileCount = 0;

# AMF changed this to run VW single-threaded without a span_server if cores=1 and then exit
if ($numParallel <= 1)
{ 
		### AMF - outdated
    ## don't do parallel. Just link the train file into place
		#my $cmd = "ln -s $trainFile $TMPDIR/train.0.gz";
		#$fileCount = 1;

	my $cmd = "zcat $trainFile | $vwCmd --cache_file $TMPDIR/$cacheFile -f $modelFile --save_per_pass";
  print STDERR "$cmd\n";
  systemCheck($cmd);
  
  $cmd = "rm -rf $TMPDIR";
	print STDERR "WARNING, SKIPPING: $cmd\n";
  #systemCheck($cmd);

	print STDERR "Finished ".localtime() ."\n";
	exit(0);
}
### else not necessary, but whatever...
else
{
  # count the number of lines first
  my $totalLines;
	if ($trainFile =~ /\.gz$/) {
    $totalLines = `zcat $trainFile | wc -l`;
	}	else {
    $totalLines = `wc -l < $trainFile`;
	}

  my $linesPerFile = int($totalLines / $numParallel);

  # cut up train file into smaller mini-train files.
	if ($trainFile =~ /\.gz$/) {
		open(IN, "gunzip -c $trainFile |") || die "can't open pipe to $trainFile";
	}
	else {
		open(IN, $trainFile) || die "can't open $trainFile";
	}
	
	my $filePath  = "$TMPDIR/train.$fileCount.gz";
	open (OUT, "| gzip -c > $filePath") or die "error starting gzip $!";
	
	my $lineCount = 0;
	my $line;
	while ($line=<IN>) 
	{
		chomp($line);
  	print OUT "$line\n";
		++$lineCount;
	
    if ($lineCount > $linesPerFile && $line eq "") {
      close OUT;
      $lineCount = 0;
      ++$fileCount;
      $filePath = "$TMPDIR/train.$fileCount.gz";
      open (OUT, "| gzip -c > $filePath") or die "error starting gzip $!";
    }
  }
	close OUT;
	++$fileCount;
}

if ($fileCount != $numParallel) {
  die "error: Number of files $fileCount different from number of processes $numParallel";
}

# create run scripts
my @runFiles = (0..($numParallel-1));
for (my $i = 0; $i < $numParallel; ++$i)
{
  my $path = "$TMPDIR/run.$i.sh";
  open(my $fh, ">", $path) or die "cannot open $path: $!";
  $runFiles[$i] = $fh;
}

# write scoring of mini-extracts to run scripts
for (my $i = 0; $i < $numParallel; ++$i)
{
  my $numStr = NumStr($i);

  my $fh = $runFiles[$i];
  my $cmd;
	if ($i == 0) {
			$cmd = "zcat $TMPDIR/train.$i.gz | $vwCmd --cache_file $TMPDIR/$cacheFile.$numStr -f $modelFile  --compressed --node $i --total $numParallel --span_server localhost --unique_id $$ --save_per_pass";
			# pass through the output from the first piece
			$cmd .= " |& tee > $TMPDIR/run.$i.sh.log";
	}
	else {
			$cmd = "zcat $TMPDIR/train.$i.gz | $vwCmd --cache_file $TMPDIR/$cacheFile.$numStr -f $modelFile.$numStr --compressed --node $i --total $numParallel --span_server localhost --unique_id $$ --save_per_pass";
			# save the output of the other pieces
			$cmd .= " >& $TMPDIR/run.$i.sh.log";
	}
  print $fh $cmd, "\n";
}

# close run script files
for (my $i = 0; $i < $numParallel; ++$i)
{
  close($runFiles[$i]);
  my $path = "$TMPDIR/run.$i.sh";
  systemCheck("chmod +x $path");
}

# start VW master process
my $masterPid = RunFork("$vwPath/cluster/spanning_tree");

# run each score script in parallel
my @children;
for (my $i = 0; $i < $numParallel; ++$i)
{
  my $cmd = "$TMPDIR/run.$i.sh";
	my $pid = RunFork($cmd);
	push(@children, $pid);
}

# wait for children to finish
foreach (@children) {
	waitpid($_, 0);
}

kill 1, $masterPid;

#my $cmd = "rm -rf $TMPDIR $modelFile.*";
my $cmd = "rm -rf $TMPDIR";
#print STDERR "WARNING, SKIPPING: $cmd\n";
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
    print STDERR $cmd, "\n";
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
