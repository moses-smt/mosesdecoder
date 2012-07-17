#!/usr/bin/perl -w
use strict;

my $source_ext = shift;
my $target_ext = shift;
my $out = shift;
my $frStop = shift;

if (not defined($out)) {
    print STDERR "$0\n";
    print STDERR "select French and English phrases from phrase-table for PSD modeling\n";
    print STDERR "  - French phrases: all phrases from phrase-table, except those that start or end with numbers, punctuation or stopwords\n";
    print STDERR "  - English phrases: all translation candidates from the phrase-table for the selected French phrases\n\n";
    die "Usage: $0 source-ext target-ext output-name [French-stoplist]\n";
}

my %frStop = ();
if (defined $frStop) {
  open(FRS,$frStop) || die "Can't open $frStop:$!\n";
  while(<FRS>){
    chomp;
    $frStop{$_}++;
  }
  close(FRS);
}

my %frPhrases = (); my %enPhrases = ();
while(<STDIN>){
    chomp;
    my ($f,$e,$a,$s) = split(/ \|\|\| /);
    my @f = split(/ /,$f);
    if (defined($frStop{$f[0]}) || defined($frStop{$f[$#f]})){next;};
    $frPhrases{$f}++;
    $enPhrases{$e}++;
}

open(EN,">$out.$target_ext") || die "Can't write to $out.$target_ext:$!\n";
foreach my $e (sort keys %enPhrases){
    print EN "$e\n";
}
close(EN);

open(FR,">$out.$source_ext") || die "Can't write to $out.$source_ext:$!\n";
foreach my $f (sort keys %frPhrases){
    print FR "$f\n";
}
close(FR);
