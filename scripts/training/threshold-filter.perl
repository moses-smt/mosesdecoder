#!/usr/bin/perl -w

use strict;

my %MIN_SCORE;
# legacy: same threshold for direct and indirect phrase translation probabilities
if ($ARGV[0] =~ /^[\d\.]+$/) {
  $MIN_SCORE{0} = $ARGV[0];
  $MIN_SCORE{2} = $ARGV[2];
}
# advanced: field:threshold,field:threshold
# recommended use is "2:0.0001"
else {
  foreach (split(/,/,$ARGV[0])) {
    my ($id,$score) = split(/:/);
    if ($score == 0) {
      die("error in spec $_ (full spec $ARGV[0])");
    }
    $MIN_SCORE{$id} = $score;
    print STDERR "score $id must be at least $score\n";
  }
}
die("please specify threshold (e.g., 0.0001)") unless scalar keys %MIN_SCORE;

my ($filtered,$total) = (0,0);
while(my $line = <STDIN>) {
  my @ITEM = split(/ \|\|\| /,$line);
  my @SCORE = split(/ /,$ITEM[2]);
  $total++;
  my $filter_this = 0;
  foreach my $key (keys %MIN_SCORE) {
    if ($SCORE[$key] < $MIN_SCORE{$key}) {
      $filter_this++;
    }
  }
  if ($filter_this) {
    $filtered++;
    next;
  }
  print $line;
}

print STDERR "filtered out $filtered of $total phrase pairs.\n";
