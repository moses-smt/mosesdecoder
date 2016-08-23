#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.
use utf8;

###############################################
# An implementation of paired bootstrap resampling for testing the statistical
# significance of the difference between two systems from (Koehn 2004 @ EMNLP)
#
# Usage: ./compare-hypotheses-with-significance.pl hypothesis_1 hypothesis_2 reference_1 [ reference_2 ... ]
#
# Author: Mark Fishel, fishel@ut.ee
#
# 22.10.2008: altered algorithm according to (Riezler and Maxwell 2005 @ MTSE'05), now computes p-value
#
# 23.01.2010: added NIST p-value and interval computation
###############################################

use warnings;
use strict;

#constants
my $TIMES_TO_REPEAT_SUBSAMPLING = 1000;
my $SUBSAMPLE_SIZE = 0; # if 0 then subsample size is equal to the whole set
my $MAX_NGRAMS = 4;
my $IO_ENCODING = "utf8"; # can be replaced with e.g. "encoding(iso-8859-13)" or alike

#checking cmdline argument consistency
if (@ARGV < 3) {
	print STDERR "Usage: ./bootstrap-hypothesis-difference-significance.pl hypothesis_1 hypothesis_2 reference_1 [ reference_2 ... ]\n";

	unless ($ARGV[0] =~ /^(--help|-help|-h|-\?|\/\?|--usage|-usage)$/) {
		die("\nERROR: not enough arguments");
	}

	exit 1;
}

print STDERR "reading data; " . `date`;

#read all data
my $data = readAllData(@ARGV);

my $verbose = $ARGV[3];

#calculate each sentence's contribution to BP and ngram precision
print STDERR "performing preliminary calculations (hypothesis 1); " . `date`;
preEvalHypo($data, "hyp1");

print STDERR "performing preliminary calculations (hypothesis 2); " . `date`;
preEvalHypo($data, "hyp2");

#start comparing
print STDERR "comparing hypotheses -- this may take some time; " . `date`;

bootstrap_report("BLEU", \&getBleu);
bootstrap_report("NIST", \&getNist);

#####
#
#####
sub bootstrap_report {
	my $title = shift;
	my $proc = shift;

	my ($subSampleScoreDiffArr, $subSampleScore1Arr, $subSampleScore2Arr) = bootstrap_pass($proc);

	my $realScore1 = &$proc($data->{refs}, $data->{hyp1});
	my $realScore2 = &$proc($data->{refs}, $data->{hyp2});

	my $scorePValue = bootstrap_pvalue($subSampleScoreDiffArr, $realScore1, $realScore2);

	my ($scoreAvg1, $scoreVar1) = bootstrap_interval($subSampleScore1Arr);
	my ($scoreAvg2, $scoreVar2) = bootstrap_interval($subSampleScore2Arr);

	print "\n---=== $title score ===---\n";

	print "actual score of hypothesis 1: $realScore1\n";
	print "95% confidence interval for hypothesis 1 score: $scoreAvg1 +- $scoreVar1\n-----\n";
	print "actual score of hypothesis 1: $realScore2\n";
	print "95% confidence interval for hypothesis 2 score: $scoreAvg2 +- $scoreVar2\n-----\n";
	print "Assuming that essentially the same system generated the two hypothesis translations (null-hypothesis),\n";
	print "the probability of actually getting them (p-value) is: $scorePValue.\n";
}

#####
#
#####
sub bootstrap_pass {
	my $scoreFunc = shift;

	my @subSampleDiffArr;
	my @subSample1Arr;
	my @subSample2Arr;

	#applying sampling
	for my $idx (1..$TIMES_TO_REPEAT_SUBSAMPLING) {
		my $subSampleIndices = drawWithReplacement($data->{size}, ($SUBSAMPLE_SIZE? $SUBSAMPLE_SIZE: $data->{size}));

		my $score1 = &$scoreFunc($data->{refs}, $data->{hyp1}, $subSampleIndices);
		my $score2 = &$scoreFunc($data->{refs}, $data->{hyp2}, $subSampleIndices);

		push @subSampleDiffArr, abs($score2 - $score1);
		push @subSample1Arr, $score1;
		push @subSample2Arr, $score2;

		if ($idx % 10 == 0) {
			print STDERR ".";
		}
		if ($idx % 100 == 0) {
			print STDERR "$idx\n";
		}
	}

	if ($TIMES_TO_REPEAT_SUBSAMPLING % 100 != 0) {
		print STDERR ".$TIMES_TO_REPEAT_SUBSAMPLING\n";
	}

	return (\@subSampleDiffArr, \@subSample1Arr, \@subSample2Arr);
}

