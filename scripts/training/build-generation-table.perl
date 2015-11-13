#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# $Id$
use warnings;
use strict;
use Getopt::Long "GetOptions";

my $_CORPUS;
my $_OUTPUT = "generation";
my $_GENERATION_FACTORS;

die "specify options" unless &GetOptions('corpus=s' => \$_CORPUS,
       'output=s' => \$_OUTPUT,
       'generation-factors=s' => \$_GENERATION_FACTORS);


die "Please use --corpus to specify the factored input corpus\n" unless $_CORPUS;

if (! defined $_GENERATION_FACTORS) {
  die "Please use --generation-factors to set generation factors\n";
}

my $___GENERATION_FACTORS = $_GENERATION_FACTORS || "0-0";
die("format for generation factors is \"0-1\" or \"0-1+0-2\" or \"0-1+0,1-1,2\", you provided $___GENERATION_FACTORS\n")
  if $___GENERATION_FACTORS !~ /^\d+(\,\d+)*\-\d+(\,\d+)*(\+\d+(\,\d+)*\-\d+(\,\d+)*)*$/;

print "output=$_OUTPUT.<factor-map>\n";

get_generation_factored();
print "Done\n";
exit 0;

sub get_generation_factored {
    print STDERR "(8) learn generation model @ ".`date`;
    foreach my $f (split(/\+/,$___GENERATION_FACTORS)) {
        my $factor = $f;
        my ($factor_e_source,$factor_e) = split(/\-/,$factor);
        &get_generation($factor, $factor_e_source, $factor_e);
    }
}


sub get_generation {
    my ($factor, $factor_e_source, $factor_e) = @_;

    print STDERR "(8) [$factor] generate generation table @ ".`date`;
    my (%WORD_TRANSLATION,%TOTAL_FOREIGN,%TOTAL_ENGLISH);

    my %INCLUDE_SOURCE;
    foreach my $factor (split(/,/,$factor_e_source)) {

	$INCLUDE_SOURCE{$factor} = 1;
    }
    my %INCLUDE;
    foreach my $factor (split(/,/,$factor_e)) {
	$INCLUDE{$factor} = 1;
    }

    my (%GENERATION,%GENERATION_TOTAL_SOURCE,%GENERATION_TOTAL_TARGET);
    open(E,$_CORPUS) or die "Can't read ".$_CORPUS;
    while(<E>) {
	chomp;
	foreach (split) {
	    my @FACTOR = split(/\|/);

	    my ($source,$target);
	    my $first_factor = 1;
	    foreach my $factor (split(/,/,$factor_e_source)) {
		$source .= "|" unless $first_factor;
		$first_factor = 0;
		$source .= $FACTOR[$factor];
	    }

	    $first_factor = 1;
	    foreach my $factor (split(/,/,$factor_e)) {
		$target .= "|" unless $first_factor;
		$first_factor = 0;
		$target .= $FACTOR[$factor];
	    }
	    $GENERATION{$source}{$target}++;
	    $GENERATION_TOTAL_SOURCE{$source}++;
	    $GENERATION_TOTAL_TARGET{$target}++;
	}
    }
    close(E);

    open(GEN,">$_OUTPUT.$factor") or die "Can't write $_OUTPUT.$factor";
    foreach my $source (keys %GENERATION) {
	foreach my $target (keys %{$GENERATION{$source}}) {
	    printf GEN ("%s %s %.7f %.7f\n",$source,$target,
			$GENERATION{$source}{$target}/$GENERATION_TOTAL_SOURCE{$source},
			$GENERATION{$source}{$target}/$GENERATION_TOTAL_TARGET{$target});
	}
    }
    close(GEN);
    safesystem("rm -f $_OUTPUT.$factor.gz") or die;
    safesystem("gzip $_OUTPUT.$factor") or die;
}

sub safesystem {
  print STDERR "Executing: @_\n";
  system(@_);
  if ($? == -1) {
      print STDERR "Failed to execute: @_\n  $!\n";
      exit(1);
  }
  elsif ($? & 127) {
      printf STDERR "Execution of: @_\n  died with signal %d, %s coredump\n",
          ($? & 127),  ($? & 128) ? 'with' : 'without';
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}

