#! /usr/bin/perl

use strict;
	
my $argv=join(" ",@ARGV);

$argv =~ s/ +/  /g;

my $newargv="";
while ($argv =~ s/(.*?\-[a-z][0-9a-z\-]* )(.*?)( \-+[a-z][0-9a-z\-]*?|$)/$3/i){my $tmp1=$1;; my $tmp2=$2; $tmp2=~ s/^ */"/; $tmp2 =~ s/ *$/"/; $tmp2 =~ s/ +/|/g; $newargv=$newargv." ".$tmp1.$tmp2; }

$newargv.=" $argv";
$argv=$newargv;
#$argv =~ s/\|/ /g;
$argv =~ s/\"//g;
print STDOUT "$argv\n";

