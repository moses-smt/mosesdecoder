#!/usr/bin/perl -w

use strict;
use Getopt::Long;

my $excludedLinesFile = undef;

GetOptions(
    "excl=s" => \$excludedLinesFile
	   ) or exit(1);

open(EXCLUDED_LINE, $excludedLinesFile) or die("Can't open excluded file $excludedLinesFile");
my @excludedLines = <EXCLUDED_LINE>;
close(EXCLUDED_LINE);

# MAIN LOOP
my $lineNum = 1;
my $linesOut = 0;
my $exclInd = 0;
my $nextLineExcl = -1;

if (@excludedLines > 0)
{
	$nextLineExcl = $excludedLines[0];
}

#open(STDIN);
while(<STDIN>)
{
    my $line = $_;
		    
    if ($nextLineExcl == $lineNum)
    {
    	$exclInd++;
    	if ($exclInd < @excludedLines) 
    	{
	    	$nextLineExcl = $excludedLines[$exclInd];
	    }
    }
    else
    {
    	print $line;
	$linesOut++;
    }
  
    $lineNum++;
}
#close(STDIN);

print STDERR "num of lines=".($lineNum-1)."\n";
print STDERR "num excluded=".(@excludedLines)."\n";
print STDERR "lines out=".($linesOut)."\n";

sub trim($)
{
    my $string = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}
