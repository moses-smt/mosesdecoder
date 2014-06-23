#! /usr/bin/perl -w 

use strict;

sub Write1Line;
sub WriteCorpus1Holdout;

my $iniPath = $ARGV[0];
my $isHiero = $ARGV[1];
my $decoderExec = $ARGV[2];
my $extractExec = $ARGV[3];
my $tmpName = $ARGV[4];
my $startLine = $ARGV[5];
my $endLine = $ARGV[6];

print STDERR "iniPath=$iniPath \n isHiero=$isHiero \n decoderExec=$decoderExec \n extractExec=$extractExec \n";

my $WORK_DIR = `pwd`;
chomp($WORK_DIR);

my $MOSES_DIR = "~/workspace/github/mosesdecoder.hieu.gna";

$decoderExec = "$MOSES_DIR/bin/$decoderExec";
$extractExec = "$MOSES_DIR/bin/$extractExec";

my $SPLIT_EXEC = `gsplit --help 2>/dev/null`; 
if($SPLIT_EXEC) {
    $SPLIT_EXEC = 'gsplit';
}
else {
    $SPLIT_EXEC = 'split';
}

my $SORT_EXEC = `gsort --help 2>/dev/null`; 
if($SORT_EXEC) {
    $SORT_EXEC = 'gsort';
}
else {
    $SORT_EXEC = 'sort';
}


my $hieroFlag = "";
if ($isHiero == 1) {
  $hieroFlag = "--Hierarchical";
}

print STDERR "WORK_DIR=$WORK_DIR \n";

my $cmd;

open (SOURCE, "source");
open (TARGET, "target");
open (ALIGNMENT, "alignment");

my $numLines = `cat source | wc -l`;

for (my $lineNum = 0; $lineNum < $numLines; ++$lineNum) {
    my $source = <SOURCE>; chomp($source);
    my $target = <TARGET>; chomp($target);
    my  $alignment = <ALIGNMENT>; chomp($alignment);
  
    if ($lineNum < $startLine || $lineNum >= $endLine) {
	next;
    }

    #print STDERR  "$source ||| $target ||| $alignment \n";
    # write out 1 line
    my $tmpDir = "$WORK_DIR/$tmpName/work$lineNum";
    `mkdir -p $tmpDir`;

    Write1Line($source, $tmpDir, "source.1");
    Write1Line($target, $tmpDir, "target.1");
    Write1Line($alignment, $tmpDir, "alignment.1");

    WriteCorpus1Holdout($lineNum, "source", $tmpDir, "source.corpus");
    WriteCorpus1Holdout($lineNum, "target", $tmpDir, "target.corpus");
    WriteCorpus1Holdout($lineNum, "alignment", $tmpDir, "alignment.corpus");

  # train
    if ($isHiero == 1) {
	$cmd = "$extractExec $tmpDir/target.corpus $tmpDir/source.corpus $tmpDir/alignment.corpus $tmpDir/extract --GZOutput";
    }
    else {
	# pb
	$cmd = "$extractExec $tmpDir/target.corpus $tmpDir/source.corpus $tmpDir/alignment.corpus $tmpDir/extract 7 --GZOutput";
    }
    $cmd = "$MOSES_DIR/scripts/generic/extract-parallel.perl 1 $SPLIT_EXEC $SORT_EXEC $cmd";
    print STDERR "Executing: $cmd\n";
    `$cmd`;

    $cmd = "$MOSES_DIR/scripts/generic/score-parallel.perl 1 $SORT_EXEC $MOSES_DIR/bin/score $tmpDir/extract.sorted.gz /dev/null $tmpDir/pt.half.gz $hieroFlag --NoLex 1";
    `$cmd`;

    $cmd = "$MOSES_DIR/scripts/generic/score-parallel.perl 1 $SORT_EXEC $MOSES_DIR/bin/score $tmpDir/extract.inv.sorted.gz /dev/null $tmpDir/pt.half.inv.gz --Inverse $hieroFlag --NoLex 1";
    `$cmd`;

    $cmd = "$MOSES_DIR/bin/consolidate $tmpDir/pt.half.gz $tmpDir/pt.half.inv.gz $tmpDir/pt $hieroFlag --OnlyDirect";
    `$cmd`;
    
  # decode
    $cmd = "$decoderExec -f $iniPath -feature-overwrite \"TranslationModel0 path=$tmpDir/pt\" -i $tmpDir/source.1 -n-best-list $tmpDir/nbest 10000 distinct -v 2";
    print STDERR "Executing: $cmd\n";
    `$cmd`;

    # count the number of translation in nbest list
    $cmd = "wc -l $tmpDir/nbest >> out";
    `$cmd`;

  `rm -rf $tmpDir`;
}

close(SOURCE);
close(TARGET);
close(ALIGNMENT);


######################
sub Write1Line
{
  my ($line, $tmpDir, $fileName) = @_;
  
  open (HANDLE, ">$tmpDir/$fileName");
  print HANDLE "$line\n";
  close (HANDLE);
}

sub WriteCorpus1Holdout
{
  my ($holdoutLineNum, $inFilePath, $tmpDir, $outFileName) = @_;

  open (INFILE, "$inFilePath");
  open (OUTFILE, ">$tmpDir/$outFileName");

  my $lineNum = 0;
  while (my $line = <INFILE>) {
    chomp($line);

    if ($lineNum != $holdoutLineNum) {
      print OUTFILE "$line\n";
    }

    ++$lineNum;
  }

  close (OUTFILE);
  close(INFILE);

}


