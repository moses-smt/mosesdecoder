#!/usr/bin/perl -w

#rayuela.pl - find reorderings

use strict;
use warnings;
use Carp;

BEGIN {
    @ARGV == 3 or croak "USAGE: rayuela.pl SOURCE TARGET ALIGNMENTS";
}

use List::MoreUtils qw(any all none notall true false firstidx first_index                                  lastidx last_index insert_after insert_after_string                                  apply after after_incl before before_incl indexes firstval first_value lastval last_value each_array each_arrayref pairwise natatime mesh zip uniq minmax);
use List::Util qw(first max maxstr min minstr reduce shuffle sum);
my ($source_segments_file,$target_segments_file,$alignments_file) = @ARGV;

my $source_segments_aoaref = store($source_segments_file);
my $target_segments_aoaref = store($target_segments_file);
my $alignments_aoaref = store($alignments_file);

SEGMENT: for my $segment_number (0..$#{$alignments_aoaref} ) {
ALIGNMENT: foreach my $alignment_pair ( @{$alignments_aoaref->[$segment_number]} ) {
my ($source_position,$target_position) = split /\-/xms, $alignment_pair, 2;
croak " segment $segment_number\n@{$source_segments_aoaref->[$segment_number]}\n";

}
					   }


sub store {
    my ($file_name) = @_;

    my @segments = ();

open my $FILE, '<', "$file_name" or croak "could not open file $file_name for reading";

    my @lines = <$FILE>;

    foreach my $line ( @lines ) {
    
	chomp $line;

	my @tokens = split /\s+/xms, $line;
    
	push @segments, \@tokens;
    
	return \@segments;
    }
}
