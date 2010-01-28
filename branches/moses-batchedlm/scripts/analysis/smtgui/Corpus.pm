#package Corpus: hold a bunch of sentences in any language, with translation factors and stats about individual sentences and the corpus as a whole
#Evan Herbst, 7 / 25 / 06

package Corpus;
BEGIN
{
	push @INC, "../perllib"; #for Error.pm
}
use Error;

return 1;

###########################################################################################################################

##### 'our' variables are available outside the package #####
#all factor names used should be in this list, just in case
our @FACTORNAMES = ('surf', 'pos', 'lemma', 'stem', 'morph');

#constructor
#arguments: short corpus name (-name), hashref of filenames to descriptions (-descriptions), formatted string with various config info (-info_line)
sub new
{
	my $class = shift;
	my %args = @_; #turn the remainder of @_ into a hash
	my ($corpusName, $refFileDescs, $infoLine) = ($args{'-name'}, $args{'-descriptions'}, $args{'-info_line'});
	my ($factorList, $inputLingmodels, $outputLingmodels) = split(/\s*:\s*/, $infoLine);
	my $self = {};
	$self->{'corpusName'} = $corpusName;
	$self->{'truth'} = []; #arrayref of arrayrefs of factors
	$self->{'input'} = []; #same; also same for any system outputs that get loaded
	$self->{'tokenCount'} = {}; #sysname => number of tokens in file
	$self->{'truthFilename'} = "";
	$self->{'inputFilename'} = "";
	$self->{'sysoutFilenames'} = {}; #hashref of (string => string) for (system name, filename)
	$self->{'phraseTableFilenames'} = {}; #factor name => filename
	$self->{'fileCtimes'} = {}; #file ID of some kind => changetime in seconds
	$self->{'factorIndices'} = {}; #factor name => index
	my @factors = split(/\s+/, $factorList);
	for(my $i = 0; $i < scalar(@factors); $i++)
	{
		$self->{'factorIndices'}->{$factors[$i]} = $i;
	}
	$self->{'inputLMs'} = {}; #factor name => lingmodel filename
	$self->{'outputLMs'} = {};
	foreach my $lmInfo (split(/\s*,\s*/, $inputLingmodels))
	{
		my @tokens = split(/\s+/, $lmInfo);
		$self->{'inputLMs'}->{$tokens[0]} = $tokens[1];
	}
	foreach my $lmInfo (split(/\s*,\s*/, $outputLingmodels))
	{
		my @tokens = split(/\s+/, $lmInfo);
		$self->{'outputLMs'}->{$tokens[0]} = $tokens[1];
	}
	$self->{'phraseTables'} = {}; #factor name (from @FACTORNAMES) => hashref of source phrases to anything; used for unknown-word counting
	$self->{'unknownCount'} = {}; #factor name => count of unknown tokens in input
	$self->{'sysoutWER'} = {}; #system name => (factor name => arrayref with system output total WER and arrayref of WER scores for individual sysout sentences wrt truth)
	$self->{'sysoutPWER'} = {}; #similarly
	$self->{'nnAdjWERPWER'} = {}; #system name => arrayref of [normalized WER, normalized PWER]
	$self->{'perplexity'} = {}; #system name => (factor name => perplexity raw score)
	$self->{'fileDescriptions'} = {}; #filename associated with us => string description of file
	$self->{'bleuScores'} = {}; #system name => (factor name => arrayref of (overall score, arrayref of per-sentence scores) )
	$self->{'bleuConfidence'} = {}; #system name => (factor name => arrayrefs holding statistical test data on BLEU scores)
	$self->{'subsetBLEUstats'} = {}; #system name => (factor name => n-gram precisions and lengths for independent corpus subsets)
	$self->{'comparisonStats'} = {}; #system name 1 => (system name 2 => (factor name => p-values, and indices of better system, for all tests used))
	$self->{'cacheFilename'} = "cache/$corpusName.cache"; #all memory of various scores is stored here
	bless $self, $class;
	$self->locateFiles($refFileDescs); #find all relevant files in the current directory; set filenames and descriptions
	$self->loadCacheFile();
	print STDERR "on load:\n";
	$self->printDetails();
	return $self;
}

#arguments: filename
#return: description string
#throw if filename doesn't belong to this corpus
sub getFileDescription
{
	my ($self, $filename) = @_;
	if(!defined($self->{'fileDescriptions'}->{$filename}))
	{
		throw Error::Simple(-text => "Corpus::getFileDescription(): invalid filename '$filename'\n");
	}
	return $self->{'fileDescriptions'}->{$filename};
}

#arguments: none
#return: list of system names (NOT including 'input', 'truth' and other special cases)
sub getSystemNames
{
	my $self = shift;
	return keys %{$self->{'sysoutFilenames'}};
}

#calculate the number of unknown factor values for the given factor in the input file
#arguments: factor name
#return: unknown factor count, total factor count (note the total doesn't depend on the factor)
#throw if we don't have an input file or a phrase table for the given factor defined or if there's no index known for the given factor
sub calcUnknownTokens
{
	my ($self, $factorName) = @_;
	#check in-memory cache first
	if(exists $self->{'unknownCount'}->{$factorName} && exists $self->{'tokenCount'}->{'input'})
	{
		return ($self->{'unknownCount'}->{$factorName}, $self->{'tokenCount'}->{'input'});
	}
	warn "calcing unknown tokens\n";
	
	$self->ensureFilenameDefined('input');
	$self->ensurePhraseTableDefined($factorName);
	$self->ensureFactorPosDefined($factorName);
	$self->loadSentences('input', $self->{'inputFilename'});
	$self->loadPhraseTable($factorName);

	#count unknown and total words
	my ($unknownTokens, $totalTokens) = (0, 0);
	my $factorIndex = $self->{'factorIndices'}->{$factorName};
	foreach my $sentence (@{$self->{'input'}})
	{
		$totalTokens += scalar(@$sentence);
		foreach my $word (@$sentence)
		{
			if(!defined($self->{'phraseTables'}->{$factorName}->{$word->[$factorIndex]}))
			{
				$unknownTokens++;
			}
		}
	}
	$self->{'unknownCount'}->{$factorName} = $unknownTokens;
	$self->{'tokenCount'}->{'input'} = $totalTokens;
	
	return ($unknownTokens, $totalTokens);
}

#arguments: system name
#return: (WER, PWER) for nouns and adjectives in given system wrt truth
#throw if given system or truth is not set or if index of 'surf' or 'pos' hasn't been specified
sub calcNounAdjWER_PWERDiff
{
	my ($self, $sysname) = @_;
	#check in-memory cache first
	if(exists $self->{'nnAdjWERPWER'}->{$sysname})
	{
		return @{$self->{'nnAdjWERPWER'}->{$sysname}};
	}
	warn "calcing NN/JJ PWER/WER\n";
	
	$self->ensureFilenameDefined('truth');
	$self->ensureFilenameDefined($sysname);
	$self->ensureFactorPosDefined('surf');
	$self->ensureFactorPosDefined('pos');
	$self->loadSentences('truth', $self->{'truthFilename'});
	$self->loadSentences($sysname, $self->{'sysoutFilenames'}->{$sysname});
	#find nouns and adjectives and score them
	my ($werScore, $pwerScore) = (0, 0);
	my $nnNadjTags = $self->getPOSTagList('nounAndAdj');
	for(my $i = 0; $i < scalar(@{$self->{'truth'}}); $i++)
	{
		my @nnAdjEWords = $self->filterFactors($self->{'truth'}->[$i], $self->{'factorIndices'}->{'pos'}, $nnNadjTags);
		my @nnAdjSWords = $self->filterFactors($self->{$sysname}->[$i], $self->{'factorIndices'}->{'pos'}, $nnNadjTags);
		my ($sentWer, $tmp) = $self->sentenceWER(\@nnAdjSWords, \@nnAdjEWords, $self->{'factorIndices'}->{'surf'});
		$werScore += $sentWer;
		($sentWer, $tmp) = $self->sentencePWER(\@nnAdjSWords, \@nnAdjEWords, $self->{'factorIndices'}->{'surf'});
		$pwerScore += $sentWer;
	}
	
	#unhog memory
	$self->releaseSentences('truth');
	$self->releaseSentences($sysname);
	$self->{'nnAdjWERPWER'}->{$sysname} = [$werScore / $self->{'tokenCount'}->{'truth'}, $pwerScore / $self->{'tokenCount'}->{'truth'}];
	return @{$self->{'nnAdjWERPWER'}->{$sysname}};
}

