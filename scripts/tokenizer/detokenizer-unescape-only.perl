#!/usr/bin/perl -w

# $Id: detokenizer.perl 4134 2011-08-08 15:30:54Z bgottesman $
# Sample De-Tokenizer
# written by Josh Schroeder, based on code by Philipp Koehn
# further modifications by Ondrej Bojar

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
use strict;
use utf8; # tell perl this script file is in UTF-8 (see all funny punct below)

while(my $line = <STDIN>) {
  chomp($line);

  # de-escape special chars
  $line =~ s/\&bar;/\|/g;   # factor separator (legacy)
  $line =~ s/\&#124;/\|/g;  # factor separator
  $line =~ s/\&lt;/\</g;    # xml
  $line =~ s/\&gt;/\>/g;    # xml
  $line =~ s/\&bra;/\[/g;   # syntax non-terminal (legacy)
  $line =~ s/\&ket;/\]/g;   # syntax non-terminal (legacy)
  $line =~ s/\&quot;/\"/g;  # xml
  $line =~ s/\&apos;/\'/g;  # xml
  $line =~ s/\&#91;/\[/g;   # syntax non-terminal
  $line =~ s/\&#93;/\]/g;   # syntax non-terminal
  $line =~ s/\&amp;/\&/g;   # escape escape

  print "$line\n";
}


