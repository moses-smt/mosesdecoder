#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# $Id$
#show-phrases-used: display all source and target phrases for each sentence in a corpus, and give average phrase length used
#usage: show-phrases-used DECODER_OUTFILE > output.html
#  where DECODER_OUTFILE is the output of moses with the -T (show alignments) option

use warnings;
use strict;

BEGIN
{
    my $wd= `pawd 2>/dev/null`;
    if (!$wd) {$wd = `pwd`;}
    chomp $wd;
	push @INC, "$wd/perllib/sun4-solaris"; #for GD; if not an absolute path, Polygon.pm throws a fit
}
use lib "perllib/sun4-solaris/auto/GD";
use GD;
use GD::Image;
use GD::Polygon;

#parse decoder output
my $infilename = shift @ARGV;
open(INPUT, "<$infilename") or die "couldn't open '$infilename' for read: $!\n"; #stdin
my @sentenceData = ([], [], [], []); #(phrases used; dropped words (as factor arrays); average phrase lengths; phrase max char counts per word) per sentence
my $curSentence = -1;
my @numSrcChars = (); #used for alignment in image
my @numTgtChars = ();
my @numSrcPhrases = ();
my @numTgtPhrases = ();
my ($sentenceSrcChars, $sentenceTgtChars, $sentenceSrcPhrases, $sentenceTgtPhrases, $tgtWords);
my $mode = 'none';
while(my $line = <INPUT>)
{
	$mode = 'none' if $line =~ /^\s*\n$/; #a blank line ends any section
	if($line =~ /TRANSLATION HYPOTHESIS DETAILS:/) #first useful info printed per sentence, so we have a new sentence
	{
		$mode = 'opts';
		if($curSentence > -1)
		{
			push @numSrcChars, $sentenceSrcChars;
			push @numTgtChars, $sentenceTgtChars;
			push @numSrcPhrases, $sentenceSrcPhrases;
			push @numTgtPhrases, $sentenceTgtPhrases;
		}
		$curSentence++;
		$sentenceSrcChars = 0; $sentenceTgtChars = 0;
		$sentenceSrcPhrases = 0; $sentenceTgtPhrases = 0;
		$tgtWords = 0;
		push @{$sentenceData[0]}, [];
		push @{$sentenceData[1]}, [];
		push @{$sentenceData[2]}, 0;
	}
	elsif($line =~ /SOURCE\/TARGET SPANS:/) {$mode = 'spans';} #redundant info; we won't check this mode below
	elsif($line =~ /WORDS\/PHRASES DROPPED:/) {$mode = 'drops';}
	#read info when in a mode
	elsif($mode eq 'opts')
	{
		die "can't parse translation-options info for sentence $curSentence" unless $line =~ /SOURCE:\s+\[(\d+)\.\.(\d+)\]\s+(\S(.*\S)?)\s*$/;
		my %details;
		$details{'srcStart'} = $1;
		$details{'srcEnd'} = $2;
		my @srcFactors = map {my @f = split(/\|/, $_); \@f;} (split(/\s+/, $3));
		$details{'srcText'} = \@srcFactors;
		$sentenceData[2]->[$curSentence] += $2 - $1 + 1; #build sum of phrase lengths
		$details{'srcNumChars'} = 0;
		foreach my $word (@srcFactors) {$details{'srcNumChars'} += maxN(map {length($_)} @$word) + 1;} #+1 accounts for interword spacing
		$sentenceSrcChars += --$details{'srcNumChars'};
		$sentenceSrcPhrases++;
		$line = <INPUT>;
		die "can't parse translation-options info for sentence $curSentence" unless $line =~ /TRANSLATED AS:\s+(\S(.*\S)?)\s*$/;
		my @words = split(/\s+/, $1);
		if($words[0] eq '<EPSILON>') {@words = ();} #source phrase was dropped
		else {$sentenceTgtPhrases++;}
		my @tgtFactors = map {my @f = split(/\|/, $_); \@f;} (split(/\s+/, $1));
		$details{'tgtText'} = \@tgtFactors;
		$details{'tgtNumChars'} = 0;
		foreach my $word (@tgtFactors) {$details{'tgtNumChars'} += maxN(map {length($_)} @$word) + 1;} #+1 accounts for interword spacing
		$sentenceTgtChars += --$details{'tgtNumChars'};
		$details{'tgtStart'} = $tgtWords;
		$tgtWords += scalar(@words);
		$details{'tgtEnd'} = $tgtWords - 1;
		push @{$sentenceData[0]->[$curSentence]}, \%details;
	}
	elsif($mode eq 'drops') #redundant info; read it just because we can (stickin' it to the man!)
	{
		die "can't parse dropped-words info for sentence $curSentence\n" unless $line =~ /\s*(\S.*\S)\s*/;
		my @factors = split(/\|/, $1);
		push @{$sentenceData[1]->[$curSentence]}, \@factors;
	}
}
close(INPUT);
#stats for final sentence, since now we know when it ended
push @numSrcChars, $sentenceSrcChars;
push @numTgtChars, $sentenceTgtChars;
push @numSrcPhrases, $sentenceSrcPhrases;
push @numTgtPhrases, $sentenceTgtPhrases;

