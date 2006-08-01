#package Corpus: hold a bunch of sentences in any language, with translation factors and stats about individual sentences and the corpus as a whole
#Evan Herbst, 7 / 25 / 06

package Corpus;
BEGIN
{
	push @INC, ".";
}
use Error;

return 1;

###########################################################################################################################

#'our' variables are available outside the package
our @FACTORNAMES = ('surf', 'pos', 'lemma', 'stem', 'morph');

#constructor
#arguments: short corpus name (-name), hashref of filenames to descriptions (-descriptions), hashref of factor names to indices in this corpus (-indices)
sub new
{
	my $class = shift;
	my %args = @_; #turn the remainder of @_ into a hash
	my ($corpusName, $refFileDescs, $factorIndices) = ($args{'-name'}, $args{'-descriptions'}, $args{'-indices'});
	my $self = {};
	$self->{'corpusName'} = $corpusName;
	$self->{'truth'} = []; #arrayref of arrayrefs of factors
	$self->{'input'} = []; #same; also same for any system outputs that get loaded
	$self->{'truthFilename'} = "";
	$self->{'inputFilename'} = "";
	$self->{'sysoutFilenames'} = {}; #hashref of (string => string) for (system name, filename)
	$self->{'phraseTableFilenames'} = {}; #factor name (from @FACTORNAMES) => filename
	$self->{'factorIndices'} = {}; #factor name => index; names ought to be strings from the standard list in @FACTORNAMES
	%{$self->{'factorIndices'}} = %$factorIndices;
	$self->{'phraseTables'} = {}; #factor name (from @FACTORNAMES) => hashref of source phrases to anything; used for unknown-word counting
	$self->{'unknownCount'} = {}; #factor name => count of unknown tokens in input
	$self->{'totalTokens'} = 0; #useful with counts of unknown tokens
	$self->{'sysoutWER'} = {}; #system name => (factor name => arrayref with system output total WER and arrayref of WER scores for individual sysout sentences wrt truth)
	$self->{'sysoutPWER'} = {}; #similarly
	$self->{'fileDescriptions'} = {}; #filename associated with us => string description of file
	$self->{'bleuScores'} = {}; #system name => (factor name => arrayref of (overall score, arrayref of per-sentence scores) )
	$self->{'cacheFilename'} = "cache/$corpusName.cache"; #all memory of various scores is stored here
	bless $self, $class;
	$self->locateFiles($refFileDescs); #find all relevant files in the current directory; set filenames and descriptions
	$self->loadCacheFile();
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

#arguments: hashref of factor names to indices (not all factors in @FACTORNAMES must be included, but no strings not from the list should be)
#return: none
sub setFactorIndices
{
	my ($self, $refIndices) = @_;
	%{$self->{'factorIndices'}} = %{$refIndices};
}

#calculate the number of unknown factor values for the given factor in the input file
#arguments: factor name
#return: unknown factor count, total factor count (note the total doesn't depend on the factor)
#throw if we don't have an input file or a phrase table for the given factor defined or if there's no index known for the given factor
sub calcUnknownTokens
{
	my ($self, $factorName) = @_;
	#check in-memory cache first
	if(defined($self->{'unknownCount'}->{$factorName}))
	{
		return ($self->{'unknownCount'}->{$factorName}, $self->{'totalTokens'});
	}
	
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
	$self->{'totalTokens'} = $totalTokens;
	
	return ($unknownTokens, $totalTokens);
}

#arguments: system name
#return: (WER, PWER) for nouns and adjectives in given system wrt truth
#throw if given system or truth is not set or if index of 'surf' or 'pos' hasn't been specified
sub calcNounAdjWER_PWERDiff
{
	my ($self, $sysname) = @_;
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
#		warn "truth " . join(', ', map {join('|', @$_)} @{$self->{'truth'}->[$i]}) . "\n";
#		warn "sysout " . join(', ', map {join('|', @$_)} @{$self->{$sysname}->[$i]}) . "\n";
		my @nnAdjEWords = $self->filterFactors($self->{'truth'}->[$i], $self->{'factorIndices'}->{'pos'}, $nnNadjTags);
		my @nnAdjSWords = $self->filterFactors($self->{$sysname}->[$i], $self->{'factorIndices'}->{'pos'}, $nnNadjTags);
#		warn "filtered truth: " . join(' ', map {join('|', @$_)} @nnAdjEWords) . "\n";
#		warn "filtered sysout: " . join(' ', map {join('|', @$_)} @nnAdjSWords) . "\n\n\n";
		my ($sentWer, $tmp) = $self->sentenceWER(\@nnAdjSWords, \@nnAdjEWords, $self->{'factorIndices'}->{'surf'});
		$werScore += $sentWer;
		($sentWer, $tmp) = $self->sentencePWER(\@nnAdjSWords, \@nnAdjEWords, $self->{'factorIndices'}->{'surf'});
		$pwerScore += $sentWer;
	}
	
	#unhog memory
	$self->releaseSentences('truth');
	$self->releaseSentences($sysname);
	return ($werScore, $pwerScore);
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
	if(defined($self->{'sysoutWER'}->{$sysname}->{$factorName}))
	{
		return $self->{'sysoutWER'}->{$sysname}->{$factorName}->[0];
	}
	
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
	return $self->{'sysoutWER'}->{$sysname}->{$factorName}->[0];
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
	if(defined($self->{'sysoutPWER'}->{$sysname}->{$factorName}))
	{
		return $self->{'sysoutPWER'}->{$sysname}->{$factorName}->[0];
	}
	
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
	return $self->{'sysoutPWER'}->{$sysname}->{$factorName}->[0];
}

