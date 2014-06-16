#!/usr/bin/perl 

use strict;
use Getopt::Long "GetOptions";

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

sub trim($);
sub DeleteScore;

my $keepScoresStr;
GetOptions(
  "keep-scores=s" => \$keepScoresStr
) or exit(1);

my @keepScores = split(/,/, $keepScoresStr);

#MAIN LOOP
while (my $line = <STDIN>) {
  chomp($line);
  #print STDERR "line=$line\n";
  
  my @toks = split(/\|/, $line);
  my @scores = split(/ /, $toks[6]);
  
  $toks[6] = DeleteScore($toks[6], \@keepScores);

  # output
  print $toks[0];
  for (my $i = 1; $i < scalar(@toks); ++$i) {
    print "|" .$toks[$i];
  }
  print "\n";
}

######################
# Perl trim function to remove whitespace from the start and end of the string
sub trim($) {
  my $string = shift;
  $string =~ s/^\s+//;
  $string =~ s/\s+$//;
  return $string;
}

sub DeleteScore
{
  my $string = $_[0];
  my @keepScores = @{$_[1]};
  
  $string = trim($string);
  my @toks = split(/ /, $string);

  $string = "";
  for (my $i = 0; $i < scalar(@keepScores); ++$i) {
    $string .= $toks[ $keepScores[$i] ] ." ";
  }
  $string = " " .$string;
  
  return $string;
}


