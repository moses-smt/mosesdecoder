#!/usr/bin/perl -w

use strict;

use Getopt::Long "GetOptions";
my ($IN,$OUT,$MXPOST);
if (!&GetOptions('mxpost=s' => \$MXPOST) ||
    !($IN = shift @ARGV) ||
    !($OUT = shift @ARGV) ||
    !defined($MXPOST)) {
        print "syntax: make-pos-en.mxpost.perl -mxpost INSTALL_DIR IN_FILE OUT_FILE\n";
        exit(1);
}

open(TAGGER,"cat $IN | perl -ne 's/—/-/g; s/\\p{Dash_Punctuation}/-/g; s/\\p{Open_Punctuation}/\(/g; s/\\p{Close_Punctuation}/\)/g; s/\\p{Initial_Punctuation}/\"/g; s/\\p{Final_Punctuation}/\"/g; s/\\p{Connector_Punctuation}/-/g; s/•/*/g; s/\\p{Currency_Symbol}/\\\$/g; s/\\p{Math_Symbol}/*/g; print \$_;' | $MXPOST/mxpost |");
open(OUT,">$OUT");
while(<TAGGER>) {
    foreach my $word_pos (split) {
	$word_pos =~ s/\/([^\/]+)$/_$1/;
	$word_pos = "//_:" if $word_pos eq "//";
	print STDERR "faulty POS tag: $word_pos\n" 
	    unless $word_pos =~ /^.+_([^_]+)$/;
	print OUT "$1 ";
    }
    print OUT "\n";
}
close(OUT);
close(TAGGER);