#calculate phrase-length statistics
my ($totalPhraseLength, $totalNumPhrases) = (0, 0);
for(my $i = 0; $i < scalar(@{$sentenceData[0]}); $i++)
{
	$totalPhraseLength += $sentenceData[2]->[$i];
	$totalNumPhrases += scalar(@{$sentenceData[0]->[$i]});
	$sentenceData[2]->[$i] /= scalar(@{$sentenceData[0]->[$i]});
}

##### create an image with colored phrases and arrows for each sentence #####
die "infilename ends in slash! should not be a directory\n" if $infilename !~ /\/([^\/]+)$/;
my $imgdir = "phraseImgs-tmp/${1}_" . time;
`mkdir -p $imgdir`; #-p => create recursively as necessary
my ($srcNumFactors, $tgtNumFactors) = (scalar(@{$sentenceData[0]->[0]->[0]->{'srcText'}->[0]}), scalar(@{$sentenceData[0]->[0]->[0]->{'tgtText'}->[0]}));
my $font = gdLargeFont;
my ($topMargin, $bottomMargin, $leftMargin, $rightMargin) = (1, 1, 1, 1); #extra pixels of background color
my $phraseEdgeHSpace = int($font->width / 2); #number of boundary pixels at horizontal edges of each phrase
my $phraseEdgeVSpace = 1; #number of boundary pixels at vertical edges
my $middleVSpace = $font->height + 6; #vertical pixels used to connect phrase color boxes

#precompute arrays of pixel coords
my $srcY = $topMargin + $phraseEdgeVSpace;
my @srcFactorYs;
for(my $i = 0; $i < $srcNumFactors; $i++) {push @srcFactorYs, $srcY + ($font->height + $phraseEdgeVSpace) * $i;}
my @tgtFactorYs;
my $tgtY = $srcY + ($font->height + $phraseEdgeVSpace) * $srcNumFactors + $middleVSpace + $phraseEdgeVSpace;
for(my $i = 0; $i < $tgtNumFactors; $i++) {push @tgtFactorYs, $tgtY + ($font->height + $phraseEdgeVSpace) * $i;}