#calculate detailed WER statistics and put them into $self
#arguments: system name, factor name to consider (default 'surf', surface form)
#return: overall surface WER for given system (w/o filtering)
#throw if given system or truth is not set or if index of factor name hasn't been specified
sub calcOverallWER
{
	my ($self, $sysname, $factorName) = (shift, shift, 'surf');
	if(scalar(@_) > 0) {$factorName = shift;}
	#check in-memory cache first
	if(exists $self->{'sysoutWER'}->{$sysname}->{$factorName})
	{
		return $self->{'sysoutWER'}->{$sysname}->{$factorName}->[0];
	}
	warn "calcing WER\n";
	
	$self->ensureFilenameDefined('truth');
	$self->ensureFilenameDefined($sysname);
	$self->ensureFactorPosDefined($factorName);
	$self->loadSentences('truth', $self->{'truthFilename'});
	$self->loadSentences($sysname, $self->{'sysoutFilenames'}->{$sysname});
	
	my ($wer, $swers, $indices) = $self->corpusWER($self->{$sysname}, $self->{'truth'}, $self->{'factorIndices'}->{$factorName});
	$self->{'sysoutWER'}->{$sysname}->{$factorName} = [$wer, $swers, $indices]; #total; arrayref of scores for individual sentences; arrayref of arrayrefs of offending words in each sentence
	
	#unhog memory
	$self->releaseSentences('truth');
	$self->releaseSentences($sysname);
	return $self->{'sysoutWER'}->{$sysname}->{$factorName}->[0] / $self->{'tokenCount'}->{'truth'};
}

#calculate detailed PWER statistics and put them into $self
#arguments: system name, factor name to consider (default 'surf')
#return: overall surface PWER for given system (w/o filtering)
#throw if given system or truth is not set or if index of factor name hasn't been specified
sub calcOverallPWER
{
	my ($self, $sysname, $factorName) = (shift, shift, 'surf');
	if(scalar(@_) > 0) {$factorName = shift;}
	#check in-memory cache first
	if(exists $self->{'sysoutPWER'}->{$sysname}->{$factorName})
	{
		return $self->{'sysoutPWER'}->{$sysname}->{$factorName}->[0];
	}
	warn "calcing PWER\n";
	
	$self->ensureFilenameDefined('truth');
	$self->ensureFilenameDefined($sysname);
	$self->ensureFactorPosDefined($factorName);
	$self->loadSentences('truth', $self->{'truthFilename'});
	$self->loadSentences($sysname, $self->{'sysoutFilenames'}->{$sysname});
	
	my ($pwer, $spwers, $indices) = $self->corpusPWER($self->{$sysname}, $self->{'truth'}, $self->{'factorIndices'}->{$factorName});
	$self->{'sysoutPWER'}->{$sysname}->{$factorName} = [$pwer, $spwers, $indices]; #total; arrayref of scores for individual sentences; arrayref of arrayrefs of offending words in each sentence
	
	#unhog memory
	$self->releaseSentences('truth');
	$self->releaseSentences($sysname);
	return $self->{'sysoutPWER'}->{$sysname}->{$factorName}->[0] / $self->{'tokenCount'}->{'truth'};
}

