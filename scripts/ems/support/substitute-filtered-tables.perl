#!/usr/bin/perl -w

# experiment.perl support script
# get filtered rule and reordering tables and place them into a configuration file

if (scalar @ARGV < 1 || ! -e $ARGV[0]) {
  die("ERROR: could not find pseudo-config with filtered tables");
}

# read config sections about filtered tables
my %CONFIG;
my $status = 0;
my $section;
open(FILTERED, $ARGV[0]) or die "Cannot open: $!";
while(<FILTERED>) {
  if (/^\[(.+)\]\s*$/) {
    if ($1 eq "ttable-file" || $1 eq "distortion-file") {
      $section = $1;
      $status = 1;
      print STDERR "found $section\n";
    }
    else {
      $status = 0;
    }
  }
  elsif ($status) {
    $CONFIG{$section} .= $_;
  }
}
close(FILTERED);

# pass through master config file and replace table sections
($status,$section) = (0);
while(<STDIN>) {
  if (/^\[(.+)\]\s*$/) {
    print $_;
    if ($1 eq "ttable-file" || $1 eq "distortion-file") {
      print STDERR "replacing $1\n";
      print $CONFIG{$1};
      $status = 1;
    }
    else {
      $status = 0;
    }
  }
  elsif (!$status) {
    print $_;
  }
}
