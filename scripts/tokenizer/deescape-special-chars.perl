#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;

while (@ARGV) {
    $_ = shift;
    /^-b$/ && ($| = 1, next); # not buffered (flush each line)
}

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
