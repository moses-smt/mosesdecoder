#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;

# handle switches
use Getopt::Long "GetOptions";
my ($IN,$OUT,$TREE_TAGGER,$BASIC,$STEM,$LANGUAGE);
if (!&GetOptions('tree-tagger=s' => \$TREE_TAGGER,
                 'basic' => \$BASIC,
                 'stem' => \$STEM,
                 'l=s' => \$LANGUAGE) ||
    !($IN = shift @ARGV) ||
    !($OUT = shift @ARGV) ||
    !defined($TREE_TAGGER) ||
    !defined($LANGUAGE)) {
	print "syntax: make-pos.tree-tagger.perl -tree-tagger INSTALL_DIR -l LANGUAGE IN_FILE OUT_FILE [-basic] [-stem]\n";
	exit(1);
}

# define the model file for the given language
my $MODEL = undef;
$MODEL = "english" if $LANGUAGE eq "en";
$MODEL = "french-utf8" if $LANGUAGE eq "fr";
$MODEL = "spanish" if $LANGUAGE eq "es";
$MODEL = "german-utf8" if $LANGUAGE eq "de";
$MODEL = "italian-utf8" if $LANGUAGE eq "it";
$MODEL = "dutch" if $LANGUAGE eq "nl";
$MODEL = "bulgarian-utf8" if $LANGUAGE eq "bg";
$MODEL = "greek" if $LANGUAGE eq "el";
die("Unknown language '$LANGUAGE'") unless defined($MODEL);
$MODEL = $TREE_TAGGER."/lib/".$MODEL.".par";

# define encoding conversion into Latin1 or Greek if required
my $CONV = "";
#$CONV = "iconv --unicode-subst=X -f utf8 -t iso-8859-1|"
$CONV = "perl -ne 'use Encode; print encode(\"iso-8859-1\", decode(\"utf8\", \$_));' |"
	unless $MODEL =~ /utf8/ || $LANGUAGE eq "bg";
$CONV = "perl -ne 'use Encode; print encode(\"iso-8859-7\", decode(\"utf8\", \$_));' |"
	if $LANGUAGE eq "el";

# pipe in data into tagger, process its output
my $first = 1;
open(TAGGER,"cat $IN | $CONV".
            "perl -ne 'foreach(split){print \$_.\"\n\";}print \"eND_oF_SeNTeNCe\n\";'|".
            "$TREE_TAGGER/bin/tree-tagger -token -lemma -sgml $MODEL|");
open(OUT,">$OUT");
while(<TAGGER>) {
	my ($word,$tag,$stem) = split;
	if ($word eq "eND_oF_SeNTeNCe") {
		print OUT "\n";
		$first = 1;
	}
	else {
		print OUT " " unless $first;
		if ($STEM) {
			$stem = $word if $stem eq "<unknown>";
			$stem =~ s/\|.+//;
			print OUT $stem;
		}
		else {
			$tag =~ s/\:.+// if $BASIC;
			print OUT $tag;
		}
		$first = 0;
	}
}
close(TAGGER);
