#!/usr/bin/perl -w

use strict;

die("ERROR: syntax: run-command-on-multiple-refsets.perl cmd in out") 
    unless scalar @ARGV == 3;
my ($cmd,$in,$out) = @ARGV;

die("ERROR: attempt to run on multiple references, but there is only one")
    if -e $in && ! -e "$in.ref0";
die("ERROR: did not find reference '$in.ref0'")
    unless -e "$in.ref0";

for(my $i=0;-e "$in.ref$i";$i++) {
    my $single_cmd = $cmd;
    $single_cmd =~ s/mref-input-file/$in.ref$i/g;
    $single_cmd =~ s/mref-output-file/$out.ref$i/g;
    system($single_cmd);
}
