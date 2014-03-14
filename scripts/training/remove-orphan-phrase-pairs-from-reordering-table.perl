#!/usr/bin/perl -w

use strict;

my ($ttable_file) = @ARGV;

die("syntax: remove-orphan-phrase-pairs-from-reordering-table.perl TTABLE < REORDERING_TABLE > REORDERING_TABLE.pruned")
    unless defined($ttable_file) && -e $ttable_file;

if ($ttable_file =~ /gz$/) {
  open(TTABLE,"zcat $ttable_file|");
}
else {
  open(TTABLE,$ttable_file);
}

# get first ttable line to be matched
my $ttable_line = <TTABLE>;
my $ttable_phrase_pair = &get_phrase_pair($ttable_line);

# loop through reordering table
while(my $reordering_line = <STDIN>) {
  my $reordering_phrase_pair = &get_phrase_pair($reordering_line);

  # if it does not match ttable line, then keep looping
  #print STDERR "$reordering_phrase_pair ?? $ttable_phrase_pair\n";
  while($reordering_phrase_pair ne $ttable_phrase_pair) {
    #print STDERR "$reordering_phrase_pair != $ttable_phrase_pair\n";
    $reordering_line = <STDIN>;
    last if !defined($reordering_line); # end of file, done
    $reordering_phrase_pair = &get_phrase_pair($reordering_line);
  }
  last if !defined($reordering_line); # end of file, done

  # print matched line
  print $reordering_line;

  # read next ttable line to be matched
  $ttable_line = <TTABLE>;
  last if !defined($ttable_line); # end of file, done
  $ttable_phrase_pair = &get_phrase_pair($ttable_line);
}
if (defined($ttable_line)) {
  print STDERR "ERROR: trailing ttable lines -> could not find $ttable_line!\n";
}

sub get_phrase_pair {
  my ($line) = @_;
  my ($src,$tgt,$other) = split(/ \|\|\| /,$line);
  return "$src ||| $tgt";
}