#arguments: system name, factor name to consider (default 'surf')
#return: array of (BLEU score, n-gram precisions, brevity penalty)
sub calcBLEU
{
	my ($self, $sysname, $factorName) = (shift, shift, 'surf');
	if(scalar(@_) > 0) {$factorName = shift;}
	#check in-memory cache first
	if(exists $self->{'bleuScores'}->{$sysname} && exists $self->{'bleuScores'}->{$sysname}->{$factorName})
	{
		return $self->{'bleuScores'}->{$sysname}->{$factorName};
	}
	warn "calcing BLEU\n";
	
	$self->ensureFilenameDefined('truth');
	$self->ensureFilenameDefined($sysname);
	$self->ensureFactorPosDefined($factorName);
	$self->loadSentences('truth', $self->{'truthFilename'});
	$self->loadSentences($sysname, $self->{'sysoutFilenames'}->{$sysname});
	
	#score structure: various total scores, arrayref of by-sentence score arrays
	if(!exists $self->{'bleuScores'}->{$sysname}) {$self->{'bleuScores'}->{$sysname} = {};}
	if(!exists $self->{'bleuScores'}->{$sysname}->{$factorName}) {$self->{'bleuScores'}->{$sysname}->{$factorName} = [[], []];}
	
	my ($good1, $tot1, $good2, $tot2, $good3, $tot3, $good4, $tot4, $totCLength, $totRLength) = (0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	my $factorIndex = $self->{'factorIndices'}->{$factorName};
	for(my $i = 0; $i < scalar(@{$self->{'truth'}}); $i++)
	{
		my ($truthSentence, $sysoutSentence) = ($self->{'truth'}->[$i], $self->{$sysname}->[$i]);
		my ($unigood, $unicount, $bigood, $bicount, $trigood, $tricount, $quadrugood, $quadrucount, $cLength, $rLength) = 
				$self->sentenceBLEU($truthSentence, $sysoutSentence, $factorIndex, 0); #last argument is whether to debug-print
		push @{$self->{'bleuScores'}->{$sysname}->{$factorName}->[1]}, [$unigood, $unicount, $bigood, $bicount, $trigood, $tricount, $quadrugood, $quadrucount, $cLength, $rLength];
		$good1 += $unigood; $tot1 += $unicount;
		$good2 += $bigood; $tot2 += $bicount;
		$good3 += $trigood; $tot3 += $tricount;
		$good4 += $quadrugood; $tot4 += $quadrucount;
		$totCLength += $cLength;
		$totRLength += $rLength;
	}
	my $brevity = ($totCLength > $totRLength || $totCLength == 0) ? 1 : exp(1 - $totRLength / $totCLength);
	my ($pct1, $pct2, $pct3, $pct4) = ($tot1 == 0 ? -1 : $good1 / $tot1, $tot2 == 0 ? -1 : $good2 / $tot2, 
													$tot3 == 0 ? -1 : $good3 / $tot3, $tot4 == 0 ? -1 : $good4 / $tot4);
	my ($logsum, $logcount) = (0, 0);
	if($tot1 > 0) {$logsum += my_log($pct1); $logcount++;}
	if($tot2 > 0) {$logsum += my_log($pct2); $logcount++;}
	if($tot3 > 0) {$logsum += my_log($pct3); $logcount++;}
	if($tot4 > 0) {$logsum += my_log($pct4); $logcount++;}
	my $bleu = $brevity * exp($logsum / $logcount);
	$self->{'bleuScores'}->{$sysname}->{$factorName}->[0] = [$bleu, 100 * $pct1, 100 * $pct2, 100 * $pct3, 100 * $pct4, $brevity];
  
	#unhog memory
	$self->releaseSentences('truth');
	$self->releaseSentences($sysname);
	return @{$self->{'bleuScores'}->{$sysname}->{$factorName}->[0]};
}

#do t-tests on the whole-corpus n-gram precisions vs. the average precisions over a set number of disjoint subsets
#arguments: system name, factor name BLEU was run on (default 'surf')
#return: arrayref of [arrayref of p-values for overall precision vs. subset average, arrayrefs of [(lower, upper) 95% credible intervals for true overall n-gram precisions]]
#
#written to try to save memory
sub statisticallyTestBLEUResults
{
	my ($self, $sysname, $factorName) = (shift, shift, 'surf');
	if(scalar(@_) > 0) {$factorName = shift;}
	#check in-memory cache first
	if(exists $self->{'bleuConfidence'}->{$sysname} && exists $self->{'bleuConfidence'}->{$sysname}->{$factorName})
	{
		return $self->{'bleuConfidence'}->{$sysname}->{$factorName};
	}
	warn "performing consistency tests\n";
	
	my $k = 30; #HARDCODED NUMBER OF SUBSETS (WE DO k-FOLD CROSS-VALIDATION); IF YOU CHANGE THIS YOU MUST ALSO CHANGE getApproxPValue() and $criticalTStat
	my $criticalTStat = 2.045; #hardcoded value given alpha (.025 here) and degrees of freedom (= $k - 1) ########################################
	$self->ensureFilenameDefined('truth');
	$self->ensureFilenameDefined($sysname);
	$self->ensureFactorPosDefined($factorName);
	
	#ensure we have full-corpus BLEU results
	if(!exists $self->{'bleuScores'}->{$sysname}->{$factorName})
	{
		$self->calcBLEU($sysname, $factorName);
	}
	if(!exists $self->{'subsetBLEUstats'}->{$sysname}) {$self->{'subsetBLEUstats'}->{$sysname} = {};}
	if(!exists $self->{'subsetBLEUstats'}->{$sysname}->{$factorName}) {$self->{'subsetBLEUstats'}->{$sysname}->{$factorName} = [];}
	
	#calculate n-gram precisions for each small subset
	my @sentenceStats = @{$self->{'bleuScores'}->{$sysname}->{$factorName}->[1]};
	for(my $i = 0; $i < $k; $i++)
	{
		my ($good1, $tot1, $good2, $tot2, $good3, $tot3, $good4, $tot4, $sysoutLength, $truthLength) = (0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		for(my $j = $i; $j < scalar(@sentenceStats); $j += $k) #subset #K consists of every Kth sentence
		{
			$good1 += $sentenceStats[$j]->[0]; $tot1 += $sentenceStats[$j]->[1];
			$good2 += $sentenceStats[$j]->[2]; $tot2 += $sentenceStats[$j]->[3];
			$good3 += $sentenceStats[$j]->[4]; $tot3 += $sentenceStats[$j]->[5];
			$good4 += $sentenceStats[$j]->[6]; $tot4 += $sentenceStats[$j]->[7];
			$sysoutLength += $sentenceStats[$j]->[8];
			$truthLength += $sentenceStats[$j]->[9];
		}
		push @{$self->{'subsetBLEUstats'}->{$sysname}->{$factorName}}, [$good1, $tot1, $good2, $tot2, $good3, $tot3, $good4, $tot4, $sysoutLength, $truthLength];
	}
	my $subsetStats = $self->{'subsetBLEUstats'}->{$sysname}->{$factorName};
	#calculate first two moments for subset scores for each n-gram precision, and t statistic
	my $fullCorpusBLEU = $self->{'bleuScores'}->{$sysname}->{$factorName}->[0]; #an arrayref
	my @means = (0) x 4;
	my @devs = (0) x 4;
	my $t = []; #t statistics for all n-gram orders
	if(!exists $self->{'bleuConfidence'}->{$sysname}) {$self->{'bleuConfidence'}->{$sysname} = {};}
	$self->{'bleuConfidence'}->{$sysname}->{$factorName} = [[], []]; #lower-bound p-values for whole corpus vs. subset average; confidence intervals for all n-gram orders
	for(my $i = 0; $i < 4; $i++) #run through n-gram orders
	{
		for(my $j = 0; $j < $k; $j++) #run through subsets
		{
			$means[$i] += $subsetStats->[$j]->[2 * $i] / $subsetStats->[$j]->[2 * $i + 1]; #matching / total n-grams
		}
		$means[$i] /= $k;
		for(my $j = 0; $j < $k; $j++) #run through subsets
		{
			$devs[$i] += ($subsetStats->[$j]->[2 * $i] / $subsetStats->[$j]->[2 * $i + 1] - $means[$i]) ** 2;
		}
		$devs[$i] = sqrt($devs[$i] / ($k - 1));
		$t->[$i] = ($fullCorpusBLEU->[$i + 1] / 100 - $means[$i]) / $devs[$i];
		push @{$self->{'bleuConfidence'}->{$sysname}->{$factorName}->[0]}, getLowerBoundPValue($t->[$i]); #p-value for overall score vs. subset average
		push @{$self->{'bleuConfidence'}->{$sysname}->{$factorName}->[1]}, 
							[$means[$i] - $criticalTStat * $devs[$i] / sqrt($k), $means[$i] + $criticalTStat * $devs[$i] / sqrt($k)]; #the confidence interval
	}
	
	return $self->{'bleuConfidence'}->{$sysname}->{$factorName};
}

#arguments: system name, factor name
#return: perplexity of language model (specified in a config file) wrt given system output
sub calcPerplexity
{
	my ($self, $sysname, $factorName) = @_;
	print STDERR "ppl $sysname $factorName\n";
	#check in-memory cache first
	if(exists $self->{'perplexity'}->{$sysname} && exists $self->{'perplexity'}->{$sysname}->{$factorName})
	{
		return $self->{'perplexity'}->{$sysname}->{$factorName};
	}
	warn "calcing perplexity\n";
	
	$self->ensureFilenameDefined($sysname);
	my $sysoutFilename;
	if($sysname eq 'truth' || $sysname eq 'input') {$sysoutFilename = $self->{"${sysname}Filename"};}
	else {$sysoutFilename = $self->{'sysoutFilenames'}->{$sysname};}
	my $lmFilename;
	if($sysname eq 'input') {$lmFilename = $self->{'inputLMs'}->{$factorName};}
	else {$lmFilename = $self->{'outputLMs'}->{$factorName};}
	my $tmpfile = ".tmp" . time;
	my $cmd = "perl ./extract-factors.pl $sysoutFilename " . $self->{'factorIndices'}->{$factorName} . " > $tmpfile";
	`$cmd`; #extract just the factor we're interested in; ngram doesn't understand factored notation
	my @output = `./ngram -lm $lmFilename -ppl $tmpfile`; #run the SRI n-gram tool
	`rm -f $tmpfile`;
	$output[1] =~ /ppl1=\s*([0-9\.]+)/;
	$self->{'perplexity'}->{$sysname}->{$factorName} = $1;
	return $self->{'perplexity'}->{$sysname}->{$factorName};
}

#run a paired t test and a sign test on BLEU statistics for subsets of both systems' outputs
#arguments: system name 1, system name 2, factor name
#return: arrayref of [arrayref of confidence levels for t test at which results differ, arrayref of index (0/1) of better system by t test,
#                     arrayref of confidence levels for sign test at which results differ, arrayref of index (0/1) of better system by sign test], 
#     where each inner arrayref has one element per n-gram order considered
sub statisticallyCompareSystemResults
{
	my ($self, $sysname1, $sysname2, $factorName) = @_;
	#check in-memory cache first
	if(exists $self->{'comparisonStats'}->{$sysname1} && exists $self->{'comparisonStats'}->{$sysname1}->{$sysname2} 
		&& exists $self->{'comparisonStats'}->{$sysname1}->{$sysname2}->{$factorName})
	{
		return $self->{'comparisonStats'}->{$sysname1}->{$sysname2}->{$factorName};
	}
	warn "comparing sysoutputs\n";
	
	$self->ensureFilenameDefined($sysname1);
	$self->ensureFilenameDefined($sysname2);
	$self->ensureFactorPosDefined($factorName);
	#make sure we have tallied results for both systems
	if(!exists $self->{'subsetBLEUstats'}->{$sysname1}->{$factorName}) {$self->statisticallyTestBLEUResults($sysname1, $factorName);}
	if(!exists $self->{'subsetBLEUstats'}->{$sysname2}->{$factorName}) {$self->statisticallyTestBLEUResults($sysname2, $factorName);}
	
	if(!exists $self->{'comparisonStats'}->{$sysname1}) {$self->{'comparisonStats'}->{$sysname1} = {};}
	if(!exists $self->{'comparisonStats'}->{$sysname1}->{$sysname2}) {$self->{'comparisonStats'}->{$sysname1}->{$sysname2} = {};}
	if(!exists $self->{'comparisonStats'}->{$sysname1}->{$sysname2}->{$factorName}) {$self->{'comparisonStats'}->{$sysname1}->{$sysname2}->{$factorName} = [];}
	my ($tConfidences, $tWinningIndices, $signConfidences, $signWinningIndices) = ([], [], [], []);
	for(my $i = 0; $i < 4; $i++) #loop over n-gram order
	{
		#t-test stats
		my ($mean, $dev) = (0, 0); #of the difference between the first and second systems' precisions
		#sign-test stats
		my ($nPlus, $nMinus) = (0, 0);
		my $j;
		for($j = 0; $j < scalar(@{$self->{'subsetBLEUstats'}->{$sysname1}->{$factorName}}); $j++)
		{
			my ($stats1, $stats2) = ($self->{'subsetBLEUstats'}->{$sysname1}->{$factorName}->[$j], $self->{'subsetBLEUstats'}->{$sysname2}->{$factorName}->[$j]);
			my ($prec1, $prec2) = ($stats1->[2 * $i] / $stats1->[2 * $i + 1], $stats2->[2 * $i] / $stats2->[2 * $i + 1]); #n-gram precisions
			$mean += $prec1 - $prec2;
			if($prec1 > $prec2) {$nPlus++;} else {$nMinus++;}
		}
		$mean /= $j;
		for($j = 0; $j < scalar(@{$self->{'subsetBLEUstats'}->{$sysname1}->{$factorName}}); $j++)
		{
			my ($stats1, $stats2) = ($self->{'subsetBLEUstats'}->{$sysname1}->{$factorName}->[$j], $self->{'subsetBLEUstats'}->{$sysname2}->{$factorName}->[$j]);
			my ($prec1, $prec2) = ($stats1->[2 * $i] / $stats1->[2 * $i + 1], $stats2->[2 * $i] / $stats2->[2 * $i + 1]); #n-gram precisions
			$dev += ($prec1 - $prec2 - $mean) ** 2;
		}
		$dev = sqrt($dev / (($j - 1) * $j)); #need the extra j because the variance of Xbar is 1/n the variance of X
		#t test
		my $t = $mean / $dev; #this isn't the standard form; remember the difference of the means is equal to the mean of the differences
		my $cc = getUpperBoundPValue($t);
		print STDERR "comparing at n=$i: mu $mean, sigma $dev, t $t -> conf >= " . (1 - $cc) . "\n";
		push @$tConfidences, $cc;
		push @$tWinningIndices, ($mean > 0) ? 0 : 1;
		#sign test
		my %binomialCoefficients; #map (n+ - n-) to a coefficient; compute on the fly!
		for(my $k = 0; $k <= $nPlus + $nMinus; $k++)
		{
			$binomialCoefficients{$k} = binCoeff($nPlus + $nMinus, $k);
		}
		my $sumCoeffs = 0;
		foreach my $coeff (values %binomialCoefficients) #get a lower bound on the probability mass inside (n+ - n-)
		{
			if($coeff > $binomialCoefficients{$nPlus}) {$sumCoeffs += $coeff;}
		}
		push @$signConfidences, $sumCoeffs;
		push @$signWinningIndices, ($nPlus > $nMinus) ? 0 : 1;
	}
	$self->{'comparisonStats'}->{$sysname1}->{$sysname2}->{$factorName} = [$tConfidences, $tWinningIndices, $signConfidences, $signWinningIndices];
	return $self->{'comparisonStats'}->{$sysname1}->{$sysname2}->{$factorName};
}

#write HTML to be displayed to compare the various versions we have of each sentence in the corpus;
#allow to filter which versions will be displayed
#(we don't write the whole page, just the contents of the body)
#arguments: filehandleref to which to write, regex to filter filename extensions to be included
#return: none
sub writeComparisonPage
{
	my ($self, $fh, $filter) = @_;
	my @filteredExtensions = grep($filter, ('e', 'f', keys %{$self->{'sysoutFilenames'}}));
	my %openedFiles = $self->openFiles(@filteredExtensions);
	my $id = 1; #sentence ID string
	while(my %lines = $self->readLineFromFiles(%openedFiles))
	{
		$self->printSingleSentenceComparison($fh, $id, %lines);
		$id++;
	}
	$self->closeFiles(%openedFiles);
}

##########################################################################################################
#####     INTERNAL     ###################################################################################
##########################################################################################################

#destructor!
#arguments: none
#return: none
sub DESTROY
{
	my $self = shift;
	$self->writeCacheFile();
}

#write all scores in memory to disk
#arguments: none
#return: none
sub writeCacheFile
{
	my $self = shift;
	if(!open(CACHEFILE, ">" . $self->{'cacheFilename'}))
	{
		warn "Corpus::writeCacheFile(): can't open '" . $self->{'cacheFilename'} . "' for write\n";
		return;
	}

	#store file changetimes to disk
	print CACHEFILE "File changetimes\n";
	my $ensureCtimeIsOutput = sub
	{
		my $ext = shift;
		#check for a previously read value
		if(exists $self->{'fileCtimes'}->{$ext} && $self->cacheIsCurrentForFile($ext)) {print CACHEFILE "$ext " . $self->{'fileCtimes'}->{$ext} . "\n";}
		else {print CACHEFILE "$ext " . time . "\n";} #our info must just have been calculated
	};
	if(exists $self->{'truthFilename'}) {&$ensureCtimeIsOutput('e');}
	if(exists $self->{'inputFilename'}) {&$ensureCtimeIsOutput('f');}
	foreach my $factorName (keys %{$self->{'phraseTableFilenames'}}) {&$ensureCtimeIsOutput("pt_$factorName");}
	foreach my $sysname (keys %{$self->{'sysoutFilenames'}}) {&$ensureCtimeIsOutput($sysname);}
	#store bleu scores to disk
	print CACHEFILE "\nBLEU scores\n";
	foreach my $sysname (keys %{$self->{'bleuScores'}})
	{
		foreach my $factorName (keys %{$self->{'bleuScores'}->{$sysname}})
		{
			print CACHEFILE "$sysname $factorName " . join(' ', @{$self->{'bleuScores'}->{$sysname}->{$factorName}->[0]});
			foreach my $sentenceBLEU (@{$self->{'bleuScores'}->{$sysname}->{$factorName}->[1]})
			{
				print CACHEFILE ";" . join(' ', @$sentenceBLEU);
			}
			print CACHEFILE "\n";
		}
	}
	#store t statistics for overall BLEU score and subsets in k-fold cross-validation
	print CACHEFILE "\nBLEU statistics\n";
	foreach my $sysname (keys %{$self->{'bleuConfidence'}})
	{
		foreach my $factorName (keys %{$self->{'bleuConfidence'}->{$sysname}})
		{
			print CACHEFILE "$sysname $factorName " . join(' ', @{$self->{'bleuConfidence'}->{$sysname}->{$factorName}->[0]});
			foreach my $subsetConfidence (@{$self->{'bleuConfidence'}->{$sysname}->{$factorName}->[1]})
			{
				print CACHEFILE ";" . join(' ', @$subsetConfidence);
			}
			print CACHEFILE "\n";
		}
	}
	#store statistics comparing system outputs
	print CACHEFILE "\nStatistical comparisons\n";
	foreach my $sysname1 (keys %{$self->{'comparisonStats'}})
	{
		foreach my $sysname2 (keys %{$self->{'comparisonStats'}->{$sysname1}})
		{
			foreach my $factorName (keys %{$self->{'comparisonStats'}->{$sysname1}->{$sysname2}})
			{
				print CACHEFILE "$sysname1 $sysname2 $factorName " . join(';', map {join(' ', @$_)} @{$self->{'comparisonStats'}->{$sysname1}->{$sysname2}->{$factorName}}) . "\n";
			}
		}
	}
	#store unknown-token counts to disk
	print CACHEFILE "\nUnknown-token counts\n";
	foreach my $factorName (keys %{$self->{'unknownCount'}})
	{
		print CACHEFILE $factorName . " " . $self->{'phraseTableFilenames'}->{$factorName} . " " . $self->{'unknownCount'}->{$factorName} . " " . $self->{'tokenCount'}->{'input'} . "\n";
	}
	#store WER, PWER to disk
	print CACHEFILE "\nWER scores\n";
	my $printWERFunc = 
	sub
	{
		my $werType = shift;
		foreach my $sysname (keys %{$self->{$werType}})
		{
			foreach my $factorName (keys %{$self->{$werType}->{$sysname}})
			{
				my ($totalWER, $sentenceWERs, $errorWords) = @{$self->{$werType}->{$sysname}->{$factorName}};
				print CACHEFILE "$werType $sysname $factorName $totalWER " . join(' ', @$sentenceWERs);
				foreach my $indices (@$errorWords)
				{
					print CACHEFILE ";" . join(' ', @$indices);
				}
				print CACHEFILE "\n";
			}
		}
	};
	&$printWERFunc('sysoutWER');
	&$printWERFunc('sysoutPWER');
	#store corpus perplexities to disk
	print CACHEFILE "\nPerplexity\n";
	foreach my $sysname (keys %{$self->{'perplexity'}})
	{
		foreach my $factorName (keys %{$self->{'perplexity'}->{$sysname}})
		{
			print CACHEFILE "$sysname $factorName " . $self->{'perplexity'}->{$sysname}->{$factorName} . "\n";
		}
	}
	print "\nNN/ADJ WER/PWER\n";
	foreach my $sysname (keys %{$self->{'nnAdjWERPWER'}})
	{
		print CACHEFILE "$sysname " . join(' ', @{$self->{'nnAdjWERPWER'}->{$sysname}}) . "\n";
	}
	print "\n";
	close(CACHEFILE);
}

#load all scores present in the cache file into the appropriate fields of $self
#arguments: none
#return: none
sub loadCacheFile
{
	my $self = shift;
	if(!open(CACHEFILE, "<" . $self->{'cacheFilename'}))
	{
		warn "Corpus::loadCacheFile(): can't open '" . $self->{'cacheFilename'} . "' for read\n";
		return;
	}
	my $mode = 'none';
	while(my $line = <CACHEFILE>)
	{
		next if $line =~ /^[ \t\n\r\x0a]*$/; #anyone know why char 10 (0x0a) shows up on empty lines, at least on solaris?
		chomp $line;
		#check for start of section
		if($line =~ /File changetimes/) {$mode = 'ctime';}
		elsif($line =~ /BLEU scores/) {$mode = 'bleu';}
		elsif($line =~ /BLEU statistics/) {$mode = 'bstats';}
		elsif($line =~ /Statistical comparisons/) {$mode = 'cmp';}
		elsif($line =~ /Unknown-token counts/) {$mode = 'unk';}
		elsif($line =~ /WER scores/) {$mode = 'wer';}
		elsif($line =~ /Perplexity/) {$mode = 'ppl';}
		elsif($line =~ /NN\/ADJ WER\/PWER/) {$mode = 'nawp';}
		#get data when in a mode already
		elsif($mode eq 'ctime')
		{
			local ($fileExtension, $ctime) = split(/\s+/, $line);
			$self->{'fileCtimes'}->{$fileExtension} = $ctime;
		}
		elsif($mode eq 'bleu')
		{
			local ($sysname, $factorName, $rest) = split(/\s+/, $line, 3);
			next if !$self->cacheIsCurrentForFile($sysname) || !$self->cacheIsCurrentForFile('e');
			if(!exists $self->{'bleuScores'}->{$sysname}) {$self->{'bleuScores'}->{$sysname} = {};}
			if(!exists $self->{'bleuScores'}->{$sysname}->{$factorName}) {$self->{'bleuScores'}->{$sysname}->{$factorName} = [[], []];}
			my @stats = map {my @tmp = split(/\s+/, $_); \@tmp;} split(/;/, $rest);
			print STDERR "bleu 1: " . join(', ', @{shift @stats}) . "\n";
			print STDERR "bleu 2: " . join(' ', map {"{" . join(', ', @$_) . "}"} @stats) . "\n";
		#	$self->{'bleuScores'}->{$sysname}->{$factorName}->[0] = shift @stats;
		#	$self->{'bleuScores'}->{$sysname}->{$factorName}->[1] = \@stats;
		}
		elsif($mode eq 'bstats')
		{
			local ($sysname, $factorName, $rest) = split(/\s+/, $line, 3);
			next if !$self->cacheIsCurrentForFile($sysname) || !$self->cacheIsCurrentForFile('e');
			if(!exists $self->{'bleuConfidence'}->{$sysname}) {$self->{'bleuConfidence'}->{$sysname} = {};}
			if(!exists $self->{'bleuConfidence'}->{$sysname}->{$factorName}) {$self->{'bleuConfidence'}->{$sysname}->{$factorName} = [[], []];}
			my @stats = map {my @tmp = split(/\s+/, $_); \@tmp;} split(/;/, $rest);
			$self->{'bleuConfidence'}->{$sysname}->{$factorName}->[0] = shift @stats;
			$self->{'bleuConfidence'}->{$sysname}->{$factorName}->[1] = \@stats;
		}
		elsif($mode eq 'cmp')
		{
			local ($sysname1, $sysname2, $factorName, $rest) = split(/\s+/, $line, 4);
			next if !$self->cacheIsCurrentForFile($sysname1) || !$self->cacheIsCurrentForFile($sysname2) || !$self->cacheIsCurrentForFile('e');
			if(!exists $self->{'comparisonStats'}->{$sysname1}) {$self->{'comparisonStats'}->{$sysname1} = {};}
			if(!exists $self->{'comparisonStats'}->{$sysname1}->{$sysname2}) {$self->{'comparisonStats'}->{$sysname1}->{$sysname2} = {};}
			if(!exists $self->{'comparisonStats'}->{$sysname1}->{$sysname2}->{$factorName}) {$self->{'comparisonStats'}->{$sysname1}->{$sysname2}->{$factorName} = [];}
			my @stats = map {my @x = split(' ', $_); \@x} split(/;/, $rest);
			$self->{'comparisonStats'}->{$sysname1}->{$sysname2}->{$factorName} = \@stats;
		}
		elsif($mode eq 'unk')
		{
			local ($factorName, $phraseTableFilename, $unknownCount, $totalCount) = split(' ', $line);
			next if !$self->cacheIsCurrentForFile('f') || !$self->cacheIsCurrentForFile("pt_$factorName");
			if(defined($self->{'phraseTableFilenames'}->{$factorName}) && $self->{'phraseTableFilenames'}->{$factorName} eq $phraseTableFilename)
			{
				$self->{'unknownCount'}->{$factorName} = $unknownCount;
				$self->{'totalTokens'} = $totalCount;
			}
		}
		elsif($mode eq 'wer')
		{
			local ($werType, $sysname, $factorName, $totalWER, $details) = split(/\s+/, $line, 5); #werType is 'sysoutWER' or 'sysoutPWER'
			next if !$self->cacheIsCurrentForFile($sysname) || !$self->cacheIsCurrentForFile('e');
			$details =~ /^([^;]*);(.*)/;
			my @sentenceWERs = split(/\s+/, $1);
			if(!exists $self->{$werType}->{$sysname}) {$self->{$werType}->{$sysname} = {};}
			$self->{$werType}->{$sysname}->{$factorName} = [$totalWER, \@sentenceWERs, []];
			my @indexLists = split(/;/, $2);
			for(my $i = 0; $i < scalar(@sentenceWERs); $i++)
			{
				my @indices = grep(/\S/, split(/\s+/, $indexLists[$i])); #find all nonempty tokens
				$self->{$werType}->{$sysname}->{$factorName}->[2] = \@indices;
			}
		}
		elsif($mode eq 'ppl')
		{
			local ($sysname, $factorName, $perplexity) = split(/\s+/, $line);
			next if !$self->cacheIsCurrentForFile($sysname);
			if(!exists $self->{'perplexity'}->{$sysname}) {$self->{'perplexity'}->{$sysname} = {};}
			$self->{'perplexity'}->{$sysname}->{$factorName} = $perplexity;
		}
		elsif($mode eq 'nawp')
		{
			local ($sysname, @scores) = split(/\s+/, $line);
			next if !$self->cacheIsCurrentForFile($sysname);
			$self->{'nnAdjWERPWER'}->{$sysname} = \@scores;
		}
	}
	close(CACHEFILE);
}

#arguments: cache type ('bleu' | ...), system name, factor name
#return: none
sub flushCache
{
	my ($self, $cacheType, $sysname, $factorName) = @_;
	if($cacheType eq 'bleu')
	{
		if(defined($self->{'bleuScores'}->{$sysname}) && defined($self->{'bleuScores'}->{$sysname}->{$factorName}))
		{
			delete $self->{'bleuScores'}->{$sysname}->{$factorName};
		}
	}
}

#arguments: file extension
#return: whether (0/1) our cache for the given file is at least as recent as the file
sub cacheIsCurrentForFile
{
	my ($self, $ext) = @_;
	return 0 if !exists $self->{'fileCtimes'}->{$ext} ;
	my @liveStats = stat($self->{'corpusName'} . ".$ext");
	return ($liveStats[9] <= $self->{'fileCtimes'}->{$ext}) ? 1 : 0;
}

##### utils #####
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
#arguments: x
sub round
{
	my $x = shift;
	if($x - int($x) < .5) {return int($x);}
	return int($x) + 1;
}

#return an approximation of the p-value for a given t FOR A HARDCODED NUMBER OF DEGREES OF FREEDOM
# (IF YOU CHANGE THIS HARDCODED NUMBER YOU MUST ALSO CHANGE statisticallyTestBLEUResults() and getLowerBoundPValue() )
#arguments: the t statistic, $t
#return: a lower bound on the probability mass outside (beyond) +/-$t in the t distribution
#
#for a wonderful t-distribution calculator, see <http://math.uc.edu/~brycw/classes/148/tables.htm#t>. UC.edu is Cincinnati.
sub getLowerBoundPValue
{
	my $t = abs(shift);
	#encode various known p-values for ###### DOF = 29 ######
	my %t2p = #since we're comparing (hopefully) very similar values, this chart is weighted toward the low end of the t-stat
	(
		0.0063 => .995,
		0.0126 => .99,
		0.0253 => .98,
		0.0380 => .97,
		0.0506 => .96,
		0.0633 => .95,
		0.0950 => .925,
		0.127  => .9,
		0.191  => .85,
		0.256  => .8,
		0.389  => .7,
		0.530  => .6,
		0.683  => .5,
		0.854  => .4,
		1.055  => .3,
		1.311  => .2, 
		1.699  => .1
	);
	foreach my $tCmp (sort keys %t2p) {return $t2p{$tCmp} if $t <= $tCmp;}
	return 0; #loosest bound ever! groovy, man
}
#arguments: the t statistic, $t
#return: an upper bound on the probability mass outside (beyond) +/-$t in the t distribution
sub getUpperBoundPValue
{
	my $t = abs(shift);
	#encode various known p-values for ###### DOF = 29 ######
	my %t2p = 
	(
		4.506 => .0001,
		4.254 => .0002,
		3.918 => .0005,
		3.659 => .001,
		3.396 => .002,
		3.038 => .005,
		2.756 => .01,
		2.462 => .02,
		2.045 => .05,
		1.699 => .1,
		1.311 => .2,
		0.683 => .5
	);
	foreach my $tCmp (reverse sort keys %t2p) {return $t2p{$tCmp} if $t >= $tCmp;}
	return 1; #loosest bound ever!
}

#arguments: n, r
#return: binomial coefficient for p = .5 (ie nCr * (1/2)^n)
sub binCoeff
{
	my ($n, $r) = @_;
	my $coeff = 1;
	for(my $i = $r + 1; $i <= $n; $i++) {$coeff *= $i; $coeff /= ($i - $r);}
	return $coeff * (.5 ** $n);
}

#throw if the given factor doesn't have an index defined
#arguments: factor name
#return: none
sub ensureFactorPosDefined
{
	my ($self, $factorName) = @_;
	if(!defined($self->{'factorIndices'}->{$factorName}))
	{
		throw Error::Simple(-text => "Corpus: no index known for factor '$factorName'\n");
	}
}

#throw if the filename field corresponding to the argument hasn't been defined
#arguments: 'truth' | 'input' | a system name
#return: none
sub ensureFilenameDefined
{
	my ($self, $sysname) = @_;
	if($sysname eq 'truth' || $sysname eq 'input')
	{
		if(!defined($self->{"${sysname}Filename"}))
		{
			throw Error::Simple(-text => "Corpus: no $sysname corpus defined\n");
		}
	}
	else
	{
		if(!defined($self->{'sysoutFilenames'}->{$sysname}))
		{
			throw Error::Simple(-text => "Corpus: no system $sysname defined\n");
		}
	}
}

#throw if there isn't a defined phrase-table filename for the given factor
#arguments: factor name
#return: none
sub ensurePhraseTableDefined
{
	my ($self, $factorName) = @_;
	if(!defined($self->{'phraseTableFilenames'}->{$factorName}))
	{
		throw Error::Simple(-text => "Corpus: no phrase table defined for factor '$factorName'\n");
	}
}

#search current directory for files with our corpus name as basename and set filename fields of $self
#arguments: hashref of filenames to descriptions
#return: none
sub locateFiles
{
	my ($self, $refDescs) = @_;
	open(DIR, "ls -x1 . |") or die "Corpus::locateFiles(): couldn't list current directory\n";
	my $corpusName = $self->{'corpusName'};
	while(my $filename = <DIR>)
	{
		chop $filename; #remove \n
		if($filename =~ /^$corpusName\.(.*)$/)
		{
			my $ext = $1;
			if($ext eq 'e') {$self->{'truthFilename'} = $filename;}
			elsif($ext eq 'f') {$self->{'inputFilename'} = $filename;}
			elsif($ext =~ /pt_(.*)/) {$self->{'phraseTableFilenames'}->{$1} = $filename;}
			else {$self->{'sysoutFilenames'}->{$ext} = $filename;}
			if(defined($refDescs->{$filename}))
			{
				$self->{'fileDescriptions'}->{$filename} = $refDescs->{$filename};
			}
		}
	}
	close(DIR);
}

#arguments: type ('truth' | 'input' | a string to represent a system output), filename
#pre: filename exists
#return: none
sub loadSentences
{
	my ($self, $sysname, $filename) = @_;
	#if the sentences are already loaded, leave them be
	if(exists $self->{$sysname} && scalar(@{$self->{$sysname}}) > 0) {return;}
	
	$self->{$sysname} = [];
	$self->{'tokenCount'}->{$sysname} = 0;
	open(INFILE, "<$filename") or die "Corpus::load(): couldn't open '$filename' for read\n";
	while(my $line = <INFILE>)
	{
		my @words = split(/\s+/, $line);
		$self->{'tokenCount'}->{$sysname} += scalar(@words);
		my $refFactors = [];
		foreach my $word (@words)
		{
			my @factors = split(/\|/, $word);
			push @$refFactors, \@factors;
		}
		push @{$self->{$sysname}}, $refFactors;
	}
	close(INFILE);	
}

#free the memory used for the given corpus (but NOT any associated calculations, eg WER)
#arguments: type ('truth' | 'input' | a string to represent a system output)
#return: none
sub releaseSentences
{
#	my ($self, $sysname) = @_;
#	$self->{$sysname} = [];
}

#arguments: factor name
#return: none
#throw if we don't have a filename for the given phrase table
sub loadPhraseTable
{
	my ($self, $factorName) = @_;
	$self->ensurePhraseTableDefined($factorName);
	
	my $filename = $self->{'phraseTableFilenames'}->{$factorName};
	open(PTABLE, "<$filename") or die "couldn't open '$filename' for read\n";
	$self->{'phraseTables'}->{$factorName} = {}; #create ref to phrase table (hash of strings, for source phrases, to anything whatsoever)
	#assume the table is sorted so that duplicate source phrases will be consecutive
	while(my $line = <PTABLE>)
	{
		my @phrases = split(/\s*\|\|\|\s*/, $line, 2);
		$self->{'phraseTables'}->{$factorName}->{$phrases[0]} = 0; #just so that it's set to something
	}
	close(PTABLE);
}

#arguments: factor name
#return: none
sub releasePhraseTable
{
	my ($self, $factorName) = @_;
	$self->{'phraseTables'}->{$factorName} = {};
}

#arguments: name of list ('nounAndAdj' | ...)
#return: arrayref of strings (postags)
sub getPOSTagList
{
	my ($self, $listname) = @_;
	##### assume PTB tagset #####
	if($listname eq 'nounAndAdj') {return ['NN', 'NNS', 'NNP', 'NNPS', 'JJ', 'JJR', 'JJS'];}
#	if($listname eq '') {return [];}
}

#arguments: list to be filtered (arrayref of arrayrefs of factor strings), desired factor index, arrayref of allowable values
#return: filtered list as array of arrayrefs of factor strings
sub filterFactors
{
	my ($self, $refFullList, $index, $refFactorValues) = @_;
	my $valuesRegex = join("|", @$refFactorValues);
	my @filteredList = ();
	foreach my $factors (@$refFullList)
	{
		if($factors->[$index] =~ m/$valuesRegex/)
		{
			push @filteredList, $factors;
		}
	}
	return @filteredList;
}

#arguments: system output (arrayref of arrayrefs of arrayrefs of factor strings), truth (same), factor index to use
#return: wer score, arrayref of sentence scores, arrayref of arrayrefs of indices of errorful words
sub corpusWER
{
	my ($self, $refSysOutput, $refTruth, $index) = @_;
	my ($totWER, $sentenceWER, $errIndices) = (0, [], []);
	for(my $i = 0; $i < scalar(@$refSysOutput); $i++)
	{
		my ($sentWER, $indices) = $self->sentenceWER($refSysOutput->[$i], $refTruth->[$i], $index);
		$totWER += $sentWER;
		push @$sentenceWER, $sentWER;
		push @$errIndices, $indices;
	}
	return ($totWER, $sentenceWER, $errIndices);
}

#arguments: system output (arrayref of arrayrefs of factor strings), truth (same), factor index to use
#return: wer score, arrayref of arrayrefs of indices of errorful words
sub sentenceWER
{
	#constants: direction we came through the table
	my ($DIR_NONE, $DIR_SKIPTRUTH, $DIR_SKIPOUT, $DIR_SKIPBOTH) = (-1, 0, 1, 2); #values don't matter but must be unique
	my ($self, $refSysOutput, $refTruth, $index) = @_;
	my ($totWER, $indices) = (0, []);
	my ($sLength, $eLength) = (scalar(@$refSysOutput), scalar(@$refTruth));
	if($sLength == 0 || $eLength == 0) {return ($totWER, $indices);} #special case
	
	my @refWordsMatchIndices = (-1) x $eLength; #at what sysout-word index this truth word is first matched
	my @sysoutWordsMatchIndices = (-1) x $sLength; #at what truth-word index this sysout word is first matched
	my $table = []; #index by sysout word index, then truth word index; a cell holds max count of matching words and direction we came to get it
	#dynamic-programming time: find the path through the table with the maximum number of matching words
	for(my $i = 0; $i < $sLength; $i++)
	{
		push @$table, [];
		for(my $j = 0; $j < $eLength; $j++)
		{
			my ($maxPrev, $prevDir) = (0, $DIR_NONE);
			if($i > 0 && $table->[$i - 1]->[$j]->[0] >= $maxPrev) {$maxPrev = $table->[$i - 1]->[$j]->[0]; $prevDir = $DIR_SKIPOUT;}
			if($j > 0 && $table->[$i]->[$j - 1]->[0] >= $maxPrev) {$maxPrev = $table->[$i]->[$j - 1]->[0]; $prevDir = $DIR_SKIPTRUTH;}
			if($i > 0 && $j > 0 && $table->[$i - 1]->[$j - 1]->[0] >= $maxPrev) {$maxPrev = $table->[$i - 1]->[$j - 1]->[0]; $prevDir = $DIR_SKIPBOTH;}
			my $match = ($refSysOutput->[$i]->[$index] eq $refTruth->[$j]->[$index] && $refWordsMatchIndices[$j] == -1 && $sysoutWordsMatchIndices[$i] == -1) ? 1 : 0;
			if($match == 1) {$refWordsMatchIndices[$j] = $i; $sysoutWordsMatchIndices[$i] = $j;}
			push @{$table->[$i]}, [($match ? $maxPrev + 1 : $maxPrev), $prevDir];
		}
	}
	
	#look back along the path and get indices of non-matching words
	my @unusedSysout = (0) x $sLength; #whether each sysout word was matched--used for outputting html table
	my ($i, $j) = ($sLength - 1, $eLength - 1);
	while($i > 0) #work our way back to the first sysout word
	{
		push @{$table->[$i]->[$j]}, 0; #length is flag to highlight cell
		if($table->[$i]->[$j]->[1] == $DIR_SKIPTRUTH)
		{
			$j--;
		}
		elsif($table->[$i]->[$j]->[1] == $DIR_SKIPOUT)
		{
			if($table->[$i - 1]->[$j]->[0] == $table->[$i]->[$j]->[0]) {unshift @$indices, $i; $unusedSysout[$i] = 1;}
			$i--;
		}
		elsif($table->[$i]->[$j]->[1] == $DIR_SKIPBOTH)
		{
			if($table->[$i - 1]->[$j - 1]->[0] == $table->[$i]->[$j]->[0]) {unshift @$indices, $i; $unusedSysout[$i] = 1;}
			$i--; $j--;
		}
	}
	#we're at the first sysout word; finish up checking for matches
	while($j > 0 && $refWordsMatchIndices[$j] != 0) {push @{$table->[0]->[$j]}, 0; $j--;}
	if($j == 0 && $refWordsMatchIndices[0] != 0) {unshift @$indices, 0; $unusedSysout[0] = 1;} #no truth word was matched to the first sysout word
	
	#print some HTML to debug the WER algorithm
#	print "<table border=1><tr><td></td><td>" . join("</td><td>", map {() . $_->[$index]} @$refTruth) . "</td></tr>";
#	for(my $i = 0; $i < $sLength; $i++)
#	{
#		print "<tr><td" . (($unusedSysout[$i] == 1) ? " style=\"background-color: #ffdd88\">" : ">") . $refSysOutput->[$i]->[$index] . "</td>";
#		for(my $j = 0; $j < $eLength; $j++)
#		{
#			print "<td";
#			if(scalar(@{$table->[$i]->[$j]}) > 2) {print " style=\"color: yellow; background-color: #000080\"";}
#			my $arrow;
#			if($table->[$i]->[$j]->[1] == $DIR_NONE) {$arrow = "&times;";}
#			elsif($table->[$i]->[$j]->[1] == $DIR_SKIPTRUTH) {$arrow = "&larr;";}
#			elsif($table->[$i]->[$j]->[1] == $DIR_SKIPOUT) {$arrow = "&uarr;";}
#			elsif($table->[$i]->[$j]->[1] == $DIR_SKIPBOTH) {$arrow = "&loz;";}
#			print ">" . $table->[$i]->[$j]->[0] . "  " . $arrow . "</td>";
#		}
#		print "</tr>";
#	}
#	print "</table>";
	
	my $matchCount = 0;
	if($sLength > 0) {$matchCount = $table->[$sLength - 1]->[$eLength - 1]->[0];}
	return ($sLength - $matchCount, $indices);
}

#arguments: system output (arrayref of arrayrefs of arrayrefs of factor strings), truth (same), factor index to use
#return: wer score, arrayref of sentence scores, arrayref of arrayrefs of indices of errorful words
sub corpusPWER
{
	my ($self, $refSysOutput, $refTruth, $index) = @_;
	my ($totWER, $sentenceWER, $errIndices) = (0, [], []);
	for(my $i = 0; $i < scalar(@$refSysOutput); $i++)
	{
		my ($sentWER, $indices) = $self->sentencePWER($refSysOutput->[$i], $refTruth->[$i], $index);
		$totWER += $sentWER;
		push @$sentenceWER, $sentWER;
		push @$errIndices, $indices;
	}
	return ($totWER, $sentenceWER, $errIndices);
}

#arguments: system output (arrayref of arrayrefs of factor strings), truth (same), factor index to use
#return: wer score, arrayref of arrayrefs of indices of errorful words
sub sentencePWER
{
	my ($self, $refSysOutput, $refTruth, $index) = @_;
	my ($totWER, $indices) = (0, []);
	my ($sLength, $eLength) = (scalar(@$refSysOutput), scalar(@$refTruth));
	my @truthWordUsed = (0) x $eLength; #array of 0/1; can only match a given truth word once
	for(my $j = 0; $j < $sLength; $j++)
	{
		my $found = 0;
		for(my $k = 0; $k < $eLength; $k++) #check output word against entire truth sentence
		{
			if(lc $refSysOutput->[$j]->[$index] eq lc $refTruth->[$k]->[$index] && $truthWordUsed[$k] == 0)
			{
				$truthWordUsed[$k] = 1;
				$found = 1;
				last;
			}
		}
		if($found == 0)
		{
			$totWER++;
			push @$indices, $j;
		}
	}
	return ($totWER, $indices);
}

#BLEU calculation for a single sentence
#arguments: truth sentence (arrayref of arrayrefs of factor strings), sysout sentence (same), factor index to use
#return: 1- through 4-gram matching and total counts (1-g match, 1-g tot, 2-g match...), candidate length, reference length
sub sentenceBLEU
{
	my ($self, $refTruth, $refSysOutput, $factorIndex, $debug) = @_;
	my ($length_reference, $length_translation) = (scalar(@$refTruth), scalar(@$refSysOutput));
	my ($correct1, $correct2, $correct3, $correct4, $total1, $total2, $total3, $total4) = (0, 0, 0, 0, 0, 0, 0, 0);
	my %REF_GRAM = ();
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
      }
      next if $i<1;
      $gram = $refSysOutput->[$i - 1]->[$factorIndex] ." ".$gram;
      if (defined($REF_GRAM{$gram}) && $REF_GRAM{$gram} > 0) {
			$REF_GRAM{$gram}--;
			$correct2++;
      }
      next if $i<2;
      $gram = $refSysOutput->[$i - 2]->[$factorIndex] ." ".$gram;
      if (defined($REF_GRAM{$gram}) && $REF_GRAM{$gram} > 0) {
			$REF_GRAM{$gram}--;
			$correct3++;
      }
      next if $i<3;
      $gram = $refSysOutput->[$i - 3]->[$factorIndex] ." ".$gram;
      if (defined($REF_GRAM{$gram}) && $REF_GRAM{$gram} > 0) {
			$REF_GRAM{$gram}--;
			$correct4++;
      }
	}
	my $total = $length_translation;
	$total1 = max(1, $total);
	$total2 = max(1, $total - 1);
	$total3 = max(1, $total - 2);
	$total4 = max(1, $total - 3);
	
	return ($correct1, $total1, $correct2, $total2, $correct3, $total3, $correct4, $total4, $length_translation, $length_reference);
}

