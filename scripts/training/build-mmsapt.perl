#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";
use FindBin qw($RealBin);

my ($DIR,$F,$E,$ALIGNMENT,$CORPUS,$SETTINGS);
die("ERROR: syntax is --alignment FILE --corpus FILESTEM --f EXT --e EXT --DIR OUTDIR --settings STRING")
    unless &GetOptions('DIR=s' => \$DIR,
		       'f=s' => \$F,
		       'e=s' => \$E,
		       'corpus=s' => \$CORPUS,
		       'alignment=s' => \$ALIGNMENT,
		       'settings=s' => \$SETTINGS)
	   && defined($DIR) && defined($F) && defined($E) && defined($CORPUS) && defined($ALIGNMENT)
           && -e $ALIGNMENT && -e "$CORPUS.$F" && -e "$CORPUS.$E";

`mkdir $DIR`;
`$RealBin/../../bin/mtt-build < $CORPUS.$F -i -o $DIR/$F`;
`$RealBin/../../bin/mtt-build < $CORPUS.$E -i -o $DIR/$E`;
`$RealBin/../../bin/symal2mam < $ALIGNMENT $DIR/$F-$E.mam`;
`$RealBin/../../bin/mmlex-build $DIR/ $F $E -o $DIR/$F-$E.lex -c $DIR/$F-$E.cooc`;

