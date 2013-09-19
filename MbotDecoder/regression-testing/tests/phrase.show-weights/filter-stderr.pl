#!/usr/bin/perl

BEGIN { use Cwd qw/ abs_path /; use File::Basename; $script_dir = dirname(abs_path($0)); push @INC, "$script_dir/../perllib"; }
use RegTestUtils;

$x=0;
while (<>) {
  chomp;
}
