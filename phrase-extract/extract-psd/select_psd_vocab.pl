#!/usr/bin/perl -w
use strict;

if (@ARGV != 3){
    print STDERR "$0\n";
    print STDERR "select French and English phrases from phrase-table for PSD modeling\n";
    print STDERR "  - French phrases: all phrases from phrase-table, except those that start or end with numbers, punctuation or stopwords\n";
    print STDERR "  - English phrases: all translation candidates from the phrase-table for the selected French phrases\n\n";
   die "Usage: $0 phrase-table output-name French-stoplist"
}

my($pt,$out,$frStop) = @ARGV;

my %frStop = ();
open(FRS,$frStop) || die "Can't open $frStop:$!\n";
while(<FRS>){
    chomp;
    $frStop{$_}++;
}
close(FRS);

my %frPhrases = (); my %enPhrases = ();
open(PT,$pt) || die "Can't open $pt:$!\n";
while(<PT>){
    chomp;
    my ($f,$e,$a,$s) = split(/ \|\|\| /);
    my @f = split(/ /,$f);
    if ($f !~ m/[a-zA-Z]/){next};
    if (defined($frStop{$f[0]}) || defined($frStop{$f[$#f]})){next;};
    if ($f =~ m/^[\p{P}\p{S}0-9]+/){next};
    if ($f =~ m/[\p{P}\p{S}0-9]+$/){next};
    $frPhrases{$f}++;
    $enPhrases{$e}++;
}
close(PT);

open(EN,">$out.en") || die "Can't write to $out.en:$!\n";
foreach my $e (keys %enPhrases){
    print EN "$e\n";
}
close(EN);

open(FR,">$out.fr") || die "Can't write to $out.fr:$!\n";
foreach my $f (keys %frPhrases){
    print FR "$f\n";
}
close(FR);
