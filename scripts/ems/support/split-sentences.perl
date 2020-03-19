#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.
#
# Based on a preprocessor written by Philipp Koehn.

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

use warnings;
use FindBin qw($RealBin);
use strict;
use utf8;

my $mydir = "$RealBin/../../share/nonbreaking_prefixes";

my %NONBREAKING_PREFIX = ();
my $language = "en";
my $prefixfile = "";
my $is_cjk = 0;
my $QUIET = 0;
my $HELP = 0;
my $LIST_ITEM = 0;
my $NOP = 0;
my $KEEP_LINES = 0;

while (@ARGV) {
	$_ = shift;
	/^-l$/ && ($language = shift, next);
	/^-p$/ && ($prefixfile = shift, next);
	/^-q$/ && ($QUIET = 1, next);
	/^-h$/ && ($HELP = 1, next);
	/^-i$/ && ($LIST_ITEM = 1, next);
	/^-n$/ && ($NOP = 1, next);
	/^-k$/ && ($KEEP_LINES = 1, next);
	/^-b$/ && ($|++, next); # no output buffering
}

if ($HELP) {
	print "Usage ./split-sentences.perl (-l [en|de|...]) [-p prefix-file] [-q] [-b] < textfile > splitfile\n";
	print "-q: quiet mode\n";
	print "-b: no output buffering (for use in bidirectional pipes)\n";
	print "-p: use a custom prefix file, overriding the installed one\n";
	print "-i: avoid splitting on list items (e.g. 1. This is the first)\n";
	print "-n: do not emit <P> after paragraphs\n";
	print "-k: keep existing line boundaries\n";
	exit;
}
if (!$QUIET) {
	print STDERR "Sentence Splitter v3\n";
	print STDERR "Language: $language\n";
}

# Is it Chinese, Japanese, Korean?
if ($language eq "yue" || $language eq "zh" || $language eq "ja") {
	$is_cjk = 1;
}

if ($prefixfile ne "") {
	print STDERR "Loading non-breaking prefixes from $prefixfile\n";
} else {

    $prefixfile = "$mydir/nonbreaking_prefix.$language";

    # Default to English, if we don't have a language-specific prefix file.
    if (!(-e $prefixfile)) {
      $prefixfile = "$mydir/nonbreaking_prefix.en";
      print STDERR "WARNING: No known abbreviations for language '$language', attempting fall-back to English version...\n";
      die ("ERROR: No abbreviations files found in $mydir\n") unless (-e $prefixfile);
    }
}