##### filesystem #####

#open as many given files as possible; only warn about the rest
#arguments: list of filename extensions to open (assume corpus name is file title)
#return: hash from type string to filehandleref, giving all files that were successfully opened
sub openFiles
{
	my ($self, @extensions) = @_;
	my %openedFiles = ();
	foreach my $ext (@extensions)
	{
		if(!open(FILE, "<" . $self->{'corpusName'} . $ext))
		{
			warn "Corpus::openFiles(): couldn't open '" . $self->{'corpusName'} . $ext . "' for read\n";
		}
		else #success
		{
			$openedFiles{$ext} = \*FILE;
		}
	}
	return %openedFiles;
}

#read one line from each given file
#arguments: hash from type string to filehandleref
#return: hash from type string to sentence (stored as arrayref of arrayrefs of factors) read from corresponding file
sub readLineFromFiles
{
	my ($self, %openedFiles) = @_;
	my %lines;
	foreach my $type (keys %openedFiles)
	{
		$lines{$type} = [];
		my $sentence = <$openedFiles{$type}>;
		my @words = split(/\s+/, $sentence);
		foreach my $word (@words)
		{
			my @factors = split(/\|/, $word);
			push @{$lines{$type}}, \@factors;
		}
	}
	return %lines;
}

#close all given files
#arguments: hash from type string to filehandleref
#return: none
sub closeFiles
{
	my ($self, %openedFiles) = @_;
	foreach my $type (keys %openedFiles)
	{
		close($openedFiles{$type});
	}
}

