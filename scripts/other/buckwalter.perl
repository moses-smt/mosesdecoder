#!/usr/bin/env perl

use strict;
use warnings;
use Encode::Arabic::Buckwalter;
use Getopt::Long "GetOptions";

my $direction;
GetOptions('direction=i' => \$direction)
    or exit(1);
# direction: 1=arabic->bw, 2=bw->arabic

die("ERROR: need to set direction") unless defined($direction);



while (my $line = <STDIN>) {
    chomp($line);

    my $lineOut;
    if ($direction == 1) {
      $lineOut =  encode 'buckwalter', decode 'utf8', $line;
    }
    elsif ($direction == 2) {
      $lineOut =  encode 'utf8', decode 'buckwalter', $line;
    }
    else {
	die("Unknown direction: $direction");
    }
    print "$lineOut\n";

}

