#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# $Id$
#sentence-by-sentence: take in a system output, with any number of factors, and a reference translation, also maybe with factors, and show each sentence and its errors
#usage: sentence-by-sentence SYSOUT [REFERENCE]+ > sentences.html

use warnings;
use strict;
use Getopt::Long;

my $sourcefile = undef;
my @truthfiles;
GetOptions(
	"source|s=s" => \$sourcefile,
	"reference|r=s" => \@truthfiles
) or exit(1);

my @sysoutfiles = @ARGV;
if (scalar(@sysoutfiles) == 0 || scalar(@truthfiles) == 0)
{
	print STDERR "usage: $0 system_output(s) > sentence-by-sentence.html
Options:
  --source,-s STRING      foreign input (can be used multiple times)
  --reference,-r STRING   English truth (can be used multiple times)

N-grams are colored by the number of supporting references:
 red for fewest, green for most, mediate shades otherwise.\n";
  exit(1);
}

####################################################################################################################

my @TRUTHS = () x scalar(@truthfiles);
for(my $i = 0; $i < scalar(@truthfiles); $i++)
{
	open($TRUTHS[$i], "<$truthfiles[$i]") or die "couldn't open '$truthfiles[$i]' for read: $!\n";
	binmode($TRUTHS[$i], ":utf8");
}
my @SYSOUTS = () x scalar(@sysoutfiles);
for(my $i = 0; $i < scalar(@sysoutfiles); $i++)
{
	open($SYSOUTS[$i], "<$sysoutfiles[$i]") or die "couldn't open '$sysoutfiles[$i]' for read: $!\n";
	binmode($SYSOUTS[$i], ":utf8");
}
binmode(STDOUT, ":utf8");
if (defined $sourcefile)
{
	open(SOURCE, "<$sourcefile") or die "couldn't open '$sourcefile' for read: $!\n";
	binmode(SOURCE, ":utf8");
}
my @bleuScores;
for(my $i = 0; $i < scalar(@sysoutfiles); $i++) {push @bleuScores, [];}
my @htmlSentences;
my @javascripts;
my @htmlColors = ('#99ff99', '#aaaaff', '#ffff99', '#ff9933', '#ff9999'); #color sentences by rank (split in n tiers)
my $ngramSingleRefColor = '#aaffaa';
my @ngramMultirefColors = ('#ff9999', '#ff9933', '#ffff99', '#a0a0ff', '#99ff99'); #arbitrary-length list; first entry is used for worst n-grams
my $numSentences = 0;
my (@sLines, @eLines);
while(readLines(\@SYSOUTS, \@sLines) && readLines(\@TRUTHS, \@eLines))
{
	#create array of lines of HTML
	my @html = ("<div class=\"sentence_%%%%\" id=\"sentence$numSentences\">"); #%%%% is a flag to be replaced

	my (@sFactors, @eFactors, $sourceFactors);
	#process source
	if (defined $sourcefile)
	{
		my $sourceLine = <SOURCE>;
		escapeMetachars($sourceLine); #remove inconsistencies in encoding
		$sourceFactors = extractFactorArrays($sourceLine);
		push @html, "<tr><td class=\"sent_title\">Source</td><td class=\"source_sentence\" id=\"source$numSentences\">"
								. getFactoredSentenceHTML($sourceFactors) . "</td></tr>\n";
	}
	#process truth
	for(my $j = 0; $j < scalar(@truthfiles); $j++)
	{
		escapeMetachars($eLines[$j]); #remove inconsistencies in encoding
		push @eFactors, extractFactorArrays($eLines[$j]);
		push @html, "<tr><td class=\"sent_title\">Ref $j</td><td class=\"truth_sentence\" id=\"truth${numSentences}_$j\">"
								. getFactoredSentenceHTML($eFactors[$j]) . "</td></tr>\n";
	}
	#process sysouts
	my @bleuData;
	for(my $j = 0; $j < scalar(@sysoutfiles); $j++)
	{
		escapeMetachars($sLines[$j]); #remove inconsistencies in encoding
		push @sFactors, extractFactorArrays($sLines[$j]);
		push @bleuData, getBLEUSentenceDetails($sFactors[$j], \@eFactors, 0);
		push @{$bleuScores[$j]}, [$numSentences, $bleuData[$j]->[0], 0]; #the last number will be the rank
		my $pwerData = getPWERSentenceDetails($sFactors[$j], \@eFactors, 0);
		push @html, "<tr><td class=\"sent_title\">Output $j</td><td class=\"sysout_sentence\" id=\"sysout$numSentences\">"
								. getFactoredSentenceHTML($sFactors[$j], $pwerData) . "</td></tr>\n";
		push @html, "<tr><td class=\"sent_title\">N-grams</td><td class=\"sysout_ngrams\" id=\"ngrams$numSentences\">"
								. getAllNgramsHTML($sFactors[$j], $bleuData[$j]->[1], scalar(@truthfiles)) . "</td></tr>\n";
	}
	splice(@html, 1, 0, "<div class=\"bleu_report\"><b>Sentence $numSentences)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;BLEU:</b> "
							. join("; ", map {sprintf("%.4lg", $_->[0]->[0]) . " (" . join('/', map {sprintf("%.4lg", $_)} @{$_->[0]}[1 .. 4]) . ") "} @bleuData) . "</div><table>\n");
	push @html, "</table></div>\n";
	push @htmlSentences, join('', @html);
	$numSentences++;
	@sLines = (); @eLines = (); #clear writable arrays to be refilled
}
foreach my $sysoutfh (@SYSOUTS) {close($sysoutfh);}
foreach my $truthfh (@TRUTHS) {close($truthfh);}

