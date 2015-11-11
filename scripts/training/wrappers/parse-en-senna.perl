#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use strict;
use warnings;

use autodie;
use FindBin qw($RealBin);
use Getopt::Long "GetOptions";

my ($SENNA,
    $SENNA_DIR,
    $SENNA_OPTIONS,
    $SPLIT_HYPHEN,
    $SPLIT_SLASH,
    $MARK_SPLIT,
    $BINARIZE,
    $UNPARSEABLE,
    $RAW_IN,
    $RAW_OUT);

$UNPARSEABLE = 0;

die("ERROR: syntax is: parse-en-senna.perl [-senna-options OPTIONS] [-split-hyphen] [-split-slash] [-mark-split] [-binarize] [-unparseable] [-raw-in PATH] [-raw-out PATH] -senna PATH -senna-dir PATH < in > out\n")
  unless &GetOptions
  ('senna=s' => \$SENNA,
   'senna-dir=s' => \$SENNA_DIR,
   'senna-options=s' => \$SENNA_OPTIONS,
   'split-hyphen' => \$SPLIT_HYPHEN,
   'split-slash' => \$SPLIT_SLASH,
   'mark-split' => \$MARK_SPLIT,
   'binarize' => \$BINARIZE,
   'unparseable' => \$UNPARSEABLE,
   'raw-in=s' => \$RAW_IN,
   'raw-out=s' => \$RAW_OUT
   )
  && defined($SENNA);

die("ERROR: file not found or not executable: '$SENNA'\n") unless -x $SENNA;
die("ERROR: could not find SENNA directory: '$SENNA_DIR'\n") unless -d $SENNA_DIR;

# Step 1: Read standard input and write two temporary files:
#
#     $tmpOriginal    Contains a copy of the input as-is
#
#     $tmpProcessed   Contains a copy of the input after pre-processing ready
#                     for input to SENNA

my $tmpOriginal = "/tmp/parse-en-senna.1.$$";
my $tmpProcessed = "/tmp/parse-en-senna.2.$$";

open(TMP_ORIGINAL, ">$tmpOriginal");

open(TMP_PROCESSED,
     "| $RealBin/../../tokenizer/deescape-special-chars.perl > $tmpProcessed;");

while(<STDIN>) {
  print TMP_ORIGINAL $_;

  # If the line is longer than 1023 bytes (including the newline) then replace
  # it with "SENTENCE_TOO_LONG\n".  This is because SENNA reads lines into a
  # 1024 character array and if a line is longer than 1023 characters then it
  # gets read in stages and treated as multiple input lines.
  my $num_bytes;
  {
    use bytes;
    $num_bytes = length($_);
  }
  if ($num_bytes > 1023) {
    print TMP_PROCESSED "SENTENCE_TOO_LONG\n";
    next;
  }

  # Replace "-LRB-", "-RRB-", etc. with "(", ")", etc.
  s/-LRB-/(/g;
  s/-RRB-/)/g;
  s/-LSB-/[/g;
  s/-RSB-/]/g;
  s/-LCB-/{/g;
  s/-RCB-/}/g;

  # Unsplit hyphens.
  s/ \@-\@ /-/g if $SPLIT_HYPHEN;
  # Unsplit slashes.
  s/ \@\/\@ /\//g if $SPLIT_SLASH;

  print TMP_PROCESSED $_;
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
  my $path = $SENNA_DIR;
  # SENNA requires -path's argument to end with a slash.
  if ($path !~ /\/$/) {
    $path .= "/";
  }
  $pipeline .= " $SENNA -path $path -usrtokens";
  $pipeline .= " $SENNA_OPTIONS" if defined($SENNA_OPTIONS);
  $pipeline .= " |";
}

if (defined($RAW_OUT)) {
  $pipeline .= " tee \"$RAW_OUT\" |";
}

# Stage 2: Convert SENNA output to Moses XML (via Berkeley output format)
$pipeline .= " $RealBin/senna2brackets.py --berkeley-style |";
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