for(my $i = 0; $i < scalar(@{$sentenceData[0]}); $i++)
{
	#create image
	my $img = new GD::Image($leftMargin + $rightMargin + max($font->width * $numSrcChars[$i] + $numSrcPhrases[$i] * 2 * $phraseEdgeHSpace,
																				$font->width * $numTgtChars[$i] + $numTgtPhrases[$i] * 2 * $phraseEdgeHSpace),
									$topMargin + $bottomMargin + $middleVSpace + ($font->height + $phraseEdgeVSpace) * ($srcNumFactors + $tgtNumFactors) + 2 * $phraseEdgeVSpace);
	#allocate colors
	my $white = $img->colorAllocate(255, 255, 255);
	$img->transparent($white); #set white to be transparent
	my $black = $img->colorAllocate(0, 0, 0);
	my $highlightCol = $img->colorAllocate(255, 0, 0); #used to show deleted source phrases
	my @bgCols = #alternating phrase background colors
	(
		$img->colorAllocate(165, 255, 138), #green
		$img->colorAllocate(237, 239, 133), #yellow
		$img->colorAllocate(255, 200, 72), #tan
		$img->colorAllocate(255, 172, 98), #orange
		$img->colorAllocate(255, 151, 151), #red
		$img->colorAllocate(254, 152, 241), #purple
		$img->colorAllocate(170, 170, 255), #blue
		$img->colorAllocate(165, 254, 250) #cyan
	);
	$img->setThickness(2); #for lines connecting phrases
	#get order of source phrases and source-phrase background colors
	my @srcPhraseIndices = (); #in each position, which phrase will be displayed next
	my @srcBGCols = () x $numSrcPhrases[$i]; #indices into bgCols
	my $nextWord = 0; #current starting index we're looking for
	while(scalar(@srcPhraseIndices) < $numSrcPhrases[$i])
	{
		for(my $k = 0; $k < $numSrcPhrases[$i]; $k++)
		{
			if($sentenceData[0]->[$i]->[$k]->{'srcStart'} == $nextWord)
			{
				$srcBGCols[$k] = scalar(@srcPhraseIndices) % scalar(@bgCols);
				push @srcPhraseIndices, $k;
				$nextWord = $sentenceData[0]->[$i]->[$k]->{'srcEnd'} + 1;
				last;
			}
		}
	}
	#calculate source-phrase pixel addresses
	my @srcStartX = () x $numSrcPhrases[$i];
	my $srcX = $leftMargin; #in pixels
	for(my $j = 0; $j < $numSrcPhrases[$i]; $j++)
	{
		$srcStartX[$j] = $srcX;
		$srcX += $font->width * $sentenceData[0]->[$i]->[$j]->{'srcNumChars'} + 2 * $phraseEdgeHSpace;
	}
	#get target-phrase pixel coords
	my @tgtStartX = () x $numSrcPhrases[$i]; #elements belonging to deleted source phrases simply aren't used
	my $tgtX = $leftMargin; #in pixels
	for(my $j = 0; $j < $numSrcPhrases[$i]; $j++)
	{
		my $k = $srcPhraseIndices[$j];
		if(length($sentenceData[0]->[$i]->[$k]->{'tgtText'}) > 0) #non-empty target phrase
		{
			$tgtStartX[$j] = $tgtX;
			$tgtX += $font->width * $sentenceData[0]->[$i]->[$k]->{'tgtNumChars'} + 2 * $phraseEdgeHSpace;
		}
	}
	#background
	$img->filledRectangle(0, 0, $img->width, $img->height, $white);
	#write phrase pairs
	for(my $j = 0; $j < $numSrcPhrases[$i]; $j++)
	{
		my $k = $srcPhraseIndices[$j];
		my $srcBottomY = $srcY + ($font->height + $phraseEdgeVSpace) * $srcNumFactors; #bottom of color
		$img->filledRectangle($srcStartX[$k], $srcY - $phraseEdgeVSpace, $srcStartX[$k] + $font->width * $sentenceData[0]->[$i]->[$k]->{'srcNumChars'} + 2 * $phraseEdgeHSpace,
									$srcBottomY, $bgCols[$srcBGCols[$k]]);
		if(length $sentenceData[0]->[$i]->[$k]->{'tgtText'} > 0) #non-empty target phrase
		{
			$img->filledRectangle($tgtStartX[$j], $tgtY - $phraseEdgeVSpace, $tgtStartX[$j] + $font->width * $sentenceData[0]->[$i]->[$k]->{'tgtNumChars'} + 2 * $phraseEdgeHSpace,
										$tgtY + ($font->height + $phraseEdgeVSpace) * $tgtNumFactors, $bgCols[$srcBGCols[$k]]);
			my ($srcMidX, $tgtMidX) = ($srcStartX[$k] + $font->width * $sentenceData[0]->[$i]->[$k]->{'srcNumChars'} / 2 + $phraseEdgeHSpace,
												$tgtStartX[$j] + $font->width * $sentenceData[0]->[$i]->[$k]->{'tgtNumChars'} / 2 + $phraseEdgeHSpace);
			$img->line($srcMidX, $srcBottomY, $tgtMidX, $tgtY, $bgCols[$srcBGCols[$k]]);
			writeFactoredStringGD($img, $srcStartX[$k] + $phraseEdgeHSpace, \@srcFactorYs, $sentenceData[0]->[$i]->[$k]->{'srcText'}, $font, $black);
			writeFactoredStringGD($img, $tgtStartX[$j] + $phraseEdgeHSpace, \@tgtFactorYs, $sentenceData[0]->[$i]->[$k]->{'tgtText'}, $font, $black);
		}
		else #empty target phrase; only show source
		{
			writeFactoredStringGD($img, $srcStartX[$k] + $phraseEdgeHSpace, \@srcFactorYs, $sentenceData[0]->[$i]->[$k]->{'srcText'}, $font, $highlightCol);
		}
	}
	#write image file
	my $imgfilename = "$imgdir/$i.png";
	open(IMAGE, ">$imgfilename") or die "couldn't create tmp image '$imgfilename': $!\n";
	print IMAGE $img->png();
	close(IMAGE);
}

#display HTML output
my $stylesheet = <<EOHTML;
<style type="text/css">
div.sentence {}
</style>
EOHTML
print "<html><head><title>Translation Options Used</title>$stylesheet</head><body>\n";
print "<span style=\"font-size:large\"><b>Overall Average Phrase Length:</b> " . sprintf("%.3lf", $totalPhraseLength / $totalNumPhrases) . "</span><p>\n";
for(my $i = 0; $i < scalar(@{$sentenceData[0]}); $i++)
{
	if($i > 0) {print "<hr width=98%>";}
	print "<div class=\"sentence\"><b>Average Phrase Length:</b> " . sprintf("%.3lf", $sentenceData[2]->[$i]) . "<p><img src=\"$imgdir/$i.png\"></div>\n";
}
print "</body></html>";

############################################################################################################################################

#2-argument
sub max
{
	my ($a, $b) = @_;
	return ($a > $b) ? $a : $b;
}

#N-argument
sub maxN
{
	die "maxN(): empty array!\n" if scalar(@_) == 0;
	my $max = shift @_;
	map {$max = $_ if $_ > $max;} @_;
	return $max;
}

#arguments: image, startX, arrayref of y-coords for writing, arrayref of arrayrefs of factor strings, font, color
sub writeFactoredStringGD
{
	my ($img, $startX, $ys, $factors, $font, $color) = @_;
	foreach my $word (@$factors)
	{
		for(my $i = 0; $i < scalar(@$ys); $i++)
		{
			$img->string($font, $startX, $ys->[$i], $word->[$i], $color);
		}
		$startX += $font->width * (maxN(map {length($_)} @$word) + 1);
	}
}
