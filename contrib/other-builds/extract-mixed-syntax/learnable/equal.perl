#! /usr/bin/perl -w 

use strict;

sub trim($);

my $file1 = $ARGV[0];
my $file2 = $ARGV[1];

open (FILE1, $file1);
open (FILE2, $file2);

my $countEqual = 0;
while (my $line1 = <FILE1>) {
  my $line2 = <FILE2>;
  if (trim($line1) eq trim($line2)) {
    ++$countEqual;
  }
}

print $countEqual ."\n";


######################
# Perl trim function to remove whitespace from the start and end of the string
sub trim($) {
  my $string = shift;
  $string =~ s/^\s+//;
  $string =~ s/\s+$//;
  return $string;
}


