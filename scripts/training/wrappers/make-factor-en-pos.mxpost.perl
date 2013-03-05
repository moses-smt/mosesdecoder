#!/usr/bin/perl -w

use strict;
use FindBin qw($RealBin);

use Getopt::Long "GetOptions";
my ($IN,$OUT,$MXPOST);
if (!&GetOptions('mxpost=s' => \$MXPOST) ||
    !($IN = shift @ARGV) ||
    !($OUT = shift @ARGV) ||
    !defined($MXPOST)) {
        print "syntax: make-pos-en.mxpost.perl -mxpost INSTALL_DIR IN_FILE OUT_FILE\n";
        exit(1);
}

my $pipeline = "perl -ne 'chop; tr/\\x20-\\x7f/\?/c; print \$_.\"\\n\";' | tee debug | ";
$pipeline .= "$MXPOST/mxpost $MXPOST/tagger.project |";
open(TAGGER,"$RealBin/../../tokenizer/deescape-special-chars.perl < $IN | $pipeline");
open(OUT,"| $RealBin/../../tokenizer/escape-special-chars.perl > $OUT");
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

