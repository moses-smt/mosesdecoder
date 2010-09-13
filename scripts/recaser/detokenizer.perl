#!/usr/bin/perl -w

# Sample De-Tokenizer
# written by Josh Schroeder, based on code by Philipp Koehn
# further modifications by Ondrej Bojar

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
use strict;
use utf8; # tell perl this script file is in UTF-8 (see all funny punct below)

my $language = "en";
my $QUIET = 0;
my $HELP = 0;
my $UPPERCASE_SENT = 0;

while (@ARGV) {
	$_ = shift;
	/^-l$/ && ($language = shift, next);
	/^-q$/ && ($QUIET = 1, next);
	/^-h$/ && ($HELP = 1, next);
	/^-u$/ && ($UPPERCASE_SENT = 1, next);
}

if ($HELP) {
	print "Usage ./detokenizer.perl (-l [en|fr|cs]) < tokenizedfile > detokenizedfile\n";
        print "Options:\n";
        print "  -u  ... uppercase the first char in the final sentence.\n";
        print "  -q  ... don't report detokenizer revision.\n";
	exit;
}

die "No built-in rules for language $language, claim en for default behaviour."
	if $language !~ /^(cs|en|fr)$/;

if (!$QUIET) {
	print STDERR "Detokenizer Version ".'$Revision: 2846 $'."\n";
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


sub ucsecondarg {
  my $arg1 = shift;
  my $arg2 = shift;
  return $arg1.uc($arg2);
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
		} elsif (($language eq "fr") && ($i<(scalar(@words)-2)) && ($words[$i] =~ /[\p{IsAlpha}][\']$/) && ($words[$i+1] =~ /^[\p{IsAlpha}]/)) {
			#right-shift the contraction for French
			$text = $text.$prependSpace.$words[$i];
			$prependSpace = "";
		} elsif (($language eq "cs") && ($i<(scalar(@words)-3))
				&& ($words[$i] =~ /[\p{IsAlpha}]$/)
				&& ($words[$i+1] =~ /^[-–]$/)
				&& ($words[$i+2] =~ /^li$/i)
				) {
			#right-shift "-li" in Czech
			$text = $text.$prependSpace.$words[$i].$words[$i+1];
			$i++; # advance over the dash
			$prependSpace = "";
		} elsif ($words[$i] =~ /^[\'\"„“`]+$/) {
			#combine punctuation smartly
                        my $normalized_quo = $words[$i];
                        $normalized_quo = '"' if $words[$i] =~ /^[„“”]+$/;
                        $quoteCount{$normalized_quo} = 0
                                if !defined $quoteCount{$normalized_quo};
                        if ($language eq "cs" && $words[$i] eq "„") {
                          # this is always the starting quote in Czech
                          $quoteCount{$normalized_quo} = 0;
                        }
                        if ($language eq "cs" && $words[$i] eq "“") {
                          # this is usually the ending quote in Czech
                          $quoteCount{$normalized_quo} = 1;
                        }
			if (($quoteCount{$normalized_quo} % 2) eq 0) {
				if(($language eq "en") && ($words[$i] eq "'") && ($i > 0) && ($words[$i-1] =~ /[s]$/)) {
					#single quote for posesssives ending in s... "The Jones' house"
					#left shift
					$text=$text.$words[$i];
					$prependSpace = " ";
				} else {
					#right shift
					$text = $text.$prependSpace.$words[$i];
					$prependSpace = "";
					$quoteCount{$normalized_quo} ++;

				}
			} else {
				#left shift
				$text=$text.$words[$i];
				$prependSpace = " ";
				$quoteCount{$normalized_quo} ++;

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

        $text =~ s/^([[:punct:]\s]*)([[:alpha:]])/ucsecondarg($1, $2)/e if $UPPERCASE_SENT;

	return $text;
}

