#!/usr/bin/perl

use strict;
use FindBin qw($RealBin);
BEGIN { require "$RealBin/LexicalTranslationModel.pm"; "LexicalTranslationModel"->import; }

if (scalar(@ARGV) < 4) {

    print STDERR $0." source target alignments output_prefix"."\n"

} else {

    my ($SOURCE,$TARGET,$ALIGNMENT,$OUT) = @ARGV;
    
    &get_lexical($SOURCE,$TARGET,$ALIGNMENT,$OUT,0);

}


