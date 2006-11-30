#!/usr/bin/perl
# given a pathname to a factored corpus, a list of (numeric) factors to keep
# and a list of (labelled) factors to attach from factors/<basename>.label
# produces new corpus on stdout

use strict;
use warnings;
use Getopt::Long;
use IO::File;
use File::Basename;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

my $factordir = "factors";
GetOptions(
  "factordir=s" => \$factordir,
);
my $corppathname = shift;

my @requested_factors = split /[\+,]/, join("+", @ARGV);
die "usage: reduce_combine.pl corpusfile 0 add_factor_label1 2 add_factor_label2 ..."
  if !defined $corppathname || 0 == scalar @requested_factors;

my @addfactors = grep { ! /^[0-9]+$/ } @requested_factors;
# these are the labelled factors we need to load;


if ($corppathname eq "-") {
  *CORP=*STDIN;

  die "Won't add factors to corpus coming from stdin." if scalar @addfactors;
} else {
  open CORP, $corppathname or die "Can't read $corppathname";
  binmode(CORP, ":utf8");
}

my $corpdn = dirname($corppathname);
my $corpbn = basename($corppathname);
my %streams = map {
  my $fn = "$corpdn/$factordir/$corpbn.$_";
  my $stream = IO::File->new($fn, "<:utf8");
  die "Can't read '$fn'" if !defined $stream;
  ( $_, $stream ); # define a mapping factorlabel->stream
} @addfactors;

my $nr=0;
while (<CORP>) {
  $nr++;
  print STDERR "." if $nr % 10000 == 0;
  print STDERR "($nr)" if $nr % 100000 == 0;
  chomp;
  my @intokens = split / /;
  # load lines of corresponding streams and ensure equal number of words
  my %lines_of_extratoks;
  foreach my $factor (keys %streams) {
    my $line = readline($streams{$factor});
    die "Additional factor file $factor contains too few sentences!"
      if !defined $line;
    chomp($line);
    my @toks = split / /, $line;
    die "Incompatible number of words in factor $factor on line $nr."
      if $#toks != $#intokens;
    $lines_of_extratoks{$factor} = \@toks;
  }
  
  # for every token, print the factors in the order as user wished
  for(my $i=0; $i<=$#intokens; $i++) {
    my $token = $intokens[$i];
    my @outtoken = ();
    my @factors = split /\|/, $token;
    # print STDERR "Token: $token\n";
    foreach my $name (@requested_factors) {
      my $f = undef;
      if ($name =~ /^[0-9]+$/o) {
        # numeric factors should be copied from original corpus
        $f = $factors[$name];
        die "Missed factor $name in $token on line $nr"
          if !defined $f || $f eq "";
      } else {
        # named factors should be obtained from the streams
	$f = $lines_of_extratoks{$name}->[$i];
        die "Missed factor $name on line $nr"
          if !defined $f || $f eq "";
      }
      # print STDERR "  Factor $name: $f\n";
      push @outtoken, $f;
    }
    print " " if $i != 0;
    print join("|", @outtoken);
  }
  print "\n";
}
close CORP;
print STDERR "Done.\n";



