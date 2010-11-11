#!/usr/bin/perl -w

use strict;
use Getopt::Long;

my $excludedLinesFile = undef;

GetOptions(
    "excl=s" => \$excludedLinesFile
	   ) or exit(1);

open(EXCLUDED_LINE, $excludedLinesFile) or die("Can't open excluded file $excludedLinesFile");
my @excludedLines = <EXCLUDED_LINE>;

my $ln = undef;
foreach $ln (@excludedLines)
{
#    print "hello $ln\n";
}
close(EXCLUDED_LINE);

# MAIN LOOP
my $lineNum = 1;

#open(STDIN);
while(<STDIN>)
{
    my $line = $_;
    
    if 
  
    $lineNum++;
}
#close(STDIN);

print STDERR "num of lines=".($lineNum-1)."\n";


sub trim($)
{
    my $string = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}
