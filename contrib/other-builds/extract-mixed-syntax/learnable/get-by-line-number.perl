#! /usr/bin/perl -w 

use strict;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

my $fileLineNum = $ARGV[0];
open (FILE_LINE_NUM, $fileLineNum);

my $nextLineNum = <FILE_LINE_NUM>;

my $lineNum = 1;
while (my $line = <STDIN>) {
  if (defined($nextLineNum) && $lineNum == $nextLineNum) {
    # matches. output line
    chomp($line);
    print "$line\n";

    # next line number
    $nextLineNum = <FILE_LINE_NUM>;
  }

  ++$lineNum;
}