if (-e "$prefixfile") {
	open(PREFIX, "<:utf8", "$prefixfile") or die "Cannot open: $!";
	while (<PREFIX>) {
		my $item = $_;
		chomp($item);
		if (($item) && (substr($item,0,1) ne "#")) {
			if ($item =~ /(.*)[\s]+(\#NUMERIC_ONLY\#)/) {
				$NONBREAKING_PREFIX{$1} = 2;
			} else {
				$NONBREAKING_PREFIX{$item} = 1;
			}
		}
	}
	close(PREFIX);
}

## Loop over text, add lines together until we get a blank line or a <p>
my $text = "";
while (<STDIN>) {
	chomp;
	if ($KEEP_LINES) {
		&do_it_for($_,"");
	} elsif (/^<.+>$/ || /^\s*$/) {
		# Time to process this block; we've hit a blank or <p>
		&do_it_for($text, $_);
		print "<P>\n" if $NOP == 0 && (/^\s*$/ && $text); ## If we have text followed by <P>
		$text = "";
	} else {
		# Append the text, with a space.
		$text .= $_. " ";
	}
}
# Do the leftover text.
&do_it_for($text,"") if $text;


sub do_it_for {
	my($text,$markup) = @_;
	print &preprocess($text) if $text;
	print "$markup\n" if ($markup =~ /^<.+>$/);
	#chop($text);
}

sub preprocess {
	# Argument is one paragraph.
	my($text) = @_;

	# Clean up spaces at head and tail of each line, as well as
	# any double-spacing.
	$text =~ s/ +/ /g;
	$text =~ s/\n /\n/g;
	$text =~ s/ \n/\n/g;
	$text =~ s/^ //g;
	$text =~ s/ $//g;

	##### Add sentence breaks as needed #####

	# Sentences can start with upper-case, numnbers,  or Indic characters
	my $sentence_start = "\\p{IsUpper}0-9";
	$sentence_start .= "\\p{Block: Devanagari}\\p{Block: Devanagari_Extended}" if ($language eq "hi" || $language eq "mr");
	$sentence_start .= "\\p{Block: Gujarati}" if $language eq "gu";
	$sentence_start .= "\\p{Block: Bengali}" if ($language eq "as" || $language eq  "bn" || $language eq "mni");
	$sentence_start .= "\\p{Block: Kannada}" if $language eq "kn";
	$sentence_start .= "\\p{Block: Malayalam}" if $language eq "ml";
	$sentence_start .= "\\p{Block: Oriya}" if $language eq "or";
	$sentence_start .= "\\p{Block: Gurmukhi}" if $language eq "pa";
	$sentence_start .= "\\p{Block: Tamil}" if $language eq "ta";
	$sentence_start .= "\\p{Block: Telugu}" if $language eq "te";
	$sentence_start .= "\\p{Block: Hangul}\\p{Block: Hangul_Compatibility_Jamo}\\p{Block: Hangul_Jamo}\\p{Block: Hangul_Jamo_Extended_A}\\p{Block: Hangul_Jamo_Extended_B}" if $language eq "ko";

	# we include danda and double danda (U+0964 and U+0965) as sentence split characters

	# Non-period end of sentence markers (?!) followed by sentence starters.
	$text =~ s/([?!؟\x{0964}\x{0965}]) +([\'\"\(\[\¿\¡\p{IsPi}]*[$sentence_start])/$1\n$2/g;

	# Multi-dots followed by sentence starters.
	$text =~ s/(\.[\.]+) +([\'\"\(\[\¿\¡\p{IsPi}]*[$sentence_start])/$1\n$2/g;

	# Add breaks for sentences that end with some sort of punctuation
	# inside a quote or parenthetical and are followed by a possible
	# sentence starter punctuation and upper case.
	$text =~ s/([?!؟\.\x{0964}\x{0965}][\ ]*[\x{300d}\x{300f}\'\"\)\]\p{IsPf}]+) +([\'\"\(\[\¿\¡\p{IsPi}]*[\ ]*[$sentence_start])/$1\n$2/g;

	# Add breaks for sentences that end with some sort of punctuation,
	# and are followed by a sentence starter punctuation and upper case.
	$text =~ s/([?!؟\.\x{0964}\x{0965}]) +([\x{300d}\x{300f}\'\"\(\[\¿\¡\p{IsPi}]+[\ ]*[$sentence_start])/$1\n$2/g;


	#NOTE: Korean no longer handled here, cos Korean has spaces.
	if ($is_cjk == 1) {
		# Chinese uses unusual end-of-sentence markers. These are NOT
		# followed by whitespace.  Nor is there any idea of capitalization.
		# There does not appear to be any unicode category for full-stops
		# in general, so list them here.  U+3002 U+FF0E U+FF1F U+FF01
		#$text =~ s/([。．？！♪])/$1\n/g;
		$text =~ s/([\x{3002}\x{ff0e}\x{FF1F}\x{FF01}]+\s*["\x{201d}\x{201e}\x{300d}\x{300f}]?\s*)/$1\n/g;

		# A normal full-stop or other Western sentence enders followed
		# by an ideograph is an end-of-sentence, always.
		$text =~ s/([\.?!؟]) *(\p{CJK})/$1\n$2/g;

		# Split close-paren-then-comma into two.
		$text =~ s/(\p{Punctuation}) *(\p{Punctuation})/ $1 $2 /g;

		# Chinese does not use any sort of white-space between ideographs.
		# Nominally, each single ideograph corresponds to one word. Add
		# spaces here, so that later processing stages can tokenize readily.
		# Note that this handles mixed latinate+CJK.
		# TODO: perhaps also CJKExtA CJKExtB etc ??? CJK_Radicals_Sup ?

		# bhaddow - Comment this out since it adds white-space between Chinese characters. This is not
		# what we want from sentence-splitter!
		#$text =~ s/(\p{Punctuation}) *(\p{CJK})/ $1 $2/g;
		#$text =~ s/(\p{CJK}) *(\p{Punctuation})/$1 $2 /g;
		#$text =~ s/([\p{CJK}\p{CJKSymbols}])/ $1 /g;
		#$text =~ s/ +/ /g;
	}

	# Urdu support
	# https://en.wikipedia.org/wiki/Urdu_alphabet#Encoding_Urdu_in_Unicode
	if ($language eq 'ur') {
	$text =~ s{
	        ( (?: [\.\?!\x{06d4}] | \.\.+ )
	          [\'\"\x{201e}\x{bb}\(\[\¿\¡\p{IsPf}]*
	          )
	        \s+
	        ( [\'\"\x{201e}\x{bb}\(\[\¿\¡\p{IsPi}]*
	          [\x{0600}-\x{06ff}]
	          )
	    }{$1\n$2}gx;
	}

	# Special punctuation cases are covered. Check all remaining periods.
	my $word;
	my $i;
	my @words = split(/\h/,$text);
	#print "NOW $text\n";
	$text = "";
	for ($i=0;$i<(scalar(@words)-1);$i++) {
	#print "Checking $words[$i] $words[$i+1]\n";
		if ($words[$i] =~ /([\p{IsAlnum}\.\-]*)([\'\"\)\]\%\p{IsPf}]*)(\.+)$/) {
			# Check if $1 is a known honorific and $2 is empty, never break.
			my $prefix = $1;
			my $starting_punct = $2;
			if ($prefix && $NONBREAKING_PREFIX{$prefix} && $NONBREAKING_PREFIX{$prefix} == 1 && !$starting_punct) {
				# Not breaking;
                ## print "NBP1 $words[$i] $words[$i+1]\n";
			} elsif ($words[$i] =~ /(\.)[\p{IsUpper}\-]+(\.+)$/) {
				# Not breaking - upper case acronym
                #print "NBP2 $words[$i] $words[$i+1]\n";
      } elsif ($LIST_ITEM
             && ($i == 0 || substr($words[$i-1], -1) eq "\n")
             && $words[$i] =~ /^\(?(([0-9]+)|([ivx]+)|([A-Za-z]))\)?\.$/) {
        	 	# Maybe list item - non breaking
 				#print "NBP3 $words[$i] $words[$i+1]\n";
			} elsif($words[$i+1] =~ /^([ ]*[\'\"\(\[\¿\¡\p{IsPi}]*[ ]*[0-9$sentence_start])/) {
				# The next word has a bunch of initial quotes, maybe a
				# space, then either upper case or a number
                #print "MAYBE $words[$i] $words[$i+1]\n";
				$words[$i] = $words[$i]."\n" unless ($prefix && $NONBREAKING_PREFIX{$prefix} && $NONBREAKING_PREFIX{$prefix} == 2 && !$starting_punct && ($words[$i+1] =~ /^[0-9]+/));
				# We always add a return for these, unless we have a
				# numeric non-breaker and a number start.
			}
		}
		$text = $text.$words[$i]." ";
	}

	# We stopped one token from the end to allow for easy look-ahead.
	# Append it now.
	$text = $text.$words[$i];

	# Clean up spaces at head and tail of each line as well as any double-spacing
	$text =~ s/ +/ /g;
	$text =~ s/\n /\n/g;
	$text =~ s/ \n/\n/g;
	$text =~ s/^ //g;
	$text =~ s/ $//g;

	# Add trailing break.
	$text .= "\n" unless $text =~ /\n$/;

	return $text;
}
