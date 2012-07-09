#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";
use FindBin qw($RealBin);

my ($JAR,$GRAMMAR,$SPLIT_HYPHEN,$MARK_SPLIT,$BINARIZE);

die("ERROR: syntax is: parse-de-berkeley.perl [-split-hyphen] [-mark-split] [-binarize] -jar jar-file -gr grammar < in > out\n") 
  unless &GetOptions
  ('jar=s' => \$JAR,
   'gr=s' => \$GRAMMAR,
   'split-hyphen' => \$SPLIT_HYPHEN,
   'mark-split' => \$MARK_SPLIT,
   'binarize' => \$BINARIZE)
  && defined($JAR) && defined($GRAMMAR);

die("ERROR: could not find jar file '$JAR'\n") unless -e $JAR;
die("ERROR: could not find grammar file '$GRAMMAR'\n") unless -e $GRAMMAR;

$BINARIZE = $BINARIZE ? "-binarize" : "";
$SPLIT_HYPHEN = $SPLIT_HYPHEN ? "| $RealBin/syntax-hyphen-splitting.perl $BINARIZE" : "";
$SPLIT_HYPHEN .= " -mark-split" if $SPLIT_HYPHEN && $MARK_SPLIT;

my $tmp = "/tmp/parse-de-berkeley.$$";

open(TMP,"| $RealBin/../../tokenizer/deescape-special-chars.perl > $tmp");
while(<STDIN>) {
  # unsplit hyphens
  s/ \@-\@ /-/g if $SPLIT_HYPHEN;

  # handle parentheses
  s/\(/*LRB*/g;
  s/\)/*RRB*/g;

  print TMP $_;
}
close(TMP);

my $cmd = "cat $tmp | java -Xmx10000m -Xms10000m -Dfile.encoding=UTF8 -jar $JAR -gr $GRAMMAR -maxLength 1000 $BINARIZE | $RealBin/berkeleyparsed2mosesxml.perl $SPLIT_HYPHEN";
print STDERR $cmd."\n";

open(PARSE,"$cmd|");
while(<PARSE>) {
  print $_;
}
close(PARSE);
`rm $tmp`;
