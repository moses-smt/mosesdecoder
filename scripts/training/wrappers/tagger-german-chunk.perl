#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use Getopt::Long "GetOptions";

# split -a 5 -d  ../europarl.clean.5.de
# ls -1 x????? | ~/workspace/coreutils/parallel/src/parallel /home/s0565741/workspace/treetagger/cmd/run-tagger-chunker-german.sh
# cat x?????.out > ../out

my $chunkedPath;
my $treetaggerPath;

GetOptions('chunked=s' => \$chunkedPath,
          'tree-tagger=s' => \$treetaggerPath);

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

#my $TMPDIR= "/tmp/chunker.$$";
my $TMPDIR= "chunker.$$";
print STDERR "TMPDIR=$TMPDIR\n";
print STDERR "chunkedPath=$chunkedPath\n";
`mkdir $TMPDIR`;

my $inPath = "$TMPDIR/in";

open(IN, ">$inPath");
binmode(IN, ":utf8");

while(my $line = <STDIN>) {
  chomp($line);
  print IN "$line\n";
}
close(IN);

# call chunker
if (!defined($chunkedPath)) {
  if (!defined($treetaggerPath)) {
    print STDERR "must defined -tree-tagger \n";
    exit(1);
  }

  $chunkedPath = "$TMPDIR/chunked";
  print STDERR "chunkedPath not defined. Now $chunkedPath \n";
  my $cmd = "$treetaggerPath/cmd/tagger-chunker-german-utf8  < $inPath > $chunkedPath";
  `$cmd`;
}

# convert chunked file into Moses XML
open(CHUNKED, "$chunkedPath");
open(IN, "$inPath");
binmode(CHUNKED, ":utf8");
binmode(IN, ":utf8");

my $sentence = <IN>;
chomp($sentence);
my @words = split(/ /, $sentence);
my $numWords = scalar @words;
my $prevTag = "";
my $wordPos = -1;

while(my $chunkLine = <CHUNKED>) {
  chomp($chunkLine);
  my @chunkToks = split(/\t/, $chunkLine);

  if (substr($chunkLine, 0, 1) eq "<") {
    if (substr($chunkLine, 0, 2) eq "</") {
      # end of tag
      print "</tree> ";
      $prevTag = "";

  	  if ($wordPos == ($numWords - 1)) {
	    # closing bracket of last word in sentence
	    print "\n";
            $sentence = <IN>;
	    chomp($sentence);
	    @words = split(/ /, $sentence);
	    $numWords = scalar @words;
	    $wordPos = -1;
	  }
    }
    else {
      # beginning of tag
      if ($wordPos == ($numWords - 1)) {
        # closing bracket of last word in sentence
        print "\n";
        $sentence = <IN>;
        chomp($sentence);
        @words = split(/ /, $sentence);
        $numWords = scalar @words;
        $wordPos = -1;
      }

      $prevTag = $chunkToks[0];
      $prevTag = substr($prevTag, 1, length($prevTag) - 2);
      print "<tree label=\"$prevTag\">";
    }
  }
  else {
    # word
    ++$wordPos;

    if (scalar(@chunkToks) != 3) {
      # parse error
      print STDERR "CHUNK LINES SHOULD BE 3 TOKS\n";
      exit(1);
    }

    if ($wordPos >= $numWords) {
      # on new sentence now
      if (length($prevTag) > 0) {
        print "</tree>";
      }
      print "\n";
      if (length($prevTag) > 0) {
	  print "<tree label=\"$prevTag\">";
      }

      $sentence = <IN>;
      chomp($sentence);
      @words = split(/ /, $sentence);
      $numWords = scalar @words;
      $wordPos = 0;
    }

    if ($chunkToks[0] ne $words[$wordPos]) {
      # word in chunk input and sentence should match
      print STDERR "NOT EQUAL:" .$chunkToks[0] ." != " .$words[$wordPos] ."\n";
      exit(1);
    }

    print $chunkToks[0] . " ";

  }

}

print "\n";

close(IN);
close(CHUNKED);

`rm -rf $TMPDIR`;

