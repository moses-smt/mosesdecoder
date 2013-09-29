#! /usr/bin/perl
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
