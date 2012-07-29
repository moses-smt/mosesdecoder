#! /usr/bin/perl -w 

# example
# last arg: parse, use hiero if specified
# psd-feature-extract-parallel.perl 8 /home/ales/moses/bin/extract-psd extract.psd.gz corpus.src.gz phrase-table.gz config.psd /home/ales/model/psd [parse]

use strict;
use File::Basename;

sub RunFork($);
sub systemCheck($);
sub GetSourcePhrase($);
sub NumStr($);

print "Started ".localtime() ."\n";

my ($cores, $extractor, $psd_extract, $src_corpus, $phrase_table, $psd_config, $out, $parse) = @ARGV;
die "Error in args. Usage: psd-feature-extract-parallel.perl cores extractor psd-extract src-corpus phrase-table psd-config output-prefix [parse]"
  if ! defined $out;

$cores = 1 if $cores < 1;

my $TMPDIR= dirname($out)."/tmp.$$";
my $mkdir_cmd = "mkdir -p $TMPDIR";
`$mkdir_cmd`;

my $fileCount = 0;
if ($cores <= 1)
{ # don't do parallel. Just link the psdextract file into place
  my $cmd = "ln -s $psd_extract $TMPDIR/psdextract.0.gz";
  print STDERR "$cmd \n";
  systemCheck($cmd);
  
  $fileCount = 1;
}
else
{
  # count the number of lines first
  my $totalLines;
	if ($psd_extract =~ /\.gz$/) {
    $totalLines = `zcat $psd_extract | wc -l`;
	}	else {
    $totalLines = `wc -l < $psd_extract`;
	}

  my $linesPerFile = int($totalLines / $cores);

  # cut up psdextract file into smaller mini-psdextract files.
	if ($psd_extract =~ /\.gz$/) {
		open(IN, "gunzip -c $psd_extract |") || die "can't open pipe to $psd_extract";
	}
	else {
		open(IN, $psd_extract) || die "can't open $psd_extract";
	}
	
	my $filePath  = "$TMPDIR/psdextract.$fileCount.gz";
	open (OUT, "| gzip -c > $filePath") or die "error starting gzip $!";
	
	my $lineCount = 0;
	my $line;
	while ($line=<IN>) 
	{
		chomp($line);
  	print OUT "$line\n";
		++$lineCount;
	
    if ($lineCount > $linesPerFile) {
      close OUT;
      $lineCount = 0;
      ++$fileCount;
      $filePath = "$TMPDIR/psdextract.$fileCount.gz";
      open (OUT, "| gzip -c > $filePath") or die "error starting gzip $!";
    }
  }
	close OUT;
	++$fileCount;
}

if ($fileCount != $cores) {
  die "error: Number of files $fileCount different from number of processes $cores";
}

# create run scripts
my @runFiles = (0..($cores-1));
for (my $i = 0; $i < $cores; ++$i)
{
  my $path = "$TMPDIR/run.$i.sh";
  open(my $fh, ">", $path) or die "cannot open $path: $!";
  $runFiles[$i] = $fh;
}

# write scoring of mini-extracts to run scripts
for (my $i = 0; $i < $cores; ++$i)
{
  my $numStr = NumStr($i);

  my $fh = $runFiles[$i];
  my $hieroarg = "";
  $hieroarg = " $parse " if defined $parse;
  my $cmd = "$extractor $TMPDIR/psdextract.$i.gz $src_corpus $hieroarg $phrase_table $psd_config $TMPDIR/train.$i $TMPDIR/index.$i";
  print $fh $cmd;
}

# close run script files
for (my $i = 0; $i < $cores; ++$i)
{
  close($runFiles[$i]);
  my $path = "$TMPDIR/run.$i.sh";
  systemCheck("chmod +x $path");
}

# run each score script in parallel
my @children;
for (my $i = 0; $i < $cores; ++$i)
{
  my $cmd = "$TMPDIR/run.$i.sh\n";
	my $pid = RunFork($cmd);
	push(@children, $pid);
}

# wait for children to finish
foreach (@children) {
	waitpid($_, 0);
}

systemCheck("mv $TMPDIR/index.0 $out.index");
my $catargs = join(" ", map { "$TMPDIR/train.$_" } (0 .. $cores - 1));
systemCheck("cat $catargs | gzip -c > $out.train.gz");

my $cmd = "rm -rf $TMPDIR";
print STDERR "WARNING: skipping $cmd\n";
#print STDERR $cmd, "\n";
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
