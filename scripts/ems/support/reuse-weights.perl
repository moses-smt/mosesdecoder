#!/usr/bin/perl

# $Id: reuse-weights.perl 41 2007-03-14 22:54:18Z hieu $

use strict;

die("ERROR: syntax: reuse-weights.perl weights.ini < moses.ini > weighted.ini")
    unless scalar @ARGV == 1;
my ($weight_file) = @ARGV;

my %WEIGHT;
my $current_weight = "";
my $weights_file_spec = "";
my $weights_file_flag = 0;
open(WEIGHT,$weight_file)
    || die("ERROR: could not open weight file: $weight_file");
while(<WEIGHT>) {
    if (/^\[weight-file\]/) {
      $weights_file_spec = "\n".$_;
      $weights_file_flag = 1;
    }
    elsif (/^\[weight\-(\S+)\]/) {
	$current_weight = $1;
    }
    elsif ($current_weight && /^(([\-\d\.]+)([Ee][+-]?[\d]+)?)$/) {
	push @{$WEIGHT{$current_weight}},$1;
    }
    elsif ($weights_file_flag && !/^\[/ && !/^\s*$/) {
      $weights_file_spec .= $_;
    }
    elsif (/^\[/) {
      $current_weight = "";
      $weights_file_flag = 0;
    }
}
close(WEIGHT);

my %IGNORE;
while(<STDIN>) {
    if (/^\[weight-file\]/) {
	$weights_file_flag = 1;
    }
    elsif (/^\[weight\-(\S+)\]/) {
	$current_weight = $1;
	if (!defined($WEIGHT{$current_weight})) {    
	    if (/^\[weight\-wt/ or /^\[weight\-pp/) {
		print $_;
		$_ = <STDIN>;
		print $_;
	    }
	    else {
		print STDERR "(reuse-weights) WARNING: no weights for weight-$1, deleting\n";
		$current_weight = "xxx";
	    }
	}
	else {
	    print $_;
	    foreach (@{$WEIGHT{$current_weight}}) {
		print $_."\n";
	    }
	}
    }
    elsif ($current_weight && /^([\-\d\.]+)([Ee][+-]?[\d]+)?$/) {
	$IGNORE{$current_weight}++;
    }
    elsif ($weights_file_flag && !/^\[/ && !/^\s*$/) {
	$weights_file_flag = 0;
        # if weight-file was not defined in weights.ini, take this version 
	#$weights_file_spec = "\n[weight-file]\n".$_;
    }
    elsif (/^\[/) {
	$current_weight = "";
	print $_;
    }
    else {
	print $_;
    }
}

foreach my $weight (keys %WEIGHT) {
    if (! defined($IGNORE{$weight})) {
	print STDERR "(reuse-weights) WARNING: new weights weight-$weight\n";
	print "\n[weight-$weight]\n";
	foreach (@{$WEIGHT{$weight}}) {
	    print $_."\n";
	}
    }
    else {
	print STDERR "(reuse-weights) weight-$weight updated ($IGNORE{$weight} -> ".(scalar @{$WEIGHT{$weight}}).")\n";
	if ($IGNORE{$weight} != scalar @{$WEIGHT{$weight}}) {
	    print STDERR "(reuse-weights) WARNING: number of weights changed\n";
	}
    }
}

print $weights_file_spec;

