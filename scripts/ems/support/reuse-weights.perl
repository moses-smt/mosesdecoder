#!/usr/bin/perl

# $Id: reuse-weights.perl 41 2007-03-14 22:54:18Z hieu $

use strict;
use warnings;

die("ERROR: syntax: reuse-weights.perl weights.ini < moses.ini > weighted.ini")
    unless scalar @ARGV == 1;
my ($weight_file) = @ARGV;

my %WEIGHT;
my $is_weight = "";
my $weights_file_spec = "";
my $weights_file_flag = 0;
open(WEIGHT,$weight_file)
    || die("ERROR: could not open weight file: $weight_file");
while(<WEIGHT>) {
    if (/^\[weight-file\]/) {
      $weights_file_spec = "\n".$_;
      $weights_file_flag = 1;
    }
    elsif (/^\[weight\]/) {
		$is_weight = 1;
    }
    elsif ($is_weight && /^([^=]*)\s*=\s*(.*)\s*$/) {
        $WEIGHT{$1}=$2;
    }
    elsif ($weights_file_flag && !/^\[/ && !/^\s*$/) {
      $weights_file_spec .= $_;
    }
    elsif (/^\[/) {
      $is_weight = 0;
      $weights_file_flag = 0;
    }
}
close(WEIGHT);

$is_weight = 0;


my %IGNORE;
while(<STDIN>) {
    if (/^\[weight-file\]/) {
	$weights_file_flag = 1;
    }
    elsif (/^\[weight\]/) {
        $is_weight=1;
        print $_;
    }
    elsif ($is_weight) {
       my $line = $_;
	   if (/^([^=]*)\s*=\s*(.*)$/ ) {
          my $key=$1;
          my $value=$2;
          if (exists $WEIGHT{$key}) {
              print $key."= ".$WEIGHT{$key}."\n";
              $IGNORE{$key}++;
          } else {
              if ($key =~ /^PhrasePairFeature/ or $key =~/^WordTranslationFeature/) {
                 print $line;
              } else {
	        	 print STDERR "(reuse-weights) WARNING: no weights for $key, deleting\n";
              }
          }
       } else {
          print $line;
       }
    }
    elsif ($weights_file_flag && !/^\[/ && !/^\s*$/) {
	$weights_file_flag = 0;
        # if weight-file was not defined in weights.ini, take this version 
	#$weights_file_spec = "\n[weight-file]\n".$_;
    }
    elsif (/^\[/) {
        if ($is_weight) {
            print_remaining(); 
        }
        $is_weight = 0;
        print $_;
    }
    else {
	print $_;
    }
}

if ($is_weight) {
    print_remaining();
}
sub print_remaining {
    foreach my $weight (keys %WEIGHT) {
        if (! defined($IGNORE{$weight})) {
            print STDERR "(reuse-weights) WARNING: new weights $weight\n";
            print "\n$weight= ";
            print $WEIGHT{$weight};
            print "\n";
        }
        else {
            print STDERR "(reuse-weights) $weight updated \n";
        }
    }
}

print $weights_file_spec;

