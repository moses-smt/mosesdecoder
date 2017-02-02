#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

#input hindi word urdu word, delete all those entries that have number on any side
use warnings;
use utf8;

use Getopt::Std;
use IO::Handle;

binmode(STDIN,  ':utf8');
binmode(STDOUT, ':utf8');
binmode(STDERR, ':utf8');
use open qw(:std :utf8);

$srcHash = ();
$trgHash = ();

$file = $ARGV[0];

@f0 = split(/\//, $file); # if file name has a path
@f1 = split(/\./, $f0[$#f0]); # last element would be the file name
@f2 = split(/\-/, $f1[1]);
$srcMark = $f2[0];
$trgMark = $f2[1];

$lang = 0;
$lang1 = 1;
$lang2 = 1;

if ($srcMark eq "en" || $srcMark eq "de"  || $srcMark eq "es"  ||  $srcMark eq "fr"  ||  $srcMark eq "it"  ||  $srcMark eq "nl"  ||  $srcMark eq "pt-br"  ||  $srcMark eq "ro"  ||  $srcMark eq "sl"  ||  $srcMark eq "tr" )
{
	print STDERR "Source is Latin\n";
	$lang1 = 0;
	$lang = $lang + 1;

}

if  ( "$trgMark" eq "en" || "$trgMark" eq "de" || "$trgMark" eq "es" || "$trgMark" eq "fr" || "$trgMark" eq "it" || "$trgMark" eq "nl" || "$trgMark" eq "pt-br" || "$trgMark" eq "ro" || "$trgMark" eq "sl" || "$trgMark" eq "tr" )
{
	print STDERR "Target is Latin\n";
	$lang2 = 0;
	$lang = $lang + 1;
}

if ("$lang" == 2)
{
	print STDERR "No Transliteration Module Possible\n";
}
else
{	print STDERR "will run Transliteration module\n";
	print STDERR "Three preprocessing steps to do:\n 1) Delete Symbol \t 2) Delete Latin from non-Latin langauge \t 3) Character Frequency based filtering\n";
	print STDERR "STARTING 1 and 2 ...\n";
	open ($IN, $ARGV[0]);
	while(<$IN>)
	{
		chomp;
		$retur = deleteSymbol($_);
		if($retur == 1)
		{
			#print "$_\n";
			$retur = deleteEnglish($lang1, $lang2, $_);
			if ($retur == 1)
			{
				#print "$_\n";
				push (@inputArr, $_);
				charFreqFilterPreprocess($_);
			}
		}
	}
	close ($IN);
}
print STDERR "DONE 1 and 2\nSTARTING 3) Preprocessing for Character filtering...\n";

charFreqFilterPreprocess2();
print STDERR "DONE 3\n";

foreach (@inputArr)
{
	charFreqFilter($_);
}

###############################Delete English##################################

sub deleteEnglish{
	@list = @_;
	$backEng = 0;

	if($list[0] == 1 && $list[1] == 1)
	{
#		print "Both are Non-Latin\n";
		if (m/[A-Za-z]/) {}
		else {$backEng = 1; return $backEng;}
	}
	elsif($list[0] == 0 && $list[1] == 1)
	{
#		print "Target is Non-Latin\n";
		@F=split("\t");
		if ($F[1] =~ m/[A-Za-z]/) {}
		else {$backEng = 1; return $backEng;}

	}
	elsif($list[0] == 1 && $list[1] == 0)
	{
#		print "Source is Non-Latin\n";
		@F=split("\t");
		if ($F[0] =~ m/[A-Za-z]/) {}
		else {$backEng = 1; return $backEng;}
	}
}
###############################Delete Symbol##################################
sub deleteSymbol{
	$back = 0;
	if (/\d+/) {}
	elsif(/\?/) {}
	elsif(/\!/) {}
	elsif(/@/) {}
	elsif(/\./) {}
	elsif(/\#/) {}
	elsif(/\%/) {}
	elsif(/\$/) {}
	elsif(/-/) {}
	elsif(/"/) {}
	elsif(/\(/) {}
	elsif(/\)/) {}
	elsif(/\&/) {}
	elsif(/\;/) {}
	elsif(/\\/) {}
	elsif(/\*/) {}
	elsif(/\+/) {}
	elsif(/\,/) {}
	elsif(/\</){}
	elsif(/\>/){}
	else
	{
		@wrds = split(/\t/);
		if($wrds[0] eq $wrds[1])
		{}
		elsif(length $wrds[0] < 3 )
		{}
		elsif(length $wrds[1] < 3)
		{}
		else
		{
			$back = 1;
			return $back;
#			print "$_\n";
		}
	}
}
#################################Char Frequency Filter Preprocess########################
sub charFreqFilterPreprocess{

	@wrds = split(/\t/);
	$srcWrd = lc $wrds[0];
	$trgWrd = lc $wrds[1];

	if($srcWrd eq $trgWrd)
	{}
	else
	{
		@src = split('',$srcWrd);
		foreach (@src)
		{
			if(exists $srcHash{$_})
			{
				$srcHash{$_}++;
			}
			else
			{
				$srcHash{$_} = 0;
			}
		}
		@trg = split('',$trgWrd);
		foreach (@trg)
		{
			if(exists $trgHash{$_})
			{
				$trgHash{$_}++;
			}
			else
			{
				$trgHash{$_} = 0;
			}
		}
	}
}
##############################Preprocess Two#############################
sub charFreqFilterPreprocess2{

###################srchash###################################

@keys = sort { $srcHash{$b} <=> $srcHash{$a} } keys %srcHash;

$bestsrcfreq = $srcHash{$keys[0]};
$srcOnePer = $bestsrcfreq * 0.005;

$take = 0; # take top 30 character from hash

foreach (@keys)
	{
#		print "$srcHash{$_}\t$_\n";

		if($take < 30)
		{
			$srcChar{$_} = 1;
#			print "$srcHash{$_}\t$_\n";

		}
		else
		{ ################# take worst characters that are not 1% of the best character################
			if($srcHash{$_} < $srcOnePer || $take > 50)
			{
				$srcBadChar{$_} = 1;
			}
		}
#		print "$_\t$srcHash{$_}\n";
		$take++;
	}

################### target hash ###################################

@keys = sort { $trgHash{$b} <=> $trgHash{$a} } keys %trgHash;

$besttrgfreq = $trgHash{$keys[0]};
$trgOnePer = $besttrgfreq * 0.005;

#print "$besttrgfreq\t$trgOnePer\n";

$take = 0; # take top 30 character from hash
foreach (@keys)
	{
		if($take < 30)
		{
			$trgChar{$_} = 1;
		}
		else
		{ ################# take worst characters that are not 1% of the best character################
			if($trgHash{$_} < $trgOnePer || $take > 50 )
			{
				$trgBadChar{$_} = 1;
			}
		}
#		print "$_\t$trgHash{$_}\n";
		$take++;
	}
}

###############################CharFreqFiltering###################################
sub charFreqFilter{
	@in = @_;
	@wrds = split(/\t/, $in[0]);
	$srcWrd = lc $wrds[0];
	$trgWrd = lc $wrds[1];

	@srcWrdArr = split("",$srcWrd);
	@trgWrdArr = split("",$trgWrd);


	$check = 0;
	$remove = 0;

########################## search if word contain any of the bad characters ####################################

	foreach (@srcWrdArr)
	{
#		print "$srcWrd\n";
		if (exists $srcBadChar{$_}) # if this character is in the list of worst characters
		{
			$remove = 1;
#			print "#######EXIT src: \t$srcWrd##########\n";
			last;
		}
	}

	if($remove == 1)
	{}
	else
	{	foreach (@trgWrdArr)
		{
			if (exists $trgBadChar{$_}) # if this character is in the list of worst characters
			{
				$remove = 1;
			#	print "EXIT target: \t$trgWrd\n";
				last;
			}
		}
	}
########################## search if word contain any of the good characters ####################################
   if($remove == 1)
   {}
   else
   {
	foreach (@srcWrdArr)
	{
		if(exists ($srcChar{$_}))
		{
			$check = 1;
			last;
		}
	}

	if($check == 1)
	{
		foreach (@trgWrdArr)
		{
			if(exists ($trgChar{$_}))
			{
#				print "$wrds[0]\t$wrds[1]\n";
				$printSrc = join (" ", split("",$wrds[0]));
				$printTrg = join (" ", split("",$wrds[1]));
				print "$printSrc\n$printTrg\n";
				last;
			}
		}
	}
  }
}
