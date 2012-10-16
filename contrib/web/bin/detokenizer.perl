#!/usr/bin/perl -w

# Sample De-Tokenizer
# written by Josh Schroeder, based on code by Philipp Koehn
#
# This script is now outdated, use scripts/tokenizer/detokenizer.perl instead

# This added by Herve Saint-Amand for compatibility with translate.cgi
$|++;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
use strict;

my $language = "en";
my $QUIET = 0;
my $HELP = 0;

while (@ARGV) {
	$_ = shift;
	/^-l$/ && ($language = shift, next);
	/^-q$/ && ($QUIET = 1, next);
	/^-h$/ && ($HELP = 1, next);
}

if ($HELP) {
	print "Usage ./detokenizer.perl (-l [en|de|...]) < tokenizedfile > detokenizedfile\n";
	exit;
}
if (!$QUIET) {
	print STDERR "Detokenizer Version 1.0\n";
	print STDERR "Language: $language\n";
}

while(<STDIN>) {
	if (/^<.+>$/ || /^\s*$/) {
		#don't try to detokenize XML/HTML tag lines
		print $_;
	}
	else {
		print &detokenize($_);
	}
}

sub detokenize {
	my($text) = @_;
	chomp($text);
	$text = " $text ";
	
	my $word;
	my $i;
	my @words = split(/ /,$text);
	$text = "";
	my %quoteCount =  ("\'"=>0,"\""=>0);
	my $prependSpace = " ";
	for ($i=0;$i<(scalar(@words));$i++) {		
		if ($words[$i] =~ /^[\p{IsSc}\(\[\{\¿\¡]+$/) {
			#perform right shift on currency and other random punctuation items
			$text = $text.$prependSpace.$words[$i];
			$prependSpace = "";
		} elsif ($words[$i] =~ /^[\,\.\?\!\:\;\\\%\}\]\)]+$/){
			#perform left shift on punctuation items
			$text=$text.$words[$i];
			$prependSpace = " ";
		} elsif (($language eq "en") && ($i>0) && ($words[$i] =~ /^[\'][\p{IsAlpha}]/) && ($words[$i-1] =~ /[\p{IsAlnum}]$/)) {
			#left-shift the contraction for English
			$text=$text.$words[$i];
			$prependSpace = " ";
		}  elsif (($language eq "fr") && ($i<(scalar(@words)-2)) && ($words[$i] =~ /[\p{IsAlpha}][\']$/) && ($words[$i+1] =~ /^[\p{IsAlpha}]/)) {
			#right-shift the contraction for French
			$text = $text.$prependSpace.$words[$i];
			$prependSpace = "";
		} elsif ($words[$i] =~ /^[\'\"]+$/) {
			#combine punctuation smartly
			if (($quoteCount{$words[$i]} % 2) eq 0) {
				if(($language eq "en") && ($words[$i] eq "'") && ($i > 0) && ($words[$i-1] =~ /[s]$/)) {
					#single quote for posesssives ending in s... "The Jones' house"
					#left shift
					$text=$text.$words[$i];
					$prependSpace = " ";
				} else {
					#right shift
					$text = $text.$prependSpace.$words[$i];
					$prependSpace = "";
					$quoteCount{$words[$i]} = $quoteCount{$words[$i]} + 1;

				}
			} else {
				#left shift
				$text=$text.$words[$i];
				$prependSpace = " ";
				$quoteCount{$words[$i]} = $quoteCount{$words[$i]} + 1;

			}
			
		} else {
			$text=$text.$prependSpace.$words[$i];
			$prependSpace = " ";
		}
	}
	
	# clean up spaces at head and tail of each line as well as any double-spacing
	$text =~ s/ +/ /g;
	$text =~ s/\n /\n/g;
	$text =~ s/ \n/\n/g;
	$text =~ s/^ //g;
	$text =~ s/ $//g;
	
	#add trailing break
	$text .= "\n" unless $text =~ /\n$/;

	return $text;
}

