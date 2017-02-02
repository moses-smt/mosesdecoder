#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use Getopt::Long "GetOptions";
use FindBin qw($RealBin);

my ($EGRET_DIR,$MOSES_DIR,$TREE_CONVERTER,$FOREST,$SPLIT_HYPHEN,$SPLIT_SLASH,$MARK_SPLIT,$BINARIZE,$UNPARSEABLE,$RAW_IN,$RAW_OUT,$EGRET_OPTIONS,$TREE_CONVERTER_OPTIONS);

$UNPARSEABLE = 0;

die("ERROR: syntax is: parse-en-egret.perl [-forest] [-split-hyphen] [-split-slash] [-mark-split] [-binarize] [-unparseable] [-raw-in PATH] [-raw-out PATH] [-egret-options OPTIONS] [-tree-converter-options OPTIONS] -egret-dir DIR -moses-dir DIR -tree-converter PATH < in > out\n")
  unless &GetOptions
  ('egret-dir=s' => \$EGRET_DIR,
   'moses-dir=s' => \$MOSES_DIR,
   'tree-converter=s' => \$TREE_CONVERTER,
   'forest' => \$FOREST,
   'split-hyphen' => \$SPLIT_HYPHEN,
   'split-slash' => \$SPLIT_SLASH,
   'mark-split' => \$MARK_SPLIT,
   'binarize' => \$BINARIZE,
   'unparseable' => \$UNPARSEABLE,
   'raw-in=s' => \$RAW_IN,
   'raw-out=s' => \$RAW_OUT,
   'egret-options=s' => \$EGRET_OPTIONS,
   'tree-converter-options=s' => \$TREE_CONVERTER_OPTIONS
   )
  && defined($EGRET_DIR) && defined($MOSES_DIR) && defined($TREE_CONVERTER);

die("ERROR: could not find egret directory: '$EGRET_DIR'\n") unless -d $EGRET_DIR;
die("ERROR: could not find moses directory: '$MOSES_DIR'\n") unless -d $MOSES_DIR;
die("ERROR: file not found or not executable: '$TREE_CONVERTER'\n") unless -x $TREE_CONVERTER;

# Pre-processing.

my $tmpEscaped = "/tmp/parse-en-egret.1.$$";
my $tmpDeescaped = "/tmp/parse-en-egret.2.$$";
my $tmpSplitPoints = "/tmp/parse-en-egret.3.$$";

open(ESCAPED, ">>$tmpEscaped");
open(DEESCAPED, "| $RealBin/../../tokenizer/deescape-special-chars.perl > $tmpDeescaped");
open(SPLIT_POINTS, ">>$tmpSplitPoints");

# Unsplit hyphen and slashes and write a file indicating where split points
# are required in later post-processing.
while(<STDIN>) {
  print ESCAPED $_;
  my @tokens = split;
  my $new_token = "";
  my $i = 0;    # current token index in input sentence
  my $j = -1;   # current token index in output sentence
  my $s = "";   # output sentence
  my $t = "";   # split point line
  while ($i <= $#tokens) {
    if (defined($SPLIT_HYPHEN) && $i <= $#tokens-1 &&
        $tokens[$i] eq "\@\-\@") {
      my $pos = length $new_token;
      $new_token .= "-$tokens[$i+1]";
      $t .= "$j,$pos,- ";
      $i += 2;
    } elsif (defined($SPLIT_SLASH) && $i <= $#tokens-1 &&
             $tokens[$i] eq "\@\/\@") {
      my $pos = length $new_token;
      $new_token .= "/$tokens[$i+1]";
      $t .= "$j,$pos,/ ";
      $i += 2;
    } else {
      $s .= "$new_token ";
      $new_token = $tokens[$i];
      $i++;
      $j++;
    }
  }
  $s .= "$new_token";
  $s =~ s/^\s+//;
  $t =~ s/^\s+//;
  print DEESCAPED "$s\n";
  print SPLIT_POINTS "$t\n";
}

