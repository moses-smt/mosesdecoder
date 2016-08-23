#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use utf8;
use Getopt::Long "GetOptions";

Getopt::Long::config("pass_through");

my ($BIN,$MODEL);

&GetOptions('bin=s' => \$BIN,
            'model=s' => \$MODEL);  # À la truecase.perl

die("ERROR: specify at least --bin BIN!") unless defined($BIN);

my $cmd = "$BIN";
$cmd .= " -case true:model=$MODEL" if defined($MODEL);
$cmd .= " " . join(' ', @ARGV) if scalar(@ARGV);  # Pass other args to $BIN.

system $cmd;
