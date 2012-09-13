#!/usr/bin/perl -w

use strict;

# Create domain file from corpora
# (helper for domain adatpation)

# Creates a file with domain names and end line numbers for different domains
# within the cleaned training corpus. This file is used by various domain 
# adaptation methods.

my ($extension,@SUBCORPORA) = @ARGV;

my $line_count = 0;
my %UNIQUE_NAME;
my $number = 1;
foreach (@SUBCORPORA) {
  # get number of lines
  if (!-e "$_.$extension" && -e "$_.$extension.gz") {
    $line_count += `zcat $_.$extension.gz | wc -l`;
  }
  elsif (-e "$_.$extension") {
    $line_count += `wc -l < $_.$extension`;
  }
  else {
    die("ERROR: could not open sub corpus file $_.$extension\n");
  }

  # construct name
  my $name = $number++; # default: cardinal number
  while(defined($UNIQUE_NAME{$name})) { $name = $number++; } # slightly paranoid
  if (/\/([^\.\/]+)\.[^\/]+$/ && !defined($UNIQUE_NAME{$1})) { # reconstruct corpus name
    $name = $1;
    $UNIQUE_NAME{$1}++;
  }
  print "$line_count $name\n";
}

