#!/usr/bin/perl -w

#sentence-by-sentence: take in a system output, with any number of factors, and a reference translation, also maybe with factors, and show each sentence and its errors
#usage: sentence-by-sentence SYSOUT REFERENCE > sentences.html

use strict;

my ($sysoutfile, $truthfile) = @ARGV;
open(SYSOUT, "<$sysoutfile") or die "couldn't open '$sysoutfile' for read\n";
open(TRUTH, "<$truthfile") or die "couldn't open '$truthfile' for read\n";
my @bleuScores;
my @htmlSentences;
my @htmlColors = ('#99ff99', '#aaaaff', '#ffff99', '#ff9933', '#ff9999'); #color sentences by rank (split in n tiers)
my @ngramColors = ('#ffff99', '#ff9933', '#99ff99', '#aaaaff', '#ff99ff'); #to be colorful and tell consecutive n-grams apart on the display
my $i = 0;
while(my $sLine = <SYSOUT>)
{
	my $eLine = <TRUTH>;
	chop $sLine; chop $eLine;
	my @sWords = split(/\s+/, $sLine);
	my @eWords = split(/\s+/, $eLine);
	my @sFactors = map {my @f = split(/\|/, $_); \@f;} @sWords;
	my @eFactors = map {my @f = split(/\|/, $_); \@f;} @eWords;
	my $bleuData = getBLEUSentenceDetails(\@sFactors, \@eFactors, 0);
	push @bleuScores, [$i, $bleuData->[0]->[0], 0]; #the last number will be the rank
	my $pwerData = getPWERSentenceDetails(\@sFactors, \@eFactors, 0);
	my $html = "<div class=\"sentence\" style=\"background-color: %%%%\">"; #the %%%% is a flag to be replaced
	$html .= "<div class=\"bleu_report\"><b>BLEU:</b> " . sprintf("%.4lg", $bleuData->[0]->[0]) . " (" . join('/', map {sprintf("%.4lg", $_)} @{$bleuData->[0]}[1 .. 4]) . ")</div>\n";
	$html .= "<h2>Reference</h2><div class=\"truth_sentence\" id=\"truth$i\">" . getFactoredSentenceHTML(\@eFactors) . "</div>\n";
	my $j = 0;
	$html .= "<h2>System Output</h2><h4>(PWER errors marked)</h4><div class=\"sysout_sentence\" id=\"sysout$i\">" . getFactoredSentenceHTML(\@sFactors, $pwerData) . "</div>\n";
	$j = 0;
	$html .= "<h2>N-grams</h2><div class=\"sysout_ngrams\" id=\"ngrams$i\">" . getAllNgramsHTML(\@sFactors, $bleuData->[1]) . "</div>\n";
	$html .= "</div>\n";
	push @htmlSentences, $html;
	$i++;
}
close(SYSOUT);
close(TRUTH);