#####
#
#####
sub bootstrap_pvalue {
	my $subSampleDiffArr = shift;
	my $realScore1 = shift;
	my $realScore2 = shift;

	my $realDiff = abs($realScore2 - $realScore1);

	#get subsample difference mean
	my $averageSubSampleDiff = 0;

	for my $subSampleDiff (@$subSampleDiffArr) {
		$averageSubSampleDiff += $subSampleDiff;
	}

	$averageSubSampleDiff /= $TIMES_TO_REPEAT_SUBSAMPLING;

	#calculating p-value
	my $count = 0;

	my $realScoreDiff = abs($realScore2 - $realScore1);

	for my $subSampleDiff (@$subSampleDiffArr) {
		if ($subSampleDiff - $averageSubSampleDiff >= $realDiff) {
			$count++;
		}
	}

	return $count / $TIMES_TO_REPEAT_SUBSAMPLING;
}

#####
#
#####
sub bootstrap_interval {
	my $subSampleArr = shift;

	my @sorted = sort @$subSampleArr;

	my $lowerIdx = int($TIMES_TO_REPEAT_SUBSAMPLING / 40);
	my $higherIdx = $TIMES_TO_REPEAT_SUBSAMPLING - $lowerIdx - 1;

	my $lower = $sorted[$lowerIdx];
	my $higher = $sorted[$higherIdx];
	my $diff = $higher - $lower;

	return ($lower + 0.5 * $diff, 0.5 * $diff);
}

#####
# read 2 hyp and 1 to \infty ref data files
#####
sub readAllData {
	my ($hypFile1, $hypFile2, @refFiles) = @_;

	my %result;

	#reading hypotheses and checking for matching sizes
	$result{hyp1} = readData($hypFile1);
	$result{size} = scalar @{$result{hyp1}};

	$result{hyp2} = readData($hypFile2);
	unless (scalar @{$result{hyp2}} == $result{size}) {
		die ("ERROR: sizes of hypothesis sets 1 and 2 don't match");
	}

	#reading reference(s) and checking for matching sizes
	$result{refs} = [];
	$result{ngramCounts} = { };
	my $i = 0;

	for my $refFile (@refFiles) {
		$i++;
		my $refDataX = readData($refFile);

		unless (scalar @$refDataX == $result{size}) {
			die ("ERROR: ref set $i size doesn't match the size of hyp sets");
		}

		updateCounts($result{ngramCounts}, $refDataX);

		push @{$result{refs}}, $refDataX;
	}

	return \%result;
}

#####
#
#####
sub updateCounts {
	my ($countHash, $refData) = @_;

	for my $snt(@$refData) {
		my $size = scalar @{$snt->{words}};
		$countHash->{""} += $size;

		for my $order(1..$MAX_NGRAMS) {
			my $ngram;

			for my $i (0..($size-$order)) {
				$ngram = join(" ", @{$snt->{words}}[$i..($i + $order - 1)]);

				$countHash->{$ngram}++;
			}
		}
	}
}

#####
#
#####
sub ngramInfo {
	my ($data, $ngram) = @_;

	my @nwords = split(/ /, $ngram);
	pop @nwords;
	my $smallGram = join(" ", @nwords);

	return log($data->{ngramCounts}->{$smallGram} / $data->{ngramCounts}->{$ngram}) / log(2.0);
}

#####
# read sentences from file
#####
sub readData {
	my $file = shift;
	my @result;

	open (FILE, $file) or die ("Failed to open `$file' for reading");
	binmode (FILE, ":$IO_ENCODING");

	while (<FILE>) {
		push @result, { words => [split(/\s+/, $_)] };
	}

	close (FILE);

	return \@result;
}

