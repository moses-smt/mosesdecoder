#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;

my ($type) = @ARGV;
if ($type =~ /^s/i) {
	print "<srcset setid=\"test\" srclang=\"any\">\n";
	print "<doc docid=\"doc\">\n";
}
elsif ($type =~ /^t/i) {
	print "<tstset setid=\"test\" trglang=\"any\" srclang=\"any\">\n";
	print "<doc sysid=\"moses\" docid=\"doc\">\n";
}
elsif ($type =~ /^r/i) {
	print "<refset setid=\"test\" trglang=\"any\" srclang=\"any\">\n";
	print "<doc sysid=\"ref\" docid=\"doc\">\n";
}
else {
	die("ERROR: specify source / target / ref");
}

my $i = 0;
while(<STDIN>) {
  chomp;
  print "<seg id=\"".(++$i)."\">$_</seg>\n";
}

print "</doc>\n";

if ($type =~ /^s/i) {
	print "</srcset>\n";
}
elsif ($type =~ /^t/i) {
	print "</tstset>\n";	
}
elsif ($type =~ /^r/i) {
	print "</refset>\n";
}