my $stylesheet = "<style type=\"text/css\">
.legend {background: #fff; border: 1px solid #000; padding: 2px; margin-bottom: 10px; margin-right: 15px}
.legend_title {font-weight: bold; font-size: medium; text-decoration: underline}
div.bleu_report {margin-bottom: 5px}
td.sent_title {font-weight: bold; font-size: medium; margin-bottom: 12px}
.source_sentence {background: #ffcccc; border: 1px solid #bbb}
.truth_sentence {background: #ccffcc; border: 1px solid #bbb}
.sysout_sentence {background: #ccccff; border: 1px solid #bbb}
table.sentence_table {border: none}
.sysout_ngrams {background: #fff; border: 1px solid #bbb}
table.ngram_table {}
td.ngram_cell {padding: 1px}\n";
for(my $i = 0; $i < scalar(@htmlColors); $i++)
{
	$stylesheet .= ".sentence_tier$i {background: $htmlColors[$i]; border: 1px solid #000088; padding: 0px 8px 0px 8px} //entire composition for a given sentence\n";
	$stylesheet .= "div.sentence_tier$i td {margin: 8px 0px 8px 0px}\n";
}
$stylesheet .= "</style>\n";

print "<html><head><meta http-equiv=\"Content-type\" content=\"text/html; charset=UTF-8\">\n";
print "<title>[" . join(', ', @sysoutfiles) . "] vs. [" . join(', ', @truthfiles) . "]: Sentence-by-Sentence Comparison</title>$stylesheet</head><body>\n";

foreach my $systemScores (@bleuScores) {rankSentencesByBLEU($systemScores);}
#javascript to sort by BLEU for any system, by order in corpus ...
print "<script type=\"text/javascript\">
var selectedSysout = 0; //index of system currently being used to rank/sort

function selectSysout(index)
{
	//update the BLEU-range text shown in the legend
	var legend = document.getElementById('legendBLEU');
	var rows = legend.getElementsByTagName('tr');
	for(var i = 0; i < rows.length; i++)
	{
		var cell = rows[i].childNodes[1];
		var spans = cell.getElementsByTagName('span');
		cell.childNodes[0].nodeValue = spans[index].firstChild.nodeValue; //something like '0.1 - 0.3'
	}

	//update the background colors of the sentence divs
	var allSentences = document.getElementById('all_sentences');
	var sentences = allSentences.childNodes;
	for(var i = 0; i < sentences.length; i++)
	{
		if(typeof sentences[i].tagName != 'undefined' && sentences[i].tagName.toLowerCase() == 'div') //text nodes have undefined tagName
		{
			var tierSpans = sentences[i].firstChild.childNodes;
			sentences[i].childNodes[2].className = tierSpans[index].firstChild.nodeValue; //something like 'tier3'
		}
	}
	selectedSysout = index; //selectedSysout is a flag to the sort functions
}

function sortByBLEU()
{
	var body = document.getElementById('all_sentences'); var row;
	switch(selectedSysout)
	{\n";
for(my $i = 0; $i < scalar(@sysoutfiles); $i++)
{
	print "case $i:
	{";
	my %rank2index = map {$bleuScores[$i]->[$_]->[2] => $_} (0 .. scalar(@htmlSentences) - 1);
	foreach my $rank (sort {$a <=> $b} keys %rank2index)
	{
		print "\trow = document.getElementById('everything" . $rank2index{$rank} . "');\n";
		print "\tbody.removeChild(row); body.appendChild(row);\n";
	}
	print "break;}\n";
}
print "}
}
function sortByCorpusOrder()
{
	var body = document.getElementById('all_sentences'); var row;\n";
for(my $j = 0; $j < scalar(@htmlSentences); $j++)
{
	print "\trow = document.getElementById('everything$j');\n";
	print "\tbody.removeChild(row); body.appendChild(row);\n";
}
print "}
</script>\n";

#legends for background colors of sentences and n-grams
my (@minBLEU, @maxBLEU);
my @bleuTiers = () x scalar(@htmlSentences); #for each sentence, arrayref of tier indices for each system
for(my $i = 0; $i < scalar(@sysoutfiles); $i++)
{
	my @a = (1e9) x scalar(@htmlColors);
	my @b = (-1e9) x scalar(@htmlColors);
	for(my $k = 0; $k < scalar(@htmlSentences); $k++)
	{
		my $tier = int($bleuScores[$i]->[$k]->[2] / (scalar(@htmlSentences) / scalar(@htmlColors)));
		push @{$bleuTiers[$k]}, $tier;
		if($bleuScores[$i]->[$k]->[1]->[0] < $a[$tier]) {$a[$tier] = $bleuScores[$i]->[$k]->[1]->[0];}
		if($bleuScores[$i]->[$k]->[1]->[0] > $b[$tier]) {$b[$tier] = $bleuScores[$i]->[$k]->[1]->[0];}
	}
	push @minBLEU, \@a;
	push @maxBLEU, \@b;
}
print "<table border=0><tr><td><div id=\"legendBLEU\" class=\"legend\"><span class=\"legend_title\">Sentence Background Colors => BLEU Ranges</span><table border=0>";
for(my $k = 0; $k < scalar(@htmlColors); $k++)
{
	print "<tr><td style=\"width: 15px; height: 15px; background: " . $htmlColors[$k] . "\"></td><td align=left style=\"padding-left: 12px\">"
							. sprintf("%.4lg", $minBLEU[0]->[$k]) . " - " . sprintf("%.4lg", $maxBLEU[0]->[$k]);
	for(my $j = 0; $j < scalar(@sysoutfiles); $j++)
	{
		print "<span style=\"display: none\">" . sprintf("%.4lg", $minBLEU[$j]->[$k]) . " - " . sprintf("%.4lg", $maxBLEU[$j]->[$k]) . "</span>";
	}
	print "</td></tr>";
}
print "</table></div></td>\n";
print "<td><div class=\"legend\"><span class=\"legend_title\">N-gram Colors => Number of Matching Reference Translations</span><table border=0>";
for(my $k = 1; $k <= scalar(@truthfiles); $k++)
{
	print "<tr><td style=\"width: 15px; height: 15px; background: " . getNgramColorHTML($k, scalar(@truthfiles)) . "\"></td><td align=left style=\"padding-left: 12px\">$k</td></tr>";
}
print "</table></div></td></tr></table><div style=\"font-weight: bold; margin-bottom: 15px\">
PWER errors are marked in red on output sentence displays.</div>
<div style=\"margin-bottom: 8px\">Color by system # "
						. join(' | ', map {"<a href=\"javascript:selectSysout($_);\">$_</a>" . (($_ == '0') ? " (default)" : "")} (0 .. scalar(@sysoutfiles) - 1)) . "</div>
<div style=\"margin-bottom: 8px\">Sort by <a href=\"javascript:sortByBLEU();\">BLEU score</a> | <a href=\"javascript:sortByCorpusOrder();\">corpus order</a> (default)</div>\n";

#sentence boxes
print "<div id=\"all_sentences\">";
for(my $j = 0; $j < scalar(@htmlSentences); $j++)
{
	print "<div id=\"everything$j\" style=\"margin: 0px; padding: 0px\">";
	print "<div class=\"ranks_container\" style=\"display: none\">" . join('', map {"<span>sentence_tier$_</span>"} @{$bleuTiers[$j]}) . "</div>";
	print "<hr width=98%>";
#	my $bgcolor = getSentenceBGColorHTML($bleuScores[0]->[$j], $i); #i is now # of sentences
	my $tierNum = $bleuTiers[$j]->[0];
	$htmlSentences[$j] =~ s/%%%%/tier$tierNum/;
	print "$htmlSentences[$j]</div>\n";
}
print "</div></body></html>";

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
#arguments: a list of elements
#return undef for an empty list, the max element otherwise
sub maxN
{
	if(scalar @_ == 0) {return undef;}
	my $val = shift @_;
	foreach my $e (@_) {if($e > $val) {$val = $e;}}
	return $val;
}
#arguments: x
sub my_log
{
  return -9999999999 unless $_[0];
  return log($_[0]);
}
#arguments: x
sub round
{
	my $x = shift;
	return ($x - int($x) < .5) ? int($x) : int($x) + 1;
}

#escape HTML metacharacters for display purposes and to allow for consistent string comparison
#arguments: string to be formatted in place
#return: none
sub escapeMetachars
{
	my $str = shift;
	$str =~ s/&\s+/&amp; /;
	$str =~ s/<\s+/&lt; /;
	$str =~ s/>\s+/&gt; /;
}

###############################################################################################################################################################

#read one line from each of any number of filehandles
#arguments: arrayref of filehandles, (empty) arrayref to be filled with read lines
#return: 1 on success, 0 on failure (on failure the lines arrayref's value isn't defined)
sub readLines
{
	my ($refFilehandles, $refLines) = @_;
	foreach my $fh (@$refFilehandles)
	{
		my $line = <$fh>;
		return 0 unless defined($line);
		push @$refLines, $line;
	}
	return 1;
}

#arguments: line read from corpus file
#return: sentence (arrayref of arrayrefs of factor strings) taken from line
sub extractFactorArrays
{
	my $line = shift;
	die "" if !defined $line;
	chomp $line;
	$line =~ s/^\s*|\s*$//g; #added by Ondrej to handle moses-mert-parallel output
	my @words = split(/\s+/, $line);
	my @factors = map {my @f = split(/\|/, $_); \@f;} @words;
	return \@factors;
}

#can handle multiple reference translations; assume at least one
#arguments: sysout sentence (arrayref of arrayrefs of factor strings), truth sentences (arrayref of same), factor index to use
#return: arrayref of [arrayref of [overall BLEU score, n-gram precisions], arrayref of matching n-gram [start index, length, arrayref of indices of matching references]]
sub getBLEUSentenceDetails
{
	my $maxNgramOrder = 4;
	my ($refSysOutput, $refTruths, $factorIndex) = @_;
	my $length_translation = scalar(@$refSysOutput); #length of sysout sentence
	my @length_references = map {scalar(@$_)} @$refTruths;
	my $closestTruthLength = (sort(map {abs($_ - $length_translation)} @length_references))[0];
	my @correct = (0) x $maxNgramOrder; #n-gram counts
	my @total = (0) x $maxNgramOrder; #n-gram counts
	my $returnData = [[], []];
	my %REF_GRAM; #hash from n-gram to arrayref with # of times found in each reference
	my $ngramMatches = []; #arrayref of n-gram [start index, length]
	for(my $j = 0; $j < scalar(@$refTruths); $j++)
	{
		for(my $i = 0; $i < $length_references[$j]; $i++)
		{
			my $gram = '';
			for(my $k = 0; $k < min($i + 1, $maxNgramOrder); $k++) #run over n-gram orders
			{
				$gram = $refTruths->[$j]->[$i - $k]->[$factorIndex] . " " . $gram;
				#increment the count for the given n-gram and given reference number
				if(!exists $REF_GRAM{$gram})
				{
					my @tmp = (0) x scalar @$refTruths;
					$tmp[$j] = 1;
					$REF_GRAM{$gram} = \@tmp;
				}
				else
				{
					$REF_GRAM{$gram}->[$j]++;
				}
			}
		}
	}
	for(my $i = 0; $i < $length_translation; $i++)
	{
		my $gram = '';
		for(my $k = 0; $k < min($i + 1, $maxNgramOrder); $k++) #run over n-gram orders
		{
			$gram = $refSysOutput->[$i - $k]->[$factorIndex] . " " . $gram;
			if(exists $REF_GRAM{$gram}) #this n-gram was found in at least one reference
			{
				$correct[$k]++;
				my @indices = ();
				my $notOvercounting = 0; #make sure we don't 'match' against truth n-grams whose instances have all been used already
				for(my $m = 0; $m < scalar(@{$REF_GRAM{$gram}}); $m++)
				{
					if($REF_GRAM{$gram}->[$m] > 0)
					{
						push @indices, $m;
						$REF_GRAM{$gram}->[$m]--;
						$notOvercounting = 1;
					}
				}
				if($notOvercounting == 1) {push @$ngramMatches, [$i - $k, $k + 1, \@indices];}
			}
		}
	}
	my $brevity = ($length_translation > $closestTruthLength || $length_translation == 0) ? 1 : exp(1 - $closestTruthLength / $length_translation);
	my @pct;
	my ($logsum, $logcount) = (0, 0);
	for(my $i = 0; $i < $maxNgramOrder; $i++)
	{
		$total[$i] = max(1, $length_translation - $i);
		push @pct, ($total[$i] == 0) ? -1 : $correct[$i] / $total[$i];
		if($total[$i] > 0)
		{
			$logsum += my_log($pct[$i]);
			$logcount++;
		}
	}
	my $bleu = $brevity * exp($logsum / $logcount);
	$returnData->[0] = [$bleu, @pct];
	$returnData->[1] = $ngramMatches;
	return $returnData;
}

#can handle multiple sentence translations; assume at least one
#arguments: sysout sentence (arrayref of arrayrefs of factor strings), truth sentences (arrayref of same), factor index to use
#return: hashref of sysout word index => whether word matches
sub getPWERSentenceDetails
{
	my ($refSysOutput, $refTruths, $factorIndex) = @_;
	my $matches = {};
	my %truthWords; #hash from word to arrayref with number of times seen in each reference (but later holds only the max)
	for(my $i = 0; $i < scalar(@$refTruths); $i++)
	{
		foreach my $eWord (@{$refTruths->[$i]})
		{
			my $factor = $eWord->[$factorIndex];
			if(exists $truthWords{$factor}) {$truthWords{$factor}->[$i]++;}
			else {my @tmp = (0) x scalar(@$refTruths); $tmp[$i] = 1; $truthWords{$factor} = \@tmp;}
		}
	}
	%truthWords = map {$_ => maxN(@{$truthWords{$_}})} (keys %truthWords); #save only the max times each word is seen in a reference
	for(my $j = 0; $j < scalar(@$refSysOutput); $j++)
	{
		if(exists $truthWords{$refSysOutput->[$j]->[$factorIndex]} && $truthWords{$refSysOutput->[$j]->[$factorIndex]} > 0)
		{
			$truthWords{$refSysOutput->[$j]->[$factorIndex]}--;
			$matches->{$j} = 1;
		}
		else
		{
			$matches->{$j} = 0;
		}
	}
	return $matches;
}

#assign ranks to sentences by BLEU score
#arguments: arrayref of arrayrefs of [sentence index, arrayref of [bleu score, n-gram precisions], rank to be assigned]
#return: none
sub rankSentencesByBLEU
{
	my $bleuData = shift;
	my $i = 0;
	#sort first on score, then on 1-gram accuracy, then on sentence index
	foreach my $sentenceData (reverse sort {my $c = $a->[1]->[0] <=> $b->[1]->[0]; if($c == 0) {my $d = $a->[1]->[1] <=> $b->[1]->[1]; if($d == 0) {$a->[0] cmp $b->[0];} else {$d;}} else {$c;}} @$bleuData) {$sentenceData->[2] = $i++;}
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

#arguments: arrayref of [sentence index, arrayref of [bleu score, n-gram precisions], rank], number of sentences
#return: HTML color string
sub getSentenceBGColorHTML
{
	my ($scoreData, $numSentences) = @_;
	my $tier = int($scoreData->[2] / ($numSentences / scalar(@htmlColors))); #0..n-1
	return $htmlColors[$tier];
}

#display all matching n-grams in the given sentence, with all 1-grams on one line, then arranged by picking, for each, the first line on which it fits,
# where a given word position can only be filled by one n-gram per line, so that all n-grams can be shown
#arguments: sentence (arrayref of arrayrefs of factor strings), arrayref of arrayrefs of matching n-gram [start, length, arrayref of matching reference indices],
# number of reference translations
#return: HTML string
sub getAllNgramsHTML
{
	my ($sentence, $ngrams, $numTruths) = @_;
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
	my ($curRow, $curCol) = (0, 0); #address in table
	$html .= "<tr>";
	foreach my $ngram (sort {my $c = $a->[3] <=> $b->[3]; if($c == 0) {$a->[0] <=> $b->[0]} else {$c}} @$ngrams) #sort by row, then word num
	{
		while($ngram->[0] > $curCol || $ngram->[3] > $curRow) {$html .= "<td></td>"; $curCol = ($curCol + 1) % $numWords; if($curCol == 0) {$html .= "</tr><tr>"; $curRow++;}}
		$html .= "<td colspan=" . $ngram->[1] . " align=center class=\"ngram_cell\" style=\"background: " . getNgramColorHTML(scalar(@{$ngram->[2]}), $numTruths) . "\">" . join(' ', map {$_->[$factorIndex]} @{$sentence}[$ngram->[0] .. $ngram->[0] + $ngram->[1] - 1]) . "</td>";
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

#auxiliary to getAllNgramsHTML()
#arguments: number of reference translations matching the n-gram, total number of references
#return: HTML color string
sub getNgramColorHTML
{
	my ($matches, $total) = @_;
	if($total == 1) {return $ngramSingleRefColor;}
	return $ngramMultirefColors[round($matches / $total * (scalar(@ngramMultirefColors) - 1))];
}
