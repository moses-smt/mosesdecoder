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
my ($doc,$system);
open(REF,$ref);
while(<REF>) {
    if (/<doc/i) {
        die unless /sysid="([^\"]+)"/i;
	$system = $1;
        die unless /docid="([^\"]+)"/i;
        $doc = $1;
    }
    elsif (/<seg[^>]+>\s*(.+)\s*<\/seg>/i) {
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
	die($doc) unless defined @{$DOC{$system}{$doc}};
	foreach my $line (@{$DOC{$system}{$doc}}) {
	    print TXT $line."\n";
	}
    }
    close(TXT);
    $i++;
}