#arguments: system name, factor name to consider (default 'surf')
#return: BLEU score, n-gram precisions, brevity penalty
sub calcBLEU
{
	my ($self, $sysname, $factorName) = (shift, shift, 'surf');
	if(scalar(@_) > 0) {$factorName = shift;}
	#check in-memory cache first
	if(defined($self->{'bleuScores'}->{$sysname}->{$factorName}))
	{
		return $self->{'bleuScores'}->{$sysname}->{$factorName};
	}
	
	$self->ensureFilenameDefined('truth');
	$self->ensureFilenameDefined($sysname);
	$self->ensureFactorPosDefined($factorName);
	$self->loadSentences('truth', $self->{'truthFilename'});
	$self->loadSentences($sysname, $self->{'sysoutFilenames'}->{$sysname});
	
	#score structure: various total scores, arrayref of by-sentence score arrays
	if(!defined($self->{'bleuScores'}->{$sysname})) {$self->{'bleuScores'}->{$sysname} = {};}
	if(!defined($self->{'bleuScores'}->{$sysname}->{$factorName})) {$self->{'bleuScores'}->{$sysname}->{$factorName} = [[], []];}
	
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
	my ($pct1, $pct2, $pct3, $pct4) = ($good1 / $tot1, $good2 / $tot2, $good3 / $tot3, $good4 / $tot4);
#	warn sprintf("n-gram prec: %d/%d = %.3lf, %d/%d = %.3lf, %d/%d = %.3lf, %d/%d = %.3lf\n", 
#						$good1, $tot1, $pct1, $good2, $tot2, $pct2, $good3, $tot3, $pct3, $good4, $tot4, $pct4);
	my $bleu = $brevity * exp((my_log($pct1) + my_log($pct2) + my_log($pct3) + my_log($pct4)) / 4);
#	warn sprintf("brevity: %.3lf (%d ref, %d sys)\n", $brevity, $totRLength, $totCLength);
	sleep 8;
	$self->{'bleuScores'}->{$sysname}->{$factorName}->[0] = [$bleu, 100 * $pct1, 100 * $pct2, 100 * $pct3, 100 * $pct4, $brevity];
  
	#unhog memory
	$self->releaseSentences('truth');
	$self->releaseSentences($sysname);
	return @{$self->{'bleuScores'}->{$sysname}->{$factorName}->[0]};
}

#write HTML to be displayed to compare the various versions we have of each sentence in the corpus;
#allow filtering of which versions will be displayed
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
	#store bleu scores to disk
	print CACHEFILE "\nBLEU scores\n";
	foreach my $sysname (keys %{$self->{'bleuScores'}})
	{
		foreach my $factorName (keys %{$self->{'bleuScores'}->{$sysname}})
		{
			print CACHEFILE "$sysname $factorName " . join(' ', @{$self->{'bleuScores'}->{$sysname}->{$factorName}->[0]});
			foreach my $sentenceBLEU (@{$self->{'bleuScores'}->{$sysname}->{$factorName}->[1]})
			{
				print CACHEFILE "; " . join(' ', @$sentenceBLEU);
			}
			print CACHEFILE "\n";
		}
	}
	#store unknown-token counts to disk
	print CACHEFILE "\nUnknown-token counts\n";
	foreach my $factorName (keys %{$self->{'unknownCount'}})
	{
		print CACHEFILE $factorName . " " . $self->{'phraseTableFilenames'}->{$factorName} . " " . $self->{'unknownCount'}->{$factorName} . " " . $self->{'totalTokens'} . "\n";
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
	#store misc scores to disk
	print CACHEFILE "\nMisc scores\n";
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
		chop $line;
		#check for start of section
		if($line eq "File changetimes\n") {$mode = 'ctime';}
		elsif($line eq "BLEU scores\n") {$mode = 'bleu';}
		elsif($line eq "Unknown-token counts\n") {$mode = 'unk';}
		elsif($line eq "WER scores") {$mode = 'wer';}
		elsif($line eq "Misc scores") {$mode = 'misc';}
		#get data when in a mode already
		elsif($mode eq 'ctime')
		{
		}
		elsif($mode eq 'bleu')
		{
			local ($sysname, $factorName, $rest) = split(/\s+/, $line, 3);
			if(!defined($self->{'bleuScores'}->{$sysname})) {$self->{'bleuScores'}->{$sysname} = {};}
			if(!defined($self->{'bleuScores'}->{$sysname}->{$factorName})) {$self->{'bleuScores'}->{$sysname}->{$factorName} = [[], []];}
			my @stats = map {my @tmp = split(/\s+/, $_); \@tmp;} split(/;/, $rest);
			$self->{'bleuScores'}->{$sysname}->{$factorName}->[0] = shift @stats;
			$self->{'bleuScores'}->{$sysname}->{$factorName}->[1] = \@stats;
		}
		elsif($mode eq 'unk')
		{
			local ($factorName, $phraseTableFilename, $unknownCount, $totalCount) = split(' ', $line);
			if(defined($self->{'phraseTableFilenames'}->{$factorName}) && $self->{'phraseTableFilenames'}->{$factorName} eq $phraseTableFilename)
			{
				$self->{'unknownCount'}->{$factorName} = $unknownCount;
				$self->{'totalTokens'} = $totalCount;
			}
		}
		elsif($mode eq 'wer')
		{
			local ($werType, $sysname, $factorName, $totalWER, $details) = split(/\s+/, $line, 5); #werType is 'sysoutWER' or 'sysoutPWER'
			$details =~ /^([^;]*);(.*)/;
			my @sentenceWERs = split(/\s+/, $1);
			if(!defined($self->{$werType}->{$sysname})) {$self->{$werType}->{$sysname} = {};}
			$self->{$werType}->{$sysname}->{$factorName} = [$totalWER, \@sentenceWERs, []];
			my @indexLists = split(/;/, $2);
			sleep 6;
			for(my $i = 0; $i < scalar(@sentenceWERs); $i++)
			{
				my @indices = split(/\s+/, $indexLists[$i]);
				$self->{$werType}->{$sysname}->{$factorName}->[2] = \@indices;
			}
		}
		elsif($mode eq 'misc')
		{
		}
	}
	close(CACHEFILE);
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
#	warn "truth is set to '" . $self->{'truthFilename'} . "'\n";
#	warn "input is set to '" . $self->{'inputFilename'} . "'\n";
#	warn "sysouts set are '" . join("', '", keys %{$self->{'sysoutFilenames'}}) . "'\n";
#	warn "phrase tables set are '" . join("', '", keys %{$self->{'phraseTableFilenames'}}) . "'\n";
}

