#!/usr/bin/env perl
# Converts AT&T FSA format to 'python lattice format'.
# Note that the input FSA needs to be epsilon-free and topologically sorted.
# This script checks for topological sortedness.
# The start node has to have the index 0.
# All path ends are assumed to be final nodes, not just the explicitly stated
# final nodes.
# Note that the output format may not contain any spaces.
# Ondrej Bojar, bojar@ufal.mff.cuni.cz
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use Getopt::Long;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

my $filelist;
my $ignore_final_state_cost = 0;
my $mangle_weights = undef;
GetOptions(
  "ignore-final-state-cost" => \$ignore_final_state_cost,
    # sometimes, final states have a cost (e.g. "45 0.05\n")
    # instead of dying there, ignore the problem
  "filelist|fl=s" => \$filelist,
  "mangle-weights=s" => \$mangle_weights,
) or exit 1;

my @infiles;
if (defined $filelist) {
  my $fh = my_open($filelist);
  while (<$fh>) {
    chomp;
    push @infiles, $_;
  }
  close $fh;
}
push @infiles, @ARGV;
@ARGV = ();
if (0 == scalar(@infiles)) {
  print STDERR "Reading input from stdin\n";
  push @infiles, "-";
}

my $err = 0;
foreach my $inf (@infiles) {
  my $nr = 0;
  NEXTLATTICE:
  my %usedids = (); # collect all used ids for densification
  my %usedtgtids = (); # collect all used ids for densification
  my @outnodes = ();
  my $fh = my_open($inf);
  my %is_final; # remember which nodes were final
  while (<$fh>) {
    chomp;
    $nr++;
    last if $_ eq ""; # assume a blank line delimits lattices
    my ($src, $tgt, $label, $weight) = split /\s+/;
    die "$inf:$nr:Bad src node index: $src" if $src !~ /^[0-9]+$/;

    if (!defined $label && !defined $weight) {
      # explicit final node, warn at the end if there are any intermed. final
      # nodes
      $is_final{$src};
      # final nodes can have a cost
      die "$inf:$nr:Final state $src has cost $tgt. Unsupported, use --ignore-final-state-cost"
        if defined $tgt && !$ignore_final_state_cost;

      next;
    }
    $weight = 0 if !defined $weight;

    $usedids{$src} = 1;
    $usedtgtids{$tgt} = 1;

    # process the weight
    # when reading RWTH FSA output, the weights are negated natural logarithms
    # we need to negate them back
    if (defined $mangle_weights) {
      if ($mangle_weights eq "expneg") {
        $weight = join(",", map {exp(-$_)} split /,/, $weight);
      } else {
        die "Bad weights mangling: $mangle_weights";
      }
    }
    # remember the node
    my $targetnode = $tgt-$src;
    die "$inf:$nr:Not topologically sorted, got arc from $src to $tgt"
      if $targetnode <= 0;
    push @{$outnodes[$src]}, [ $label, $weight, $tgt ];
  }
  if (eof($fh)) {
    close $fh;
    $fh = undef;
  }

  # Assign our dense IDs: source node ids are assigned first
  my %denseids = (); # maps node ids from the file to dense ids
  my $nextid = 0;
  foreach my $id (sort {$a<=>$b} keys %usedids) {
    $denseids{$id} = $nextid;
    $nextid++;
  }
  # All unseen target nodes then get the same next id, the final node id
  foreach my $id (keys %usedtgtids) {
    next if defined $denseids{$id};
    $denseids{$id} = $nextid;
  }

  foreach my $f (keys %is_final) {
    if (defined $outnodes[$f]) {
      print STDERR "$inf:Node $f is final but it has outgoing edges!\n";
      $err = 1;
    }
  }
#   # Verbose: print original to dense IDs mapping
#   foreach my $src (sort {$a<=>$b} keys %denseids) {
#     print STDERR "$src  ...> $denseids{$src}\n";
#   }

  print "(";
  for(my $origsrc = 0; $origsrc < @outnodes; $origsrc++) {
    my $src = $denseids{$origsrc};
    next if !defined $src; # this original node ID is not used at all
    next if $src == $nextid; # this is the ultimate merged final node
    my $outnode = $outnodes[$origsrc];
    print "(";
    foreach my $arc (@$outnode) {
      my $origtgt = $arc->[2];
      my $tgt = $denseids{$origtgt};
      if (!defined $tgt) {
        # this was a final node only
        $tgt = $denseids{$origtgt} = $nextid;
        $nextid++;
      }
      my $step_to_target = $tgt - $src;
      die "$inf:Bug, I damaged top-sortedness (orig $origsrc .. $origtgt; curr $src .. $tgt)." if $step_to_target <= 0;
      print "('".apo($arc->[0])."',$arc->[1],$step_to_target),";
    }
    print "),";
  }
  print ")\n";
  goto NEXTLATTICE if defined $fh && ! eof($fh);
}
die "There were errors." if $err;

sub apo {
  my $s = shift;
  # protects apostrophy and backslash
  $s =~ s/\\/\\\\/g;
  $s =~ s/(['])/\\$1/g;
  return $s;
}

sub my_open {
  my $f = shift;
  if ($f eq "-") {
    binmode(STDIN, ":utf8");
    return *STDIN;
  }

  die "Not found: $f" if ! -e $f;

  my $opn;
  my $hdl;
  my $ft = `file '$f'`;
  # file might not recognize some files!
  if ($f =~ /\.gz$/ || $ft =~ /gzip compressed data/) {
    $opn = "zcat '$f' |";
  } elsif ($f =~ /\.bz2$/ || $ft =~ /bzip2 compressed data/) {
    $opn = "bzcat '$f' |";
  } else {
    $opn = "$f";
  }
  open $hdl, $opn or die "Can't open '$opn': $!";
  binmode $hdl, ":utf8";
  return $hdl;
}
