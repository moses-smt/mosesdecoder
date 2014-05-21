#!/usr/bin/perl

use strict;
use Data::Dumper;
use Getopt::Long;

use FindBin qw($Bin);
use lib "$Bin";

use Algorithm::Diff::XS;
use Text::Soundex;

my $INSTANCES;
my $WITH_CATEGORIES=0;
GetOptions(
    "i|instances=s" => \$INSTANCES,
    "c|with-categories" => \$WITH_CATEGORIES,
);

sub max {
    return $_[0] > $_[1] ? $_[0] : $_[1];
}

$Text::Soundex::nocode = 'Z000';

sub soundex2 {
    my $str = shift;
    return $str if (length($str) <= 2);
    my $code = soundex($str);
    return $code eq "Z000" ? $str : $code; 
} 

my $INSTANCES_FILE;
if ($INSTANCES) {
    open($INSTANCES_FILE, ">", $INSTANCES) or die "Could not open $INSTANCES\n";
}

my %PATTERN;
my %CATEGORY;

while (<STDIN>) {
    chomp;
    my ($s, $t, $category) = split(/\t/, $_);
    
    $s =~ s/^\s+|\s+$//g;
    $t =~ s/^\s+|\s+$//g;
    
    my @stoks = split(/\s+/, $s);
    my @ttoks = split(/\s+/, $t);
    
    print STDERR "[$.]\n" if ($. % 10000 == 0); 
    
    my $same = 0;
    my @changes;
    
    my $diff = Algorithm::Diff::XS->new( \@stoks, \@ttoks, { keyGen => \&soundex2 } );
    while($diff->Next()) {
        if($diff->Same()) {
            my ($a, $b) = $diff->Get(qw( Range1 Range2 ));
            
            my @w = $diff->Same();
            my @a = $diff->Range(1);
            my @b = $diff->Range(2);
            
            my ($err, $cor);
            foreach my $i (0 .. $#w) {
                if($stoks[$a[$i]] eq $ttoks[$b[$i]]) {
                    $err = $cor = $ttoks[$b[$i]];
                    $same++;
                }
                else {
                    $err = $stoks[$a[$i]];
                    $cor = $ttoks[$b[$i]];
                }
                push(@changes, [$err, $cor]);
            }
        }
        elsif(not $diff->Range(1)) {
            my $words = join(" ", $diff->Items(2));            
            push(@changes, [undef, $words]); 
        }
        elsif(not $diff->Range(2))  {
            my $words = join(" ", $diff->Items(1));            
            push(@changes, [$words, undef]); 
        }
        else {
            my $words1 = join(" ", $diff->Items(1));
            my $words2 = join(" ", $diff->Items(2));
            push(@changes, [$words1, $words2]);  
        }        

        if ($WITH_CATEGORIES) {
            push($changes[-1], $category);
        }
    }
    
    my $noChange = 1;
    foreach (@changes) {
        my ($a, $b, $c) = @$_;
        if ($a ne $b) {
            $noChange = 0;
            my $pattern = make_pattern($a, $b);
            $PATTERN{$pattern}++;

            if ($WITH_CATEGORIES) {
                $CATEGORY{$pattern}=$c;
            }
        }
        else {
            $PATTERN{"MATCH\tMATCH"}++;
        }
    }
    if ($noChange) {
        $PATTERN{"NULL\tNULL"}++;
    }
    
}

foreach my $p (sort { $PATTERN{$b} <=> $PATTERN{$a} } keys %PATTERN) {
    if ($WITH_CATEGORIES) {
        print "$p\t$PATTERN{$p}\t$CATEGORY{$p}\n";
    }
    else {
        print "$p\t$PATTERN{$p}\n";
    }
}

sub make_pattern {
    my ($a, $b) = @_;
    
    my $pattern;
    my $patternPrint;
    if ($a eq $b) {
        $pattern = '^' . quotemeta($a) . '$';
        $patternPrint = $a;
    }
    elsif(not $a and $b) {
        $pattern = '^' . quotemeta("NULL -> $b") . '$';
        $patternPrint = "ins(«${b}»)";
        $patternPrint =~ s/\s/·/g;
    }
    elsif($a and not $b) {
        $pattern = '^' . quotemeta("$a -> NULL") . '$';
        $patternPrint = "del(«${a}»)";
        $patternPrint =~ s/\s/·/g;
    }
    elsif($a and $b) {
        my @a = split(//, $a);
        my @b = split(//, $b);
        
        my $diff = Algorithm::Diff::XS->new( \@a, \@b );
        
        my @left;
        my @leftPrint;
        
        my @right;
        my @rightPrint;
        
        my $same = 0;
        while($diff->Next()) {
            if($diff->Same()) {
                my @w = map { quotemeta($_) } $diff->Same();
                if (@w >= 3) {
                    $same++;
                    push(@left, '(\w{3,})');
                    push(@right, "\\$same");
                    
                    push(@leftPrint, '(\w{3,})');
                    push(@rightPrint, "\\$same");
                }
                else {
                    push(@left, @w);                
                    push(@right, @w);                    

                    push(@leftPrint, $diff->Same());
                    push(@rightPrint, $diff->Same());
                }
            }
            elsif(not $diff->Range(1)) {
                push(@right, map { quotemeta($_) } $diff->Items(2));
                push(@rightPrint, $diff->Items(2));
            }
            elsif(not $diff->Range(2))  {
                push(@left, map { quotemeta($_) } $diff->Items(1));
                push(@leftPrint, $diff->Items(1));
            }
            else {
                push(@left, map { quotemeta($_) } $diff->Items(1));                
                push(@right, map { quotemeta($_) } $diff->Items(2));

                push(@leftPrint, $diff->Items(1));                
                push(@rightPrint, $diff->Items(2));
            }    
        }
        
        $pattern = '^' . join("", @left) . quotemeta(" -> ") . join("", @right) . '$';
        $patternPrint = "sub(«" . join("", @leftPrint) . "»,«" . join("", @rightPrint) . "»)";
        $patternPrint =~ s/\s/·/g;
    }
    
    if ($INSTANCES) {
        print $INSTANCES_FILE "$a\t$b\n";    
    } 
    
    return "$pattern\t$patternPrint";
}