#arguments: type ('truth' | 'input' | a string to represent a system output), filename
#pre: filename exists
#return: none
sub loadSentences
{
	my ($self, $sysname, $filename) = @_;
	$self->{$sysname} = [];
	open(INFILE, "<$filename") or die "Corpus::load(): couldn't open '$filename' for read\n";
	while(my $line = <INFILE>)
	{
		my @words = split(/\s+/, $line);
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
	my ($self, $sysname) = @_;
	$self->{$sysname} = [];
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
	my ($self, $refSysOutput, $refTruth, $index) = @_;
	my ($totWER, $indices) = (0, []);
	my ($sLength, $eLength) = (scalar(@$refSysOutput), scalar(@$refTruth));
	for(my $j = 0; $j < min($sLength, $eLength); $j++)
	{
		if(lc $refSysOutput->[$j]->[$index] ne lc $refTruth->[$j]->[$index]) #check output word against truth word in same position
		{
			$totWER++;
			push @$indices, $j;
		}
	}
	$totWER += max(0, $sLength - $eLength);
	return ($totWER, $indices);
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
			if($debug != 0) {warn "'$gram' ";}
			$correct1 += 1;
      }
      next if $i<1;
      $gram = $refSysOutput->[$i - 1]->[$factorIndex] ." ".$gram;
      if (defined($REF_GRAM{$gram}) && $REF_GRAM{$gram} > 0) {
			$REF_GRAM{$gram}--;
			if($debug != 0) {warn "'$gram' ";}
			$correct2 += 1;
      }
      next if $i<2;
      $gram = $refSysOutput->[$i - 2]->[$factorIndex] ." ".$gram;
      if (defined($REF_GRAM{$gram}) && $REF_GRAM{$gram} > 0) {
			$REF_GRAM{$gram}--;
			if($debug != 0) {warn "'$gram' ";}
			$correct3 += 1;
      }
      next if $i<3;
      $gram = $refSysOutput->[$i - 3]->[$factorIndex] ." ".$gram;
      if (defined($REF_GRAM{$gram}) && $REF_GRAM{$gram} > 0) {
			$REF_GRAM{$gram}--;
			if($debug != 0) {warn "'$gram' ";}
			$correct4 += 1;
      }
	}
	if($debug != 0) {warn "\n";}
	my $total = $length_translation;
	$total1 = max(1, $total);
	$total2 = max(1, $total - 1);
	$total3 = max(1, $total - 2);
	$total4 = max(1, $total - 3);
	if($debug != 0)
	{
	warn "BLEU($factorIndex)\n";
	warn "truth: " . join(' ', map {join('|', @$_)} @{$refTruth}) . "\n";
	warn "sysop: " . join(' ', map {join('|', @$_)} @{$refSysOutput}) . "\n";
	warn "stats: $correct1 / $total1, $correct2 / $total2, $correct3 / $total3, $correct4 / $total4\n";
	sleep 8;
	}
	
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
