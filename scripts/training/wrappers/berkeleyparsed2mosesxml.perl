#!/usr/bin/perl -w

use strict;

while(<STDIN>) {
  if (/^\(\(\)\)/) {
    print "\n"; # parse failures
    next;
  }

  # prep
  s/^\( /\(TOP /;

  # escape words
  s/\&/\&amp;/g;   # escape escape
  s/\|/\&bar;/g;   # factor separator
  s/\</\&lt;/g;    # xml
  s/\>/\&gt;/g;    # xml
  s/\'/\&apos;/g;  # xml
  s/\"/\&quot;/g;  # xml
  s/\[/\&#91;/g;   # syntax non-terminal
  s/\]/\&#93;/g;   # syntax non-terminal
  
  # convert into tree
  s/\((\S+) /<tree label=\"$1\"> /g;
  s/\)/ <\/tree> /g;
  s/\"\-LRB\-\"/\"LRB\"/g; # labels
  s/\"\-RRB\-\"/\"RRB\"/g;
  s/\-LRB\-/\(/g; # tokens
  s/\-RRB\-/\)/g;
  s/ +/ /g;
  s/ $//g;

  # output, replace words with original
  print $_;
}