close(SPLIT_POINTS);
close(DEESCAPED);
close(ESCAPED);

# Construct the parsing / post-processing pipeline:

# Stage 1: Parse (unless the user has provided Egret input via -raw-in option).
my $pipeline = "";
if (defined($RAW_IN)) {
  $pipeline .= "cat \"$RAW_IN\" |";
} else {
  $pipeline .= "$EGRET_DIR/egret";
  $pipeline .= " -lapcfg";
  $pipeline .= " -data=$EGRET_DIR/eng_grammar";
  $pipeline .= " -printForest" if $FOREST;
  $pipeline .= " -i=$tmpDeescaped";
  $pipeline .= " $EGRET_OPTIONS" if defined($EGRET_OPTIONS);
  $pipeline .= " |";
}
if (defined($RAW_OUT)) {
  $pipeline .= "tee \"$RAW_OUT\" |";
}

# Stage 2: Convert trees to forests (unless we already have forests)
unless ($FOREST) {
  $pipeline .= 'sed \'s/^(//\' |';      # Remove opening (
  $pipeline .= 'sed \'s/)$//\' |';      # Remove closing )
  $pipeline .= "$TREE_CONVERTER";
  $pipeline .= " -input_format penn";
  $pipeline .= " -output_format egret";
  $pipeline .= " |";
}

# Stage 3: Postprocess using Moses' postprocess-egret-forests
# This performs some minor transformations to the forest: Moses-style escaping
# of special characters; removal of Egret's "^g" suffixes from constituent
# labels; and marking of slash/hyphen split points (using @ characters).
$pipeline .= "$MOSES_DIR/bin/postprocess-egret-forests";
$pipeline .= " --Escape" if $FOREST;
$pipeline .= " --MarkSplitPoints $tmpSplitPoints";
$pipeline .= " |";

# Stage 4: Postprocess using Travatar's tree-converter.
# This normalizes the forest weights and performs hyphen / slash splitting (if
# requested).  The option -tree-converter-options can be used to enable
# additional tree-converter transformations (such as binarization).
#my $output_format = $FOREST ? "egret" : "mosesxml";
my $output_format = $FOREST ? "egret" : "penn";
$pipeline .= "$TREE_CONVERTER";
$pipeline .= " -input_format egret";
$pipeline .= " -output_format $output_format";
# FIXME Single split option
$pipeline .= " -split \@\-\@" if defined($SPLIT_HYPHEN);
$pipeline .= " -split \@\/\@" if defined($SPLIT_SLASH);
$pipeline .= " $TREE_CONVERTER_OPTIONS" if defined($TREE_CONVERTER_OPTIONS);
$pipeline .= " |";

unless ($FOREST) {
  $pipeline .= 'sed \'s/^()$//\' |';    # Remove empty trees (failed parses)
  $pipeline .= 'sed \'s/^(/( (/\' |';   # Add Berkeley-style opening ( + blank
  $pipeline .= 'sed \'s/)$/))/\' |';    # Add Berkeley-style closing )
  $pipeline .= 'sed \'s/^$/(())/\' |';  # Restore empty trees (Berkeley-style)
  $pipeline .= "$RealBin/berkeleyparsed2mosesxml.perl |";
  $pipeline .= 'sed \'s/^<tree label="TOP"/<tree label="ROOT"/\' |';
}

# Run the parsing / post-processing pipeline.

open(PARSE, $pipeline);

if ($FOREST) {
  while (<PARSE>) {
    print $_;
  }
} else {
  open(TMPESCAPED, $tmpEscaped);
  while (<PARSE>) {
    my $outLine = $_;
    my $unparsedLine = <TMPESCAPED>;
    if ($UNPARSEABLE == 1 && length($outLine) == 1) {
      print $unparsedLine;
    } else {
      print $outLine;
    }
  }
}

close(PARSE);

`rm $tmpSplitPoints`;
`rm $tmpDeescaped`;
`rm $tmpEscaped`;
