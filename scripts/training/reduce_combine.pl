#!/usr/bin/perl
# given a pathname to a factored corpus, a list of (numeric) factors to keep
# and a list of (labelled) factors to attach from factors/<basename>.label
# produces new corpus on stdout

use strict;
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

my @args = split /\+/, join("+", @ARGV);
my $keepfactors = shift @args;
my @addfactors = @args;
die "usage: reduce_combine.pl corpusfile 0,1,2 add_factor_label1 add_factor_label2 ..."
  if !defined $corppathname || !defined $keepfactors;

my @keepfactors = ();
if ($keepfactors =~ /^[0-9,]+$/) {
  # assume these are really factors to keep
  @keepfactors = split /,/, $keepfactors;
} else {
  # assume there are no factors to keep, just to output some added
  unshift @addfactors, $keepfactors;
}

open CORP, $corppathname or die "Can't read $corppathname";
binmode(CORP, ":utf8");

my $corpdn = dirname($corppathname);
my $corpbn = basename($corppathname);
my @streams = map {
  my $fn = "$corpdn/$factordir/$corpbn.$_";
  IO::File->new($fn, "<:utf8") or die "Can't read '$fn'"
} @addfactors;

my $nr=0;
while (<CORP>) {
  $nr++;
  print STDERR "." if $nr % 10000 == 0;
  print STDERR "($nr)" if $nr % 100000 == 0;
  chomp;
  my @intokens = split / /;
  my $extratokens = undef;
  for(my $i=0; $i<=$#streams; $i++) {
    my $line = readline($streams[$i]);
    die "Additional factor file $addfactors[$i] contains too few sentences!"
      if !defined $line;
    chomp($line);
    my @extrafactors = split / /, $line;
    die "Incompatible number of words in factor $addfactors[$i] on line $nr."
      if $#extrafactors != $#intokens;
    for(my $j=0; $j<=$#extrafactors; $j++) {
      $extratokens->[$j]->[$i] = $extrafactors[$j];
    }
  }
  my @outline = ();
  for(my $i=0; $i<=$#intokens; $i++) {
    my $token = $intokens[$i];
    my @outtoken = ();
    my @factors = split /\|/, $token;
    foreach my $fid (@keepfactors) {
      my $f = @factors[$fid];
      die "Missed factor $fid in $token on line $nr"
        if !defined $f;
      push @outtoken, $factors[$fid];
    }
    push @outtoken, @{$extratokens->[$i]} if 0 < scalar @addfactors;
    push @outline, join("|", @outtoken);
  }
  print join(" ", @outline)."\n";
}
close CORP;
print STDERR "Done.\n";



