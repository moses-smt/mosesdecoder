#!/usr/bin/perl

use strict;

# split -a 5 -d  ../europarl.clean.5.de 
# ls -1 x????? | ~/workspace/coreutils/parallel/src/parallel /home/s0565741/workspace/treetagger/cmd/run-tagger-chunker-german.sh 
# cat x?????.out > ../out

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

my $chunkedPath = $ARGV[1];

my $TMPDIR= "/tmp/chunker.$$";
print STDERR "TMPDIR=$TMPDIR\n";
`mkdir $TMPDIR`;

my $inPath = "$TMPDIR/in";

open(IN, ">$inPath");

while(my $line = <STDIN>) {
  chomp($line);
  print IN "$line\n";
}
close(IN);

# convert chunked file into Moses XML
open(CHUNKED, "$chunkedPath");
open(IN, "$inPath");

my $sentence = <STDIN>
chomp($sentence);
my @words = split(/ /, $sentence);
my $numWords = scalar @words;

while(my $chunkLine = <CHUNKED>) {
  chomp($chunkLine);

}

close(IN);
close(CHUNKED);

`rmdir -rf $TMPDIR`;

