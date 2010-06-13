#!/usr/bin/perl -w

use strict;

die("ERROR: syntax: run-command-on-multiple-refsets.perl cmd in out") 
    unless scalar @ARGV == 3;
my ($cmd,$in,$out) = @ARGV;

for(my $i=0;-e "$in.ref$i";$i++) {
    my $single_cmd = $cmd;
    $single_cmd =~ s/mref-input-file/$in.ref$i/g;
    $single_cmd =~ s/mref-output-file/$out.ref$i/g;
    system($single_cmd);
}
