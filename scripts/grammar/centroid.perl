#!/usr/bin/perl
use strict;
use Getopt::Long;
use File::Spec;

my $DIR = ".";
GetOptions(
  "d|directory=s" => \$DIR,  
);

my $i = 0;
my @files = map { s/\.\d\.\d.ini/.ini/; $_ } <$DIR/cross.*/work.err-cor/binmodel.err-cor/moses.mert.?.?.ini>;
open(INIS, "paste $DIR/cross.*/work.err-cor/binmodel.err-cor/moses.mert.?.?.ini |");
my @inis = <INIS>;
close(INIS);

my @names = qw(UnknownWordPenalty WordPenalty PhrasePenalty TranslationModel Distortion LM);
my $pattern = join("|", @names);

my $sparse = "";

my %inis;
my $next_is_weight_file = 0;
foreach my $FIELD (0 .. $#files) {
    my $content = "";
    foreach (@inis) {
        chomp;
        my @t = split(/\t/, $_);
        
        if ($t[0] =~ /^((${pattern})\d+=)/) {
            my $name = $1;
            my @weights = map { my ($name, @values) = split(/\s+/, $_); [@values] } @t;
            
            my @sums;
            foreach my $set (@weights) {
                foreach my $i (0 .. $#{$set}) {
                    $sums[$i] += $set->[$i];
                }
            }
            
            foreach my $sum (@sums) {
                $sum = $sum/@t;
            }
            
            my $values = join(" ", @sums);
            $content .=  "$name $values\n";    
        }
        else {
            if ($next_is_weight_file) {
                $content .= File::Spec->rel2abs($files[$FIELD]) . ".sparse\n";
                $sparse = average_sparse(@t) if not($sparse);
                $next_is_weight_file = 0;
            }
            else {
                $content .= $t[$FIELD] . "\n";
                if ($t[0] =~ /\[weight-file\]/) {
                    $next_is_weight_file = 1;
                }
            }
        }
        
    }
    $inis{$files[$FIELD]} = $content;
}

foreach my $ini (sort keys %inis) {
    open(INI, ">", $ini) or die "Could not create $ini\n";
    print INI $inis{$ini};
    close(INI);
    
    if ($sparse) {
        open(SPARSE, ">", "$ini.sparse");
        print SPARSE $sparse;
        close(SPARSE);
    }
    
}

sub average_sparse {
    my @weights = @_;
    my %weights;
    my %counts;
    
    foreach my $file (@weights) {
        open(W, "<", $file) or die "Could not open $file\n";
        while (<W>) {
           chomp;
           my ($f, $w) = split;
           $weights{$f} += $w;
           $counts{$f}++;
        }
        close(W);
    }
    
    foreach (keys %weights) {
        $weights{$_} /= $counts{$_};
    }
    
    return join("\n", map { "$_ $weights{$_}" } sort keys %weights);
}

