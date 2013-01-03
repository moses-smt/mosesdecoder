#!/usr/bin/perl

# Extract a training corpus for local language models (Monz 2011)
# from a factored input corpus (form|tag).

use strict;
use warnings;
use Getopt::Long;
use File::Basename;
use File::Path;
use List::Util qw(min max);

my $headtag = "HEAD";
my $window = 5;

if (! GetOptions(
    "headtag|h=s" => \$headtag,
    "window|w=i" => \$window,
  )) {
  die "Usage: $0 [-window SIZE] [-headtag TAG]";
}

my %contexts;

my $line_num = 0;
while (<STDIN>) {
  chomp(my $line = $_);
  $line_num++;
  my @tokens = split " ", $line;
  my @words;
  my @tags;
  for my $token (@tokens) {
    my ($word, $tag) = split /\|/, $token;
    push @words, $word;
    push @tags, $tag;
  }

  for (my $i = 0; $i < scalar @tags; $i++) {
    die "Missing tag on line $line_num" if ! defined $tags[$i];
    process_window($i, $words[$i], \@tags);
  }
}

sub process_window {
  my ($idx, $word, $tags) = @_;
  my $first = max(0, $idx - $window + 1);
  my $last = min(scalar @$tags - 1, $idx + $window - 1);
  my @outtokens = ();
  for (my $i = $first; $i <= $last; $i++) {
    my $tag = ($i == $idx) ? "HEAD" : @$tags[$i];
    my $pos = $i - $idx;
    push @outtokens, "$tag|$pos|$word";
  }
  print join(" ", @outtokens), "\n";
}

