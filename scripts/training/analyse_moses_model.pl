#!/usr/bin/perl

# $Id$
# given a moses.ini file, checks the translation and generation tables and reports
# statistics on ambiguity
# Ondrej Bojar

use warnings;
use strict;
use Getopt::Long;

GetOptions(
);

my $ini = shift;
die "usage!" if !defined $ini;

open INI, $ini or die "Can't read $ini";
my $section = undef;
while (<INI>) {
  if (/^\[([^\]]*)\]\s*$/) {
    $section = $1;
  }
  if (/^[0-9]/) {
    if ($section eq "ttable-file") {
      chomp;
      my ($phrase_table_impl, $src, $tgt, $c, $fn) = split / /;
      # $fn = ensure_relative_to_origin($fn, $ini);
      my $ttstats = get_ttable_stats($fn);
      print_ttable_stats($src, $tgt, $fn, $ttstats);
    }
    if ($section eq "lmodel-file") {
      chomp;
      my ($a, $factor, $c, $fn) = split / /;
      # $fn = ensure_relative_to_origin($fn, $ini);
      my $lmstats = get_lmodel_stats($fn);
      print_lmodel_stats($factor, $fn, $lmstats);
    }
    if ($section eq "generation-file") {
      chomp;
      my ($src, $tgt, $c, $fn) = split / /;
      # $fn = ensure_relative_to_origin($fn, $ini);
      my $gstats = get_generation_stats($fn);
      print_generation_stats($src, $tgt, $fn, $gstats);
    }
  }
}
close INI;



sub ensure_relative_to_origin {
  my $target = shift;
  my $originfile = shift;
  return $target if $target =~ /^\/|^~/; # the target path is absolute already
  $originfile =~ s/[^\/]*$//;
  return $originfile."/".$target;
}


sub get_ttable_stats {
  my $fn = shift;
  my $opn = $fn =~ /\.gz$/ ? "zcat $fn |" : $fn;
  open IN, $opn or die "Can't open $opn";
  my $totphrs = 0;
  my $srcphrs = 0;
  my $lastsrc = undef;
  while (<IN>) {
    chomp;
    my ($src, $tgt, undef) = split /\|\|\|/;
    $totphrs ++;
    next if defined $lastsrc && $src eq $lastsrc;
    $lastsrc = $src;
    $srcphrs ++;
  }
  die "No phrases in $fn!" if !$totphrs;
  return { "totphrs"=>$totphrs, "srcphrs"=>$srcphrs };
}

sub print_ttable_stats {
  my ($src, $tgt, $fn, $stat) = @_;
  print "Translation $src -> $tgt ($fn):\n";
  print "  $stat->{totphrs}\tphrases total\n";
  printf "  %.2f\tphrases per source phrase\n", $stat->{totphrs}/$stat->{srcphrs};
}

sub get_generation_stats {
  my $fn = shift;
  my $opn = $fn =~ /\.gz$/ ? "zcat $fn |" : $fn;
  open IN, $opn or die "Can't open $opn";
  my $totphrs = 0;
  my $srcphrs = 0;
  my $lastsrc = undef;
  while (<IN>) {
    chomp;
    my ($src, $tgt, undef) = split /\s+/;
    $totphrs ++;
    next if defined $lastsrc && $src eq $lastsrc;
    $lastsrc = $src;
    $srcphrs ++;
  }
  die "No items in $fn!" if !$totphrs;
  return { "tot"=>$totphrs, "src"=>$srcphrs };
}

sub print_generation_stats {
  my ($src, $tgt, $fn, $stat) = @_;
  print "Generation $src -> $tgt ($fn):\n";
  printf "  %.2f\toutputs per source token\n", $stat->{tot}/$stat->{src};
}

sub get_lmodel_stats {
  my $fn = shift;
  my $opn = $fn =~ /\.gz$/ ? "zcat $fn |" : $fn;
  open IN, $opn or die "Can't open $opn";
  my %cnts;
  while (<IN>) {
    chomp;
    last if /^\\1-grams/;
    $cnts{$1} = $2 if /^ngram ([0-9]+)=([0-9]+)$/;
  }
  return { "ngrams"=>\%cnts };
}

sub print_lmodel_stats {
  my ($fact, $fn, $stat) = @_;
  print "Language model over $fact ($fn):\n";
  my @ngrams = sort {$a<=>$b} keys %{$stat->{ngrams}};
  print "  ".join("\t", @ngrams)."\n";
  print "  ".join("\t", map {$stat->{ngrams}->{$_}} @ngrams)."\n";
}


