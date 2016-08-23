#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use Getopt::Long "GetOptions";
use FindBin qw($RealBin);

my $___FACTOR_DELIMITER = "|";

# utilities
my $ZCAT = "gzip -cd";
my $BZCAT = "bzcat";

my ($CORPUS,$REDUCED,$FACTOR,$_XML);
die("ERROR: wrong syntax when invoking reduce-factors")
    unless &GetOptions('corpus=s' => \$CORPUS,
                       'reduced-corpus=s' => \$REDUCED,
		       'factor=s' => \$FACTOR,
		       'xml' => \$_XML);

&reduce_factors($CORPUS,$REDUCED,$FACTOR);

# from train-model.perl
sub reduce_factors {
    my ($full,$reduced,$factors) = @_;

    my @INCLUDE = sort {$a <=> $b} split(/,/,$factors);

    print STDERR "(1.0.5) reducing factors to produce $reduced  @ ".`date`;
    while(-e $reduced.".lock") {
        sleep(10);
    }
    if (-e $reduced) {
        print STDERR "  $reduced in place, reusing\n";
        return;
    }
    if (-e $reduced.".gz") {
        print STDERR "  $reduced.gz in place, reusing\n";
        return;
    }

    unless ($_XML) {
        # peek at input, to check if we are asked to produce exactly the
        # available factors
        my $inh = open_or_zcat($full);
        my $firstline = <$inh>;
        die "Corpus file $full is empty" unless $firstline;
        close $inh;
        # pick first word
        $firstline =~ s/^\s*//;
        $firstline =~ s/\s.*//;
        # count factors
        my @WORD = split(/ /,$firstline);
        my @FACTOR = split(/$___FACTOR_DELIMITER/,$WORD[0]);
        my $maxfactorindex = scalar(@FACTOR)-1;
        if (join(",", @INCLUDE) eq join(",", 0..$maxfactorindex)) {
          # create just symlink; preserving compression
          my $realfull = $full;
          if (!-e $realfull && -e $realfull.".gz") {
            $realfull .= ".gz";
            $reduced =~ s/(\.gz)?$/.gz/;
          }
          safesystem("ln -s '$realfull' '$reduced'")
            or die "Failed to create symlink $realfull -> $reduced";
          return;
        }
    }

    # The default is to select the needed factors
    `touch $reduced.lock`;
    *IN = open_or_zcat($full);
    open(OUT,">".$reduced) or die "ERROR: Can't write $reduced";
    my $nr = 0;
    while(<IN>) {
        $nr++;
        print STDERR "." if $nr % 10000 == 0;
        print STDERR "($nr)" if $nr % 100000 == 0;
        s/<\S[^>]*>/ /g if $_XML; # remove xml
        chomp; s/ +/ /g; s/^ //; s/ $//;
        my $first = 1;
        foreach (split) {
            my @FACTOR = split /\Q$___FACTOR_DELIMITER/;
              # \Q causes to disable metacharacters in regex
            print OUT " " unless $first;
            $first = 0;
            my $first_factor = 1;
            foreach my $outfactor (@INCLUDE) {
              print OUT $___FACTOR_DELIMITER unless $first_factor;
              $first_factor = 0;
              my $out = $FACTOR[$outfactor];
              die "ERROR: Couldn't find factor $outfactor in token \"$_\" in $full LINE $nr" if !defined $out;
              print OUT $out;
            }
            # for(my $factor=0;$factor<=$#FACTOR;$factor++) {
                # next unless defined($INCLUDE{$factor});
                # print OUT "|" unless $first_factor;
                # $first_factor = 0;
                # print OUT $FACTOR[$factor];
            # }
        }
        print OUT "\n";
    }
    print STDERR "\n";
    close(OUT);
    close(IN);
    `rm -f $reduced.lock`;
}

sub open_or_zcat {
  my $fn = shift;
  my $read = $fn;
  $fn = $fn.".gz" if ! -e $fn && -e $fn.".gz";
  $fn = $fn.".bz2" if ! -e $fn && -e $fn.".bz2";
  if ($fn =~ /\.bz2$/) {
      $read = "$BZCAT $fn|";
  } elsif ($fn =~ /\.gz$/) {
      $read = "$ZCAT $fn|";
  }
  my $hdl;
  open($hdl,$read) or die "Can't read $fn ($read)";
  return $hdl;
}

sub safesystem {
  print STDERR "Executing: @_\n";
  system(@_);
  if ($? == -1) {
    print STDERR "ERROR: Failed to execute: @_\n  $!\n";
    exit(1);
  }
  elsif ($? & 127) {
    printf STDERR "ERROR: Execution of: @_\n  died with signal %d, %s coredump\n",
      ($? & 127),  ($? & 128) ? 'with' : 'without';
    exit(1);
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}


