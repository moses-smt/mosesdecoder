#!/bin/sh
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

if [ $# -lt 8 ]
	then
  		echo "Usage: $0 <\"java options\"> <berkeleyaligner jar file> <input file stem> <previous berkeley param dir> <output directory> <source lang> <target lang> <alignment name (i.e. 'berk' or 'low-posterior')> <posterior threshold> [aligner options...]"
  		exit 1
fi

JAVA_OPTS=$1
JAR=$2
INFILE=$3
PARAMDIR=$4
OUTNAME=$5
SLANG=$6
TLANG=$7
TAG=$8
POSTERIOR=$9
shift
shift
shift
shift
shift
shift
shift
shift
shift

JAVA_CMD="/usr/local/share/java/bin/java \
      $JAVA_OPTS -jar $JAR -Data.trainSources $INFILE.list \
      -Main.loadParamsDir $PARAMDIR -exec.execDir $OUTNAME \
      -Main.loadLexicalModelOnly false -Data.englishSuffix $SLANG \
      -Data.foreignSuffix $TLANG -exec.create true -Main.saveParams false \
      -Main.alignTraining true -Main.forwardModels HMM \
      -Main.reverseModels HMM -Main.mode JOINT -Main.iters 0 \
      -Data.testSources -EMWordAligner.posteriorDecodingThreshold $POSTERIOR \
      $@"
echo "Running $JAVA_CMD"
$JAVA_CMD

#clean up
rm $OUTNAME/training.*Input.txt
rm $OUTNAME/training.*Trees.txt
gzip $OUTNAME/training.$SLANG-$TLANG.A3
gzip $OUTNAME/training.$TLANG-$SLANG.A3

#now shift the output
perl -e "
use strict;
while (<STDIN>) {
  chomp();
  my @pairs = split(\" \");
  for (my \$i=0;\$i<scalar(@pairs);\$i++) {
    die (\"bad pair \$pairs[\$i]\n\") unless \$pairs[\$i] =~ /(\d+)\-(\d+)/;
    \$pairs[\$i] = (\$2).\"-\".(\$1);
  }
  print join(\" \",@pairs);
  print(\"\n\");
};" < $OUTNAME/training.$SLANG-$TLANG.align > $OUTNAME.$TAG

gzip $OUTNAME/training.$SLANG-$TLANG.align
