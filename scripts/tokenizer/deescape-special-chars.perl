#!/usr/bin/perl -w

use strict;

while(<STDIN>) {
  s/\&bar;/\|/g;   # factor separator (legacy)
  s/\&#124;/\|/g;  # factor separator
  s/\&lt;/\</g;    # xml
  s/\&gt;/\>/g;    # xml
  s/\&bra;/\[/g;   # syntax non-terminal (legacy)
  s/\&ket;/\]/g;   # syntax non-terminal (legacy)
  s/\&quot;/\"/g;  # xml
  s/\&apos;/\'/g;  # xml
  s/\&#91;/\[/g;   # syntax non-terminal
  s/\&#93;/\]/g;   # syntax non-terminal
  s/\&amp;/\&/g;   # escape escape
  print $_;
}
