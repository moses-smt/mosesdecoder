#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;

my ($lowercase,$cluster_file,$in,$out,$tmp) = @ARGV;

my $CLUSTER = &read_cluster_from_mkcls($cluster_file);

# is $lowercase a script?
if ($lowercase =~ /\//) {
  open(IN,"$lowercase < $in|") || die("ERROR: could not open input");
  $lowercase = 0;
}
else {
  open(IN,$in) || die("ERROR: could not open input");
}
binmode(IN, ":utf8");
open(OUT,">$out");
binmode(OUT, ":utf8");
while(<IN>) {
  chop;
  s/\s+/ /g;
  s/^ //;
  s/ $//;
  my $first = 1;
  foreach my $word (split) {
    # if lowercase is a flag
    if ($lowercase) {
      $word = lc($word);
    }
    my $cluster = defined($$CLUSTER{$word}) ? $$CLUSTER{$word} : "0";
    print OUT " " unless $first;
    print OUT $cluster;
    $first = 0;
  }
  print OUT "\n";
}
close(OUT);
close(IN);

sub read_cluster_from_mkcls {
  my ($file) = @_;
  my %CLUSTER;
  open(CLUSTER_FILE,$file) || die("ERROR: could not open cluster file '$file'");
  binmode(CLUSTER_FILE, ":utf8");
  while(<CLUSTER_FILE>) {
    chop;
    my ($word,$cluster) = split;
    $CLUSTER{$word} = $cluster;
  }
  close(CLUSTER_FILE);
  return \%CLUSTER;
}

sub add_cluster_to_string {
}


