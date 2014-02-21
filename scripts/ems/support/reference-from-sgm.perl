#!/usr/bin/perl -w

use strict;

die("ERROR syntax: reference-from-sgm.perl ref src out") 
    unless scalar @ARGV == 3;
my ($ref,$src,$txt) = @ARGV;

# get order of the documents
my @ORDER;
open(ORDER,$src) || die("ERROR not found: $src");
while(<ORDER>) {
    next unless /docid="([^\"]+)"/;
    push @ORDER,$1;
}
close(ORDER);

# get from sgm file which lines belong to which system
my %DOC;
my $system_from_refset = 0;
my ($doc,$system);
open(REF,$ref) or die "Cannot open: $!";
while(my $line = <REF>) {
    if ($line =~ /<refset/ && $line =~ /refid="([^\"]+)"/i) {
      $system = $1;
      $system_from_refset = 1;
    }
    if ($line =~ /<doc/i) {
        die unless $line =~ /sysid="([^\"]+)"/i || $system_from_refset;
        $system = $1 unless $system_from_refset;
        die unless $line =~ /docid="([^\"]+)"/i;
        $doc = $1;
    }
    while ($line =~ /<seg[^>]+>\s*(.*)\s*$/i &&
	   $line !~ /<seg[^>]+>\s*(.*)\s*<\/seg>/i) {
	my $next_line = <REF>;
	$line .= $next_line;
	chop($line);
    }
    if ($line =~ /<seg[^>]+>\s*(.+)\s*<\/seg>/i) {
   	push @{$DOC{$system}{$doc}},$1;
    }
}
close(REF);

my $i=0;
foreach my $system (keys %DOC) {
    my $outfile = $txt;
    if (scalar keys %DOC > 1) {
	if ($outfile =~ /\.\d+$/) {
	    $outfile .= ".ref$i";
	}
	else {
	    $outfile .= $i;
	}
    }
    open(TXT,">$outfile") || die($outfile);
    foreach my $doc (@ORDER) {
	die("can't find '$doc' for ref '$system'") unless defined @{$DOC{$system}{$doc}};
	foreach my $line (@{$DOC{$system}{$doc}}) {
	    print TXT $line."\n";
	}
    }
    close(TXT);
    $i++;
}
