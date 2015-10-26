#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use strict;
use warnings;

use autodie;
use FindBin qw($RealBin);
use Getopt::Long "GetOptions";

my ($BLLIP_DIR,
    $BLLIP_MODEL_DIR,
    $SPLIT_HYPHEN,
    $SPLIT_SLASH,
    $MARK_SPLIT,
    $BINARIZE,
    $UNPARSEABLE,
    $RAW_IN,
    $RAW_OUT);

$UNPARSEABLE = 0;

die("ERROR: syntax is: parse-en-bllip.perl [-split-hyphen] [-split-slash] [-mark-split] [-binarize] [-unparseable] [-raw-in PATH] [-raw-out PATH] -bllip-dir PATH -bllip-model-dir PATH < in > out\n")
  unless &GetOptions
  ('bllip-dir=s' => \$BLLIP_DIR,
   'bllip-model-dir=s' => \$BLLIP_MODEL_DIR,
   'split-hyphen' => \$SPLIT_HYPHEN,
   'split-slash' => \$SPLIT_SLASH,
   'mark-split' => \$MARK_SPLIT,
   'binarize' => \$BINARIZE,
   'unparseable' => \$UNPARSEABLE,
   'raw-in=s' => \$RAW_IN,
   'raw-out=s' => \$RAW_OUT
   )
  && defined($BLLIP_DIR) && defined($BLLIP_MODEL_DIR);

die("ERROR: could not find BLLIP directory: '$BLLIP_DIR'\n")
  unless -d $BLLIP_DIR;

die("ERROR: could not find BLLIP model directory: '$BLLIP_MODEL_DIR'\n")
  unless -d $BLLIP_MODEL_DIR;

# Step 1: Read standard input and write two temporary files:
#
#     $tmpOriginal    Contains a copy of the input as-is
#
#     $tmpProcessed   Contains a copy of the input after pre-processing ready
#                     for input to the parser

my $tmpOriginal = "/tmp/parse-en-bllip.1.$$";
my $tmpProcessed = "/tmp/parse-en-bllip.2.$$";

open(TMP_ORIGINAL, ">$tmpOriginal");

open(TMP_PROCESSED,
     "| $RealBin/../../tokenizer/deescape-special-chars.perl > $tmpProcessed;");

while(<STDIN>) {
  print TMP_ORIGINAL $_;

  chomp;

  # Unsplit hyphens.
  s/ \@-\@ /-/g if $SPLIT_HYPHEN;
  # Unsplit slashes.
  s/ \@\/\@ /\//g if $SPLIT_SLASH;

  print TMP_PROCESSED "<s> $_ </s>\n";
}

close(TMP_ORIGINAL);
close(TMP_PROCESSED);

# Step 2: Parse $tmpProcessed then pass the raw output through a post-processing
#         pipeline.

my $pipeline = "";

# Stage 1: Parse input (unless given pre-parsed input via -raw-in option).
if (defined($RAW_IN)) {
  $pipeline .= "cat \"$RAW_IN\" |";
} else {
  $pipeline .= "cat $tmpProcessed |";
  $pipeline .= " $BLLIP_DIR/first-stage/PARSE/parseIt";
  $pipeline .= "  -K -l399 -N50 $BLLIP_MODEL_DIR/parser |";
  $pipeline .= " $BLLIP_DIR/second-stage/programs/features/best-parses";
  $pipeline .= " -l $BLLIP_MODEL_DIR/reranker/features.gz";
  $pipeline .= " $BLLIP_MODEL_DIR/reranker/weights.gz";
  $pipeline .= " |";
}

if (defined($RAW_OUT)) {
  $pipeline .= " tee \"$RAW_OUT\" |";
}

# Stage 2: Convert BLLIP output to Moses XML (via Berkeley output format)
$pipeline .= " sed -e 's/(S1 \\(.*\\)/(TOP \\1/' |";
$pipeline .= " $RealBin/berkeleyparsed2mosesxml.perl |";

# Stage 3: Re-split hyphens / slashes.
if ($SPLIT_HYPHEN) {
  $pipeline .= " $RealBin/syntax-hyphen-splitting.perl";
  $pipeline .= " -binarize" if $BINARIZE;
  $pipeline .= " -mark-split" if $MARK_SPLIT;
  $pipeline .= " |";
}
if ($SPLIT_SLASH) {
  $pipeline .= " $RealBin/syntax-hyphen-splitting.perl -slash";
  $pipeline .= " -binarize" if $BINARIZE;
  $pipeline .= " -mark-split" if $MARK_SPLIT;
  $pipeline .= " |";
}

# Run the parsing + post-processing pipeline.
open(PARSE, $pipeline);
open(TMP_ORIGINAL, $tmpOriginal);
while (<PARSE>) {
  my $parsedLine = $_;
  my $originalLine = <TMP_ORIGINAL>;
  if ($UNPARSEABLE == 1 && length($parsedLine) == 1) {
    print $originalLine;
  } else {
    print $parsedLine;
  }
}
close(PARSE);

`rm $tmpOriginal`;
`rm $tmpProcessed`;