##### write HTML #####

#print HTML for comparing various versions of a sentence, with special processing for each version as appropriate
#arguments: filehandleref to which to write, sentence ID string, hashref of version string to sentence (stored as arrayref of arrayref of factor strings)
#return: none
sub printSingleSentenceComparison
{
	my ($self, $fh, $sentID, $sentences) = @_;
	my $curFH = select;
	select $fh;
	#javascript to reorder rows to look nice afterward
	print "<script type=\"text/javascript\">
	function reorder_$sentID()
	{/*
		var table = document.getElementById('div_$sentID').firstChild;
		var refTransRow = table.getElementById('row_e');
		var inputRow = table.getElementById('row_f');
		table.removeRow(refTransRow);
		table.removeRow(inputRow);
		var newRow1 = table.insertRow(0);
		var newRow2 = table.insertRow(1);
		newRow1.childNodes = inputRow.childNodes;
		newRow2.childNodes = refTransRow.childNodes;*/
	}
	</script>";
	#html for sentences
	print "<div id=\"div_$sentID\" style=\"padding: 3px; margin: 5px\">";
	print "<table border=\"1\">";
#	my $rowCount = 0;
#	my @bgColors = ("#ffefbf", "#ffdf7f");
	#process all rows in order
	foreach my $sentType (keys %$sentences)
	{
		my $bgcolor = $bgColors[$rowCount % 2];
		print "<tr id=\"row_$sentType\"><td align=right>";
		#description of sentence
		if(defined($self->{'fileDescriptions'}->{$self->{'corpusName'} . $sentType}))
		{
			print "(" . $self->{'fileDescriptions'}->{$self->{'corpusName'} . $sentType} . ")";
		}
		else
		{
			print "($sentType)";
		}
		print "</td><td align=left>";
		#sentence with markup
		if($sentType eq 'f') #input
		{
#			$self->writeHTMLSentenceWithFactors($fh, $sentences->{$sentType}, $inputColor);
		}
		elsif($sentType eq 'e') #reference translation
		{
#			$self->writeHTMLSentenceWithFactors($fh, $sentences->{$sentType}, $reftransColor);
		}
		else #system output
		{
#			$self->writeHTMLTranslationHighlightedWithFactors($fh, $sentences->{$sentType}, $sentences->{'e'}, $highlightColors);
		}
		print "</td></tr>";
#		$rowCount++;
	}
	print "</table>";
	print "</div>\n";
	select $curFH;
}

#print contents of all fields of this object, with useful formatting for arrayrefs and hashrefs
#arguments: none
#return: none
sub printDetails
{
	my $self = shift;
	foreach my $key (keys %$self)
	{
		if(ref($self->{$key}) eq 'HASH')
		{
			print STDERR "obj: $key => {" . join(', ', map {"$_ => " . $self->{$key}->{$_}} (keys %{$self->{$key}})) . "}\n";
		}
		elsif(ref($self->{$key}) eq 'ARRAY')
		{
			print STDERR "obj: $key => (" . join(', ', @{$self->{$key}}) . ")\n";
		}
		elsif(ref($self->{$key}) eq '') #not a reference
		{
			print STDERR "obj: $key => " . $self->{$key} . "\n";
		}
	}
}
