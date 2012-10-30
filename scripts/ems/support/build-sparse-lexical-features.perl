#!/usr/bin/perl -w

use strict;

# Build necessary files for sparse lexical features
# * target word insertion
# * source word deletion
# * word translation
# * phrase length

my ($corpus,$input_extension,$output_extension,$outfile_prefix,$specification) = @ARGV;
my $ini = "";
my $report = "";
my %ALREADY;

foreach my $feature_spec (split(/,\s*/,$specification)) {
  my @SPEC = split(/\s+/,$feature_spec);
  if ($SPEC[0] eq 'target-word-insertion') {
    if ($SPEC[1] eq 'top' && $SPEC[2] =~ /^\d+$/) {
      my $file = &create_top_words($output_extension, $SPEC[2]);
      $ini .= "[target-word-insertion-feature]\n0 $file\n\n";
      $report .= "twi\n";
    }
    else {
      die("ERROR: Unknown parameter specification in '$feature_spec'\n");
    }
  }
  elsif ($SPEC[0] eq 'source-word-deletion') {
    if ($SPEC[1] eq 'top' && $SPEC[2] =~ /^\d+$/) {
      my $file = &create_top_words($input_extension, $SPEC[2]);
      $ini .= "[source-word-deletion-feature]\n0 $file\n\n";
      $report .= "swd\n";
    }
    else {
      die("ERROR: Unknown parameter specification in '$feature_spec'\n");
    }
  }
  elsif ($SPEC[0] eq 'word-translation') {
    if ($SPEC[1] eq 'top' && $SPEC[2] =~ /^\d+$/ && $SPEC[3] =~ /^\d+$/) {
      my $file_in  = &create_top_words($input_extension,  $SPEC[2]);
      my $file_out = &create_top_words($output_extension, $SPEC[3]);
      $ini .= "[word-translation-feature]\n0-0 0 0 0 $file_in $file_out\n\n";
      $report .= "wt\n";
    }
    else {
      die("ERROR: Unknown parameter specification in '$feature_spec'\n");
    }
  }
  elsif ($SPEC[0] eq 'phrase-length') {
    $ini .= "[phrase-length-feature]\ntrue\n\n";
    $report .= "pl\n";
  }
  else {
    die("ERROR: Unknown feature type '$SPEC[0]' in specification '$feature_spec'\nfull spec: '$specification'\n");
  }
}

open(INI,">$outfile_prefix.ini");
print INI $ini;
print INI "\n[report-sparse-features]\n$report\n";
print INI "\n[use-alignment-info]\ntrue\n\n";
close(INI);

sub create_top_words {
  my ($extension, $count) = @_;
  my $file = "$outfile_prefix.$extension.top$count";
  return $file if defined($ALREADY{"$extension,$count"});
  $ALREADY{"$extension,$count"}++;

  # get counts
  my %COUNT;
  open(CORPUS,"$corpus.$extension");
  while(<CORPUS>) {
    chop;
    foreach (split) {
      $_ =~ s/\|.+//; # only surface factor at this point
      $COUNT{$_}++ unless $_ eq "";
    }
  }
  close(CORPUS);

  # sort
  my @COUNT_WORD;
  foreach (keys %COUNT) {
    next if $COUNT{$_} <= 3; # avoid large tail
    next if $_ =~ /:/; # avoid colon bug
    push @COUNT_WORD,sprintf("%09d %s",$COUNT{$_},$_);
  }
  my @SORTED = reverse sort @COUNT_WORD;

  # write top n to file
  open(TOP,">$file");
  for(my $i=0; $i<$count && $i<scalar(@SORTED); $i++) {
    $SORTED[$i] =~ /^\d+ (.+)$/;
    print TOP "$1\n"; 
  }
  close(TOP);

  return $file;
}
