#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use Getopt::Long "GetOptions";
use FindBin qw($RealBin);

my ($JAR,$GRAMMAR,$SPLIT_HYPHEN,$SPLIT_SLASH,$MARK_SPLIT,$BINARIZE,$UNPARSEABLE);

$UNPARSEABLE = 0;

die("ERROR: syntax is: parse-de-berkeley.perl [-split-hyphen] [-split-slash] [-mark-split] [-binarize] -jar jar-file -gr grammar -unparseable < in > out\n")
  unless &GetOptions
  ('jar=s' => \$JAR,
   'gr=s' => \$GRAMMAR,
   'split-hyphen' => \$SPLIT_HYPHEN,
   'split-slash' => \$SPLIT_SLASH,
   'mark-split' => \$MARK_SPLIT,
   'binarize' => \$BINARIZE,
   'unparseable' => \$UNPARSEABLE

   )
  && defined($JAR) && defined($GRAMMAR);

#print STDERR "UNPARSEABLE=$UNPARSEABLE\n";

die("ERROR: could not find jar file '$JAR'\n") unless -e $JAR;
die("ERROR: could not find grammar file '$GRAMMAR'\n") unless -e $GRAMMAR;

$BINARIZE = $BINARIZE ? "-binarize" : "";
$SPLIT_HYPHEN = $SPLIT_HYPHEN ? "| $RealBin/syntax-hyphen-splitting.perl $BINARIZE" : "";
$SPLIT_HYPHEN .= " -mark-split" if $SPLIT_HYPHEN && $MARK_SPLIT;
$SPLIT_SLASH = $SPLIT_SLASH ? "| $RealBin/syntax-hyphen-splitting.perl -slash $BINARIZE" : "";
$SPLIT_SLASH .= " -mark-split" if $SPLIT_SLASH && $MARK_SPLIT;

my $tmp = "/tmp/parse-de-berkeley.$$";
my $tmpEscaped = "/tmp/parse-de-berkeley.2.$$";
#print STDERR "tmp=$tmp\n";
#print STDERR "tmpEscaped=$tmpEscaped\n";

open(TMP,"| $RealBin/../../tokenizer/deescape-special-chars.perl > $tmp");
open(TMPESCAPED, ">>$tmpEscaped");
while(<STDIN>) {
  print TMPESCAPED $_;

  # unsplit hyphens
  s/ \@-\@ /-/g if $SPLIT_HYPHEN;
  # unsplit slashes
  s/ \@\/\@ /\//g if $SPLIT_SLASH;

  # handle parentheses
  s/\(/*LRB*/g;
  s/\)/*RRB*/g;

  # handle @ (the parser does something weird with these)
  s/\@/\\\@/g;

  print TMP $_;
}
close(TMP);
close(TMPESCAPED);

my $cmd = "cat $tmp | java -Xmx10000m -Xms10000m -Dfile.encoding=UTF8 -jar $JAR -gr $GRAMMAR -maxLength 1000 $BINARIZE | $RealBin/berkeleyparsed2mosesxml.perl $SPLIT_HYPHEN $SPLIT_SLASH";
#print STDERR "Executing: $cmd \n";

open (TMP, $tmp);
open (TMPESCAPED, $tmpEscaped);

open(PARSE,"$cmd|");
while(<PARSE>) {
  s/\\\@/\@/g;
  my $outLine = $_;
  my $unparsedLine = <TMPESCAPED>;

  #print STDERR "unparsedLine=$unparsedLine";
  #print STDERR "outLine=$outLine" .length($outLine) ."\n";

  if ($UNPARSEABLE == 1 && length($outLine) == 1) {
	  print $unparsedLine;
  }
  else {
	  print $outLine;
  }
}
close(PARSE);
`rm $tmp`;
`rm $tmpEscaped`;
