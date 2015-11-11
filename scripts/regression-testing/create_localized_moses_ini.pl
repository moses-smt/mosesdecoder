#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
my $script_dir; BEGIN { use Cwd qw/ abs_path /; use File::Basename; $script_dir = dirname(abs_path($0)); push @INC, $script_dir; }
use MosesScriptsRegressionTesting;

my $data_dir=$ARGV[0]; shift(@ARGV);
my $conf=$ARGV[0]; shift(@ARGV);

print STDERR "configuration file: |$conf|\n";
print STDERR "data_dir: |$data_dir|\n";

my $local_moses_ini = MosesScriptsRegressionTesting::get_localized_moses_ini($conf, $data_dir);

print STDERR "local_moses_ini: |$local_moses_ini|\n";
print STDOUT "$local_moses_ini\n";

