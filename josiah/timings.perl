#!/usr/bin/env perl

use strict;

my ($decoder_start,$decoder_end,$sampler_end,$sampler_start);

my $decoder_total=0;
my $sampler_total=0;
my $n=0;

while(<>) {
    if (/^TIME.*\[([0-9]+)\]/) {
       my $time = $1;
       $decoder_start=$time if /Running decoder/;
       if (/Running sampler/) {
           my $decoder = $time - $decoder_start;
           $sampler_start = $time;
           print "Decoder: $decoder\n";
           $decoder_total += $decoder;
       }
       if (/Outputting results/) {
           my $sampler = $time - $sampler_start;
           print "Sampler: $sampler\n";
           $sampler_total += $sampler;
           ++$n;
       }
    }
}

my $sampler_mean = $sampler_total / $n;
my $decoder_mean = $decoder_total / $n;

print "Sampler: Total: $sampler_total Mean: $sampler_mean\n";
print "Decoder: Total: $decoder_total Mean: $decoder_mean\n";