#####
# calculate each sentence's contribution to the ngram precision and brevity penalty
#####
sub preEvalHypo {
	my $data = shift;
	my $hypId = shift;

	for my $lineIdx (0..($data->{size} - 1)) {
		preEvalHypoSnt($data, $hypId, $lineIdx);
	}
}

#####
#
#####
sub preEvalHypoSnt {
	my ($data, $hypId, $lineIdx) = @_;

	my ($correctNgramCounts, $totalNgramCounts);
	my ($refNgramCounts, $hypNgramCounts);
	my ($coocNgramInfoSum, $totalNgramAmt);

	my $hypSnt = $data->{$hypId}->[$lineIdx];

	#update total hyp len
	$hypSnt->{hyplen} = scalar @{$hypSnt->{words}};

	#update total ref len with closest current ref len
	$hypSnt->{reflen} = getClosestLength($data->{refs}, $lineIdx, $hypSnt->{hyplen});
	$hypSnt->{avgreflen} = getAvgLength($data->{refs}, $lineIdx);

	$hypSnt->{correctNgrams} = [];
	$hypSnt->{totalNgrams} = [];

	#update ngram precision for each n-gram order
	for my $order (1..$MAX_NGRAMS) {
		#hyp ngrams
		$hypNgramCounts = groupNgrams($hypSnt, $order);

		#ref ngrams
		$refNgramCounts = groupNgramsMultiSrc($data->{refs}, $lineIdx, $order);

		$correctNgramCounts = 0;
		$totalNgramCounts = 0;
		$coocNgramInfoSum = 0;
		$totalNgramAmt = 0;
		my $coocUpd;

		#correct, total
		for my $ngram (keys %$hypNgramCounts) {
			if (!exists $refNgramCounts->{$ngram}) {
				$refNgramCounts->{$ngram} = 0;
			}
			$coocUpd = min($hypNgramCounts->{$ngram}, $refNgramCounts->{$ngram});
			$correctNgramCounts += $coocUpd;
			$totalNgramCounts += $hypNgramCounts->{$ngram};

			if ($coocUpd > 0) {
				$coocNgramInfoSum += ngramInfo($data, $ngram);
			}

			$totalNgramAmt++;
		}

		$hypSnt->{correctNgrams}->[$order] = $correctNgramCounts;
		$hypSnt->{totalNgrams}->[$order] = $totalNgramCounts;
		$hypSnt->{ngramNistInfoSum}->[$order] = $coocNgramInfoSum;
		$hypSnt->{ngramNistCount}->[$order] = $totalNgramAmt;
	}
}

#####
# draw a subsample of size $subSize from set (0..$setSize) with replacement
#####
sub drawWithReplacement {
	my ($setSize, $subSize) = @_;

	my @result;

	for (1..$subSize) {
		push @result, int(rand($setSize));
	}

	return \@result;
}

#####
#
#####
sub getNist {
	my ($refs, $hyp, $idxs) = @_;

	#default value for $idxs
	unless (defined($idxs)) {
		$idxs = [0..((scalar @$hyp) - 1)];
	}

	#vars
	my ($hypothesisLength, $referenceLength) = (0, 0);
	my (@infosum, @totalamt);

	#gather info from each line
	for my $lineIdx (@$idxs) {

		my $hypSnt = $hyp->[$lineIdx];

		#update total hyp len
		$hypothesisLength += $hypSnt->{hyplen};

		#update total ref len with closest current ref len
		$referenceLength += $hypSnt->{avgreflen};

		#update ngram precision for each n-gram order
		for my $order (1..$MAX_NGRAMS) {
			$infosum[$order] += $hypSnt->{ngramNistInfoSum}->[$order];
			$totalamt[$order] += $hypSnt->{ngramNistCount}->[$order];
		}
	}

	my $toplog = log($hypothesisLength / $referenceLength);
	my $btmlog = log(2.0/3.0);

	#compose nist score
	my $brevityPenalty = ($hypothesisLength > $referenceLength)? 1.0: exp(log(0.5) * $toplog * $toplog / ($btmlog * $btmlog));

	my $sum = 0;

	for my $order (1..$MAX_NGRAMS) {
		$sum += $infosum[$order]/$totalamt[$order];
	}

	my $result = $sum * $brevityPenalty;

	return $result;
}

