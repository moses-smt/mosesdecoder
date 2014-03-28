#!/usr/bin/perl 

use strict;

# split -a 5 -d  ../europarl.clean.5.de 
# ls -1 x????? | ~/workspace/coreutils/parallel/src/parallel /home/s0565741/workspace/treetagger/cmd/run-tagger-chunker-german.sh 
# cat x?????.out > ../out

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

my $chunkedPath = $ARGV[0];

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

# convert chunked file into Moses XML
open(CHUNKED, "$chunkedPath");
open(IN, "$inPath");

my $sentence = <IN>;
chomp($sentence);
my @words = split(/ /, $sentence);
my $numWords = scalar @words;
my $prevTag = "";
my $wordPos = 0;

while(my $chunkLine = <CHUNKED>) {
  chomp($chunkLine);
  my @chunkToks = split(/\t/, $chunkLine);

  if (substr($chunkLine, 0, 1) eq "<") {
    if (substr($chunkLine, 0, 2) eq "</") {
      # end of tag
      print "</tree> ";
      $prevTag = "";
    }
    else {
      # beginning of tag
      $prevTag = $chunkToks[0];
      $prevTag = substr($prevTag, 1, length($prevTag));
      print "<tree label=\"$prevTag\">";
    }
  }
  else {
    # word
    if (scalar(@chunkToks) != 3) {
      print STDERR "chunk lines should be 3 toks\n";
    }
    if ($chunkToks[0] ne $words[$wordPos]) {
      print STDERR "chunk word " .$chunkToks[0] ." not equal " .$words[$wordPos] ."\n";
    }
    print $chunkToks[0] . " ";
    ++$wordPos;

    if ($wordPos >= $numWords) {
      print "\n";
      $sentence = <IN>;
      chomp($sentence);
      @words = split(/ /, $sentence);
      $numWords = scalar @words;
      $wordPos = 0;
    }
  }

}

close(IN);
close(CHUNKED);

#`rm -rf $TMPDIR`;

