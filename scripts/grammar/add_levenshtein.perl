#!/usr/bin/perl

use strict;
use Text::LevenshteinXS qw(distance);

sub max {
    return $_[0] > $_[1] ? $_[0] : $_[1];
}

while (<STDIN>) {
    chomp;
    my @fields = split(/\s\|\|\|\s/);
    
    my ($source, $target) = @fields;
    my @source = split(/\s/, $source);
    my @target = split(/\s/, $target);
    
    my $srcShort = "";
    my $trgShort = "";
    
    my %map;
    for my $t (@source) {
        if(not exists($map{$t})) {
            $map{$t} = chr(65 + scalar keys %map);
        }
        $srcShort .= $map{$t};
    }
    
    for my $t (@target) {
        if(not exists($map{$t})) {
            $map{$t} = chr(65 + scalar keys %map);
        }
        $trgShort .= $map{$t};
    }
    
    
    my $d = distance($srcShort, $trgShort);
    #my $m = max(length($fields[0]), length($fields[1]));
     
    my $score = sprintf("%.4f", exp($d));
    $score += 0;
     
    #print $score, "\n";
    $fields[2] .= " $score";
     
    print join(" ||| ", @fields), "\n"
}
