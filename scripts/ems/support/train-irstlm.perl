#!/usr/bin/perl -w

use strict;

# wrapper for irstlm training

my $IRSTLM = shift @ARGV;

my $settings = join(" ",@ARGV);
$settings =~ s/\-order/\-n/;
$settings =~ s/\-text/\-i/;
$settings =~ s/\-lm/\-o/;

if ($settings !~ /\-o +(\S+)/) {
  die("ERROR: no output file specified");
}
my $lm = $1;
$settings =~ s/(\-o +\S+)/$1.iarpa.gz/;

my $cmd = "IRSTLM=$IRSTLM $IRSTLM/scripts/build-lm.sh $settings ; ~/moses/irstlm/bin/compile-lm --text yes $lm.iarpa.gz $lm";
print STDERR $cmd."\n";
print `$cmd`;
