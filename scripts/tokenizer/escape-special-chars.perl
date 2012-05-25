#!/usr/bin/perl -w

use strict;

while(<STDIN>) {
  chop;

  # avoid general madness
  s/[\000-\037]//g;
  s/\s+/ /g;
	s/^ //g;
	s/ $//g;

  # special characters in moses
  s/\&/\&amp;/g;
  s/\|/\&bar;/g;
  s/\</\&lt;/g;
  s/\>/\&gt;/g;
  s/\[/\&#91;/g;
  s/\]/\&#93;/g;
  
  # restore xml instructions
  s/\&lt;(\S+) translation="([^\"]+)"&gt; (.+?) &lt;\/(\S+)&gt;/\<$1 translation=\"$2\"> $3 <\/$4>/g;
  print $_."\n";
}
