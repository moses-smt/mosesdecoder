#! /usr/bin/perl -w 

use strict;

my $iniPath = $ARGV[0];
my $isHiero = $ARGV[1];
my $decoderExec = $ARGV[2];
my $extractExec = $ARGV[3];
my $tmpName = $ARGV[4];

my $WORK_DIR = `pwd`;
chomp($WORK_DIR);

my $MOSES_DIR = "~/workspace/github/mosesdecoder.hieu";

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

my $lineNum = 0;
my ($source, $target, $alignment);
while ($source = <SOURCE>) {
    chomp($source);
    $target = <TARGET>; chomp($target);
    $alignment = <ALIGNMENT>; chomp($alignment);
  
    #print STDERR  "$source ||| $target ||| $alignment \n";
  
  # write out 1 line
    my $tmpDir = "$WORK_DIR/$tmpName/work$lineNum";
    `mkdir -p $tmpDir`;
                  
    open (SOURCE1, ">$tmpDir/source");
    open (TARGET1, ">$tmpDir/target");
    open (ALIGNMENT1, ">$tmpDir/alignment");
  
    print SOURCE1 "$source\n";
    print TARGET1 "$target\n";
    print ALIGNMENT1 "$alignment\n";

    close (SOURCE1);
    close (TARGET1);
    close (ALIGNMENT1);

  # train
    if ($isHiero == 1) {
	$cmd = "$extractExec $tmpDir/target $tmpDir/source $tmpDir/alignment $tmpDir/extract --GZOutput";
    }
    else {
	# pb
	$cmd = "$extractExec $tmpDir/target $tmpDir/source $tmpDir/alignment $tmpDir/extract 7 --GZOutput";
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
    $cmd = "$decoderExec -f $iniPath -feature-overwrite \"TranslationModel0 path=$tmpDir/pt\" -i $tmpDir/source -feature-add \"ConstrainedDecoding path=$tmpDir/target\"";
    print STDERR "Executing: $cmd\n";
    `$cmd`;

#  `rm -rf $tmpDir`;

    ++$lineNum;
}

close(SOURCE);
close(TARGET);
close(ALIGNMENT);