#####
# refs: arrayref of different references, reference = array of lines, line = array of words, word = string
# hyp: arrayref of lines of hypothesis translation, line = array of words, word = string
# idxs: indices of lines to include; default value - full set (0..size_of_hyp-1)
#####
sub getBleu {
	my ($refs, $hyp, $idxs) = @_;

	#default value for $idxs
	unless (defined($idxs)) {
		$idxs = [0..((scalar @$hyp) - 1)];
	}

	#vars
	my ($hypothesisLength, $referenceLength) = (0, 0);
	my (@correctNgramCounts, @totalNgramCounts);
	my ($refNgramCounts, $hypNgramCounts);

	#gather info from each line
	for my $lineIdx (@$idxs) {
		my $hypSnt = $hyp->[$lineIdx];

		#update total hyp len
		$hypothesisLength += $hypSnt->{hyplen};

		#update total ref len with closest current ref len
		$referenceLength += $hypSnt->{reflen};

		#update ngram precision for each n-gram order
		for my $order (1..$MAX_NGRAMS) {
			$correctNgramCounts[$order] += $hypSnt->{correctNgrams}->[$order];
			$totalNgramCounts[$order] += $hypSnt->{totalNgrams}->[$order];
		}
	}

	#compose bleu score
	my $brevityPenalty = ($hypothesisLength < $referenceLength)? exp(1 - $referenceLength/$hypothesisLength): 1;

	my $logsum = 0;

	for my $order (1..$MAX_NGRAMS) {
		$logsum += safeLog($correctNgramCounts[$order] / $totalNgramCounts[$order]);
	}

	return $brevityPenalty * exp($logsum / $MAX_NGRAMS);
}

#####
#
#####
sub getAvgLength {
	my ($refs, $lineIdx) = @_;

	my $result = 0;
	my $count = 0;

	for my $ref (@$refs) {
		$result += scalar @{$ref->[$lineIdx]->{words}};
		$count++;
	}

	return $result / $count;
}

#####
#
#####
sub getClosestLength {
	my ($refs, $lineIdx, $hypothesisLength) = @_;

	my $bestDiff = infty();
	my $bestLen = infty();

	my ($currLen, $currDiff);

	for my $ref (@$refs) {
		$currLen = scalar @{$ref->[$lineIdx]->{words}};
		$currDiff = abs($currLen - $hypothesisLength);

		if ($currDiff < $bestDiff or ($currDiff == $bestDiff and $currLen < $bestLen)) {
			$bestDiff = $currDiff;
			$bestLen = $currLen;
		}
	}

	return $bestLen;
}

#####
#
#####
sub groupNgrams {
	my ($snt, $order) = @_;
	my %result;

	my $size = scalar @{$snt->{words}};
	my $ngram;

	for my $i (0..($size-$order)) {
		$ngram = join(" ", @{$snt->{words}}[$i..($i + $order - 1)]);

		$result{$ngram}++;
	}

	return \%result;
}

#####
#
#####
sub groupNgramsMultiSrc {
	my ($refs, $lineIdx, $order) = @_;
	my %result;

	for my $ref (@$refs) {
		my $currNgramCounts = groupNgrams($ref->[$lineIdx], $order);

		for my $currNgram (keys %$currNgramCounts) {
			if (!exists $result{$currNgram}) {
				$result{$currNgram} = 0;
			}
			$result{$currNgram} = max($result{$currNgram}, $currNgramCounts->{$currNgram});
		}
	}

	return \%result;
}

#####
#
#####
sub safeLog {
	my $x = shift;

	return ($x > 0)? log($x): -infty();
}

#####
#
#####
sub infty {
	return 1e6000;
}

#####
#
#####
sub min {
	my ($a, $b) = @_;

	return ($a < $b)? $a: $b;
}

#####
#
#####
sub max {
	my ($a, $b) = @_;

	return ($a > $b)? $a: $b;
}

sub poww {
	my ($a, $b) = @_;

	return exp($b * log($a));
}
