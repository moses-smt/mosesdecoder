#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;

  my $file = shift(@ARGV);
  open(MYFILE, $file);
 my @words;
 my $factor_f;
 my $factor_e;
 my $sentence;

  while (<MYFILE>) {
 	chomp;
 	#print "$_\n";

	$sentence = "$_";
	@words = split(/ /, $sentence);

	foreach (@words)
	{
 		 my ($factor_f,$factor_e) = split(/\-/,"$_");
		 print $factor_e . " " . $factor_f . " ";
 	}

	print "\n";
 }
 close (MYFILE);
