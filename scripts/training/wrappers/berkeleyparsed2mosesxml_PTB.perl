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
  s/\|/\&#124;/g;  # factor separator
  s/\</\&lt;/g;    # xml
  s/\>/\&gt;/g;    # xml
  s/\'\'/\&quot;/g;
  s/``/\&quot;/g;
  s/\'/\&apos;/g;  # xml
  s/\"/\&quot;/g;  # xml
  s/\[/\&#91;/g;   # syntax non-terminal
  s/\]/\&#93;/g;   # syntax non-terminal


  # escape parentheses that were part of the input text
  s/(\(\S+ )\(\)/$1\&openingparenthesis;\)/g;
  s/(\(\S+ )\)\)/$1\&closingparenthesis;\)/g;


  
  # convert into tree
  s/\((\S+) /<tree label=\"$1\"> /g;
  s/\)/ <\/tree> /g;
  s/\"\-LRB\-\"/\"LRB\"/g; # labels
  s/\"\-RRB\-\"/\"RRB\"/g;
  s/\-LRB\-/\(/g; # tokens
  s/\-RRB\-/\)/g;
  s/ +/ /g;
  s/ $//g;
  
  # de-escape parentheses that were part of the input text
  s/\&openingparenthesis;/\(/g;
  s/\&closingparenthesis;/\)/g;

  s/tree label=\"\&quot;\"/tree label=\"QUOT\"/g;
  #s/tree label=\"''\"/tree label=\"QUOT\"/g;
  #s/tree label=\"``\"/tree label=\"QUOT\"/g;

  # output, replace words with original
  print $_;
}
