#!/usr/bin/perl

use utf8; 

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

while (my $line = <STDIN>) {
  chomp($line);
  #$line =~ tr/\040-\176/ /c;
  #$line =~ s/[^[:print:]]/ /g;
  #$line =~ s/\s+/ /g; 
  $line =~ s/\p{C}/ /g; 

  print "$line\n";
}

