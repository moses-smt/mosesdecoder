#!/bin/sh

if [ $# -lt 6 ] 	
	then
  		echo "Usage: $0 <\"java options\"> <berkeleyaligner jar file> <input file stem> <output directory> <source lang> <target lang> [aligner options...]"
  		exit 1
fi

JAVA_OPTS=$1
JAR=$2
INFILE=$3
OUTDIR=$4
SLANG=$5
TLANG=$6
shift
shift
shift
shift
shift
shift

echo $INFILE > $INFILE.list
/usr/local/share/java/bin/java $JAVA_OPTS -jar $JAR -Data.trainSources $INFILE.list -exec.execDir $OUTDIR -Data.englishSuffix $SLANG -Data.foreignSuffix $TLANG -exec.create true -Main.SaveParams true -Main.alignTraining false -Data.testSources  $@ 
