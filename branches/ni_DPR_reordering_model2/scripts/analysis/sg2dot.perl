#!/usr/bin/perl -w
# 
# Author : Loic BARRAULT
# Script to convert MOSES searchgraph to DOT format
#

use strict;
use File::Path;
use File::Basename;
use File::Copy;
use Getopt::Long;

my $organize_to_stacks = 0;
GetOptions(
  "organize-to-stacks" => \$organize_to_stacks,
) or exit 1;

my %stacks = ();
$stacks{0}{0} = 0;

print STDOUT "digraph searchgraph\n{\nrankdir=LR\n";


my($line, $cpt, $from, $to, $label, $recombined, $transition, $o, $stack, $state);
$cpt = 0;
 
$line=<>; #skip first line (the empty hypothesis, no arc in fact)

my $nr = 0;
while(($line=<>) ) 
{
        $nr++;
	$from = "";
	$to = "";
	$label = "";
	$recombined = "";
	chomp($line);
	#print STDERR "$line\n";
	
	#Three kinds of lines in searchgraph
	#0 hyp=0 stack=0 forward=1 fscore=-205.192
	#0 hyp=5 stack=1 back=0 score=-0.53862 transition=-0.53862 forward=181 fscore=-205.36 covered=0-0 out=I am , pC=-0.401291, c=-0.98555
	#256 hyp=6566 stack=2 back=23 score=-2.15644 transition=-0.921959 recombined=6302 forward=15519 fscore=-112.807 covered=2-2 out=countries , , pC=-0.640574, c=-1.07215

	if($line =~ /hyp=(\d+).+stack=(\d+).+back=(\d+).+transition=([^ ]*).+recombined=(\d+).+out=(.*)(, pC|$)/)
	{ 
		#print STDERR "hyp=$1, stack=$2, from=$3, transition=$4, recombined=$5, out=$6\n";
		$to = $1;
		$stack = $2;
		$from = $3;
		$transition=$4;
		$recombined = $5;
		$o = $6;	
		$label = "[color=blue label=";
		
		$to = $recombined;
		$stacks{$stack}{$recombined} = $recombined if $organize_to_stacks;
		#$stack++;
		#$stacks{$stack}{$recombined} = $recombined;
	}
	elsif($line =~ /hyp=(\d+).+stack=(\d+).+back=(\d+).+transition=([^ ]*).+out=(.*)(, pC|$)/)
	{
		#print STDERR "hyp=$1, stack=$2, from=$3, transition=$4, out=$5\n";
		$to = $1;
		$stack = $2;
		$from = $3;
		$transition=$4;
		$o = $5;
		$label = "[label=";
		$stacks{$stack}{$to} = $to if $organize_to_stacks;
		#$stack++;
		#$stacks{$stack}{$to} = $to;
	}
	else{
          die "$nr:Bad line: $line\n";
        }

	$o =~ s/\"/\\"/g	;
	#print STDERR "out = $o after regexp\n";
	$label .= "\"$o  p=$transition\"]\n";
	#$label .= " p=$transition\"]\n";
	
	print STDOUT "$from -> $to $label";
		
	$cpt++;
}

if ($organize_to_stacks) {
	foreach $stack (sort (keys(%stacks)))
	{
		print STDOUT "{ rank=same; ";
		foreach $state (sort keys %{ $stacks{$stack} } )
		{
			print STDOUT "$stacks{$stack}{$state} ";
		}
		print STDOUT "}\n";
	}
}

print STDOUT "\n}\n";

print STDERR "Converted $nr lines.\n";