rankSentencesByBLEU(\@bleuScores);
my $stylesheet = <<EOHTML;
<style type="text/css">
h2 {font-weight: bold; font-size: large; margin-bottom: 12px}
h4 {font-weight: bold; font-size: small}
#legend {background-color: #fff; border: 1px solid #000; padding: 2px; margin-bottom: 15px}
#legend_title {font-weight: bold; font-size: medium; text-decoration: underline}
div.sentence {background-color: #ffffee; border: 1px solid #000088; padding: 0px 8px 0px 8px} //entire composition
div.bleu_report {margin-bottom: 5px}
div.truth_sentence {background-color: #ccffcc; border: 1px solid #bbb; margin: 8px 0px 8px 0px}
div.sysout_sentence {background-color: #ccccff; border: 1px solid #bbb; margin: 8px 0px 8px 0px}
table.sentence_table {border: none}
div.sysout_ngrams {background-color: #fff; border: 1px solid #bbb; margin-top: 8px; margin-bottom: 8px}
table.ngram_table {}
td.ngram_cell {padding: 1px}
</style>
EOHTML
print "<html><head><title>$sysoutfile vs. $truthfile: Sentence-by-sentence Comparison</title>$stylesheet</head><body>\n";

#legend for background colors
my @minBLEU = (1e9) x scalar(@htmlColors);
my @maxBLEU = (-1e9) x scalar(@htmlColors);
for(my $k = 0; $k < scalar(@htmlSentences); $k++)
{
	my $tier = int($bleuScores[$k]->[2] / (scalar(@htmlSentences) / scalar(@htmlColors)));
	if($bleuScores[$k]->[1] < $minBLEU[$tier]) {$minBLEU[$tier] = $bleuScores[$k]->[1];}
	elsif($bleuScores[$k]->[1] > $maxBLEU[$tier]) {$maxBLEU[$tier] = $bleuScores[$k]->[1];}
}
print "<div id=legend><span id=legend_title>BLEU Ranges</span><table border=0>";
for(my $k = 0; $k < scalar(@htmlColors); $k++)
{
	print "<tr><td style=\"width: 15px; height: 15px; background-color: " . $htmlColors[$k] . "\"></td><td align=left style=\"padding-left: 12px\">" 
							. sprintf("%.4lg", $minBLEU[$k]) . " - " . sprintf("%.4lg", $maxBLEU[$k]) . "</td>";
}
print "</table></div>\n";

#sentence boxes
my $j = 0;
foreach my $sentenceHTML (@htmlSentences)
{
	if($j > 0) {print "<hr width=98%>";}
	my $bgcolor = getSentenceBGColorHTML($bleuScores[$j], $i); #i is now # of sentences
	$sentenceHTML =~ s/%%%%/$bgcolor/;
	print "$sentenceHTML\n";
	$j++;
}
print "</body></html>";

##################### utils #####################

#arguments: a, b (scalars)
sub min
{
	my ($a, $b) = @_;
	return ($a < $b) ? $a : $b;
}
#arguments: a, b (scalars)
sub max
{
	my ($a, $b) = @_;
	return ($a > $b) ? $a : $b;
}
#arguments: x
sub my_log
{
  return -9999999999 unless $_[0];
  return log($_[0]);
}

###############################################################################################################################################################

#arguments: sysout sentence (arrayref of arrayrefs of factor strings), truth sentence (same), factor index to use
#return: arrayref of [arrayref of [overall BLEU score, n-gram precisions], arrayref of matching n-gram [start index, length]]
sub getBLEUSentenceDetails
{
	my ($refSysOutput, $refTruth, $factorIndex) = @_;
	my ($length_reference, $length_translation) = (scalar(@$refTruth), scalar(@$refSysOutput));
	my ($correct1, $correct2, $correct3, $correct4, $total1, $total2, $total3, $total4) = (0, 0, 0, 0, 0, 0, 0, 0);
	my $returnData = [[], []];
	my %REF_GRAM = ();
	my $ngramMatches = []; #arrayref of n-gram [start index, length]
	my ($i, $gram);
	for($i = 0; $i < $length_reference; $i++)
	{
		$gram = $refTruth->[$i]->[$factorIndex];
		$REF_GRAM{$gram}++;
		next if $i<1;
		$gram = $refTruth->[$i - 1]->[$factorIndex] ." ".$gram;
		$REF_GRAM{$gram}++;
      next if $i<2;
      $gram = $refTruth->[$i - 2]->[$factorIndex] ." ".$gram;
      $REF_GRAM{$gram}++;
      next if $i<3;
      $gram = $refTruth->[$i - 3]->[$factorIndex] ." ".$gram;
      $REF_GRAM{$gram}++;
	}
	for($i = 0; $i < $length_translation; $i++)
	{
      $gram = $refSysOutput->[$i]->[$factorIndex];
      if (defined($REF_GRAM{$gram}) && $REF_GRAM{$gram} > 0) {
			$REF_GRAM{$gram}--;
			$correct1++;
			push @$ngramMatches, [$i, 1];
      }
      next if $i<1;
      $gram = $refSysOutput->[$i - 1]->[$factorIndex] ." ".$gram;
      if (defined($REF_GRAM{$gram}) && $REF_GRAM{$gram} > 0) {
			$REF_GRAM{$gram}--;
			$correct2++;
			push @$ngramMatches, [$i - 1, 2];
      }
      next if $i<2;
      $gram = $refSysOutput->[$i - 2]->[$factorIndex] ." ".$gram;
      if (defined($REF_GRAM{$gram}) && $REF_GRAM{$gram} > 0) {
			$REF_GRAM{$gram}--;
			$correct3++;
			push @$ngramMatches, [$i - 2, 3];
      }
      next if $i<3;
      $gram = $refSysOutput->[$i - 3]->[$factorIndex] ." ".$gram;
      if (defined($REF_GRAM{$gram}) && $REF_GRAM{$gram} > 0) {
			$REF_GRAM{$gram}--;
			$correct4++;
			push @$ngramMatches, [$i - 3, 4];
      }
	}
	my $total = $length_translation;
	$total1 = max(1, $total);
	$total2 = max(1, $total - 1);
	$total3 = max(1, $total - 2);
	$total4 = max(1, $total - 3);
	my $brevity = ($length_translation > $length_reference || $length_translation == 0) ? 1 : exp(1 - $length_reference / $length_translation);
	my ($pct1, $pct2, $pct3, $pct4) = ($total1 == 0 ? -1 : $correct1 / $total1, $total2 == 0 ? -1 : $correct2 / $total2, 
													$total3 == 0 ? -1 : $correct3 / $total3, $total4 == 0 ? -1 : $correct4 / $total4);
	my ($logsum, $logcount) = (0, 0);
	if($total1 > 0) {$logsum += my_log($pct1); $logcount++;}
	if($total2 > 0) {$logsum += my_log($pct2); $logcount++;}
	if($total3 > 0) {$logsum += my_log($pct3); $logcount++;}
	if($total4 > 0) {$logsum += my_log($pct4); $logcount++;}
	my $bleu = $brevity * exp($logsum / $logcount);
	$returnData->[0] = [$bleu, $pct1, $pct2, $pct3, $pct4];
	$returnData->[1] = $ngramMatches;
	return $returnData;
}

#arguments: sysout sentence (arrayref of arrayrefs of factor strings), truth sentence (same), factor index to use
#return: hashref of sysout word index => whether word matches
sub getPWERSentenceDetails
{
	my ($refSysOutput, $refTruth, $factorIndex) = @_;
	my $indices = {};
	my ($sLength, $eLength) = (scalar(@$refSysOutput), scalar(@$refTruth));
	my @truthWordUsed = (0) x $eLength; #array of 0/1; can only match a given truth word once
	for(my $j = 0; $j < $sLength; $j++)
	{
		$indices->{$j} = 0;
		for(my $k = 0; $k < $eLength; $k++) #check output word against entire truth sentence
		{
			if(lc $refSysOutput->[$j]->[$factorIndex] eq lc $refTruth->[$k]->[$factorIndex] && $truthWordUsed[$k] == 0)
			{
				$truthWordUsed[$k] = 1;
				$indices->{$j} = 1;
				last;
			}
		}
	}
	return $indices;
}

#assign ranks to sentences by BLEU score
#arguments: arrayref of arrayrefs of [sentence index, bleu score, rank to be assigned]
#return: none
sub rankSentencesByBLEU
{
	my $bleuData = shift;
	my $i = 0;
	#sort first on score, secondarily on sentence index
	foreach my $sentenceData (reverse sort {my $c = $a->[1] <=> $b->[1]; if($c == 0) {$a->[0] cmp $b->[0];} else {$c;}} @$bleuData) {$sentenceData->[2] = $i++;}
}

###############################################################################################################################################################

#write HTML for a sentence containing factors (display words in a row)
#arguments: sentence  (arrayref of arrayrefs of factor strings), PWER results (hashref from word indices to 0/1 whether matched a truth word)
#return: HTML string
sub getFactoredSentenceHTML
{
	my $sentence = shift;
	my $pwer = 0; if(scalar(@_) > 0) {$pwer = shift;}
	my $html = "<table class=\"sentence_table\"><tr>";
	for(my $i = 0; $i < scalar(@$sentence); $i++) #loop over words
	{
		my $style = ''; #default
		if($pwer ne '0' && $pwer->{$i} == 0) {$style = 'color: #cc0000; font-weight: bold';}
		$html .= "<td align=center style=\"$style\">" . join("<br>", @{$sentence->[$i]}) . "</td>";
	}
	return $html . "</tr></table>";
}

#arguments: arrayref of [sentence index, bleu score, rank], number of sentences
#return: HTML color string
sub getSentenceBGColorHTML
{
	my ($scoreData, $numSentences) = @_;
	my $tier = int($scoreData->[2] / ($numSentences / scalar(@htmlColors))); #0..n-1
	return $htmlColors[$tier];
}

#display all matching n-grams in the given sentence, with all 1-grams on one line, then arranged by picking, for each, the first line on which it fits,
# where a given word position can only be filled by one n-gram per line, so that all n-grams can be shown
#arguments: sentence (arrayref of arrayrefs of factor strings), arrayref of arrayrefs of matching n-gram [start, length]
#return: HTML string
sub getAllNgramsHTML
{
	my ($sentence, $ngrams) = @_;
	my $factorIndex = 0;
	my @table = (); #array or arrayrefs each of which represents a line; each position has the index of the occupying n-gram, or -1 if none
	my $n = 0; #n-gram index
	foreach my $ngram (sort {$a->[0] <=> $b->[0]} @$ngrams)
	{
		#check for an open slot in an existing row
		my $foundRow = 0;
		my $r = 0;
		foreach my $row (@table)
		{
			if(rowIsClear($row, $ngram) == 1)
			{
				@{$row}[$ngram->[0] .. ($ngram->[0] + $ngram->[1] - 1)] = ($n) x $ngram->[1];
				push @$ngram, $r; #add row index
				$foundRow = 1;
				last;
			}
			$r++;
		}
		#add row if necessary
		if($foundRow == 0)
		{
			my @row = (-1) x scalar(@$sentence);
			@row[$ngram->[0] .. ($ngram->[0] + $ngram->[1] - 1)] = ($n) x $ngram->[1];
			push @$ngram, scalar(@table); #add row index
			push @table, \@row;
		}
		$n++;
	}
	
	my $html = "<table class=\"ngram_table\"><tr><td align=center>" . join("</td><td align=center>", map {$_->[$factorIndex]} @$sentence) . "</td></tr>";
	
	my $numWords = scalar(@$sentence);
	my ($curRow, $curCol) = (0, 0);
	my $colorIndex = 0;
	$html .= "<tr>";
	foreach my $ngram (sort {my $c = $a->[2] <=> $b->[2]; if($c == 0) {$a->[0] <=> $b->[0]} else {$c}} @$ngrams) #sort by row, then word num
	{
		while($ngram->[0] > $curCol || $ngram->[2] > $curRow) {$html .= "<td></td>"; $curCol = ($curCol + 1) % $numWords; if($curCol == 0) {$html .= "</tr><tr>"; $curRow++;}}
		$html .= "<td colspan=" . $ngram->[1] . " align=center class=\"ngram_cell\" style=\"background-color: " . $ngramColors[$colorIndex] . "\">" . join(' ', map {$_->[$factorIndex]} @{$sentence}[$ngram->[0] .. $ngram->[0] + $ngram->[1] - 1]) . "</td>";
		$colorIndex = ($colorIndex + 1) % scalar(@ngramColors);
		$curCol = ($curCol + $ngram->[1]) % $numWords; if($curCol == 0) {$html .= "</tr><tr>"; $curRow++;}
	}
	$html .= "</tr>";

	return $html . "</table>\n";
}

#auxiliary to getAllNgramsHTML(): check a table row for an empty piece at the right place for the given n-gram
#arguments: row (arrayref of ints), n-gram (arrayref of [start index, length])
#return: whether (0/1) row is clear
sub rowIsClear
{
	my ($row, $ngram) = @_;
	return (maxN(@{$row}[$ngram->[0] .. $ngram->[0] + $ngram->[1] - 1]) == -1) ? 1 : 0;
}

#arguments: array of numeric values
#return: max value, or empty list if input list is empty
sub maxN
{
	if(scalar(@_) == 0) {return ();}
	my $max = $_[0];
	for(my $i = 1; $i < scalar(@_); $i++)
	{
		if($_[$i] > $max) {$max = $_[$i];}
	}
	return $max;
}
