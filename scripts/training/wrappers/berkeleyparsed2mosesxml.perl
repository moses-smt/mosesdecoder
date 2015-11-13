#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;

while(<STDIN>) {
  if (/^\(\(\)\)/) {
    print "\n"; # parse failures
    next;
  }

  # prep
  s/^\( \( (.+) \)$/\(TOP $1/; # remove double wrapped parenthesis
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


  # output, replace words with original
  print $_;
}
