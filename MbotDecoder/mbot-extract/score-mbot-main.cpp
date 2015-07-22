#include <sstream>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <assert.h>
#include <cstring>
#include <set>
#include <algorithm>

#include "SafeGetline.h"
#include "tables-core.h"
#include "MbotAlignment.h"
#include "score.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"

using namespace std;

#define LINE_MAX_LENGTH 100000

LexicalTable lexTable;
bool lexFlag = true;
bool inverseFlag = false;
bool goodTuringFlag = false;
bool unpairedExtractFormatFlag = false;
#define COC_MAX 10
bool unalignedFlag = false;
int countOfCounts[COC_MAX+1];

Vocabulary vcbT;
Vocabulary vcbS;

vector<string> tokenize( const char [] );

void writeCountOfCounts( const string &fileNameCountOfCounts );
double computeLexicalTranslation( const MBOTPHRASE &, const MBOTPHRASE &, const MbotAlignment & );
double computeLexicalTranslationInverse( const MBOTPHRASE &, const MBOTPHRASE &, const MbotAlignment & );
void processPhrasePairs( vector< MbotAlignment > & , ostream &phraseTableFile, bool isSingleton);
void outputPhrasePair(const MbotAlignmentCollection &phrasePair, float, int, ostream &phraseTableFile, bool isSingleton);
//double computeUnalignedPenalty( const PHRASE &, const PHRASE &, const MbotAlignment & );
void printSourcePhrase(const MBOTPHRASE &, const MBOTPHRASE &, const MbotAlignment &, ostream &);
void printTargetPhrase(const MBOTPHRASE &, const MBOTPHRASE &, const MbotAlignment &, ostream &);
void printInvSourcePhrase(const MBOTPHRASE &, const MBOTPHRASE &, const MbotAlignment &, ostream &);
void printInvTargetPhrase(const MBOTPHRASE &, const MBOTPHRASE &, const MbotAlignment &, ostream &);


int main(int argc, char* argv[])
{
  cerr << "Score v2.0 written by Philipp Koehn\n"
       << "scoring methods for extracted rules\n";

  if (argc < 3) {
    cerr << "syntax: score-mbot extract lex phrase-table [--Inverse] [--GoodTuring] [--UnalignedPenalty] [--MinCountHierarchical count] [--Singleton]\n";
    exit(1);
  }
  string fileNameExtract = argv[1];
  string fileNameLex = argv[2];
  string fileNamePhraseTable = argv[3];
  string fileNameCountOfCounts;

  for(int i=4; i<argc; i++) {
    if (strcmp(argv[i],"inverse") == 0 || strcmp(argv[i],"--Inverse") == 0) {
      inverseFlag = true;
      cerr << "using inverse mode\n";
    } else if (strcmp(argv[i],"--GoodTuring") == 0) {
      goodTuringFlag = true;
      fileNameCountOfCounts = string(fileNamePhraseTable) + ".coc";
      cerr << "adjusting phrase translation probabilities with Good Turing discounting\n";
    } else if (strcmp(argv[i],"--NoLex") == 0) {
    	lexFlag = false;
    } else if (strcmp(argv[i],"--UnalignedPenalty") == 0) {
      unalignedFlag = true;
      cerr << "using unaligned word penalty\n";
      //} else if (strcmp(argv[i],"--MinCountHierarchical") == 0) {
      //minCountHierarchical = atof(argv[++i]);
      //cerr << "dropping all phrase pairs occurring less than " << minCountHierarchical << " times\n";
      //minCountHierarchical -= 0.00001; // account for rounding
    }
  }

  if (lexFlag)
    lexTable.load( fileNameLex );

  // compute count of counts for Good Turing discounting
  if (goodTuringFlag) {
    for(int i=1; i<=COC_MAX; i++) countOfCounts[i] = 0;
  }

    // get information on which side mbots
  string whichSide = "direct";
  if ( inverseFlag )
	whichSide = "inverse";

  // sorted phrase extraction file
  InputFileStream extractFile(fileNameExtract);

  if (extractFile.fail()) {
    cerr << "ERROR: could not open extract file " << fileNameExtract << endl;
    exit(1);
  }
  istream &extractFileP = extractFile;

  // output file: phrase translation table
  ostream *phraseTableFile;

  OutputFileStream *outputFile = new OutputFileStream();
  bool success = outputFile->Open(fileNamePhraseTable);
  if (!success) {
    cerr << "ERROR: could not open file phrase table file "
         << fileNamePhraseTable << endl;
    exit(1);
  }
  phraseTableFile = outputFile;
  

  // loop through all extracted phrase translations
  float lastCount = 0.0f;
  vector< MbotAlignment > phrasePairsWithSameF;
  bool isSingleton = true;
  int i=0;
  char line[LINE_MAX_LENGTH],lastLine[LINE_MAX_LENGTH];
  lastLine[0] = '\0';
  MbotAlignment *lastPhrasePair = NULL;

  while (true) {
	//cout << "Line: " << i << endl;
    if (extractFileP.eof()) break;
    if (++i % 100000 == 0) cerr << "." << flush;
    SAFE_GETLINE((extractFileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
    if (extractFileP.eof())	break;

    // identical to last line? just add count
    if (strcmp(line,lastLine) == 0) {
      //cout << "line equal lastline" << endl;
      lastPhrasePair->count += lastCount;
      continue;
    }
    strcpy( lastLine, line );

    // create new phrase pair
    //cout << "about to create phrasePair" << endl;
    MbotAlignment phrasePair;
    phrasePair.create( line, i, whichSide);
    lastCount = phrasePair.count;
    //cout << "created phrasePair" << endl;

    // only differs in count? just add count
    if (lastPhrasePair != NULL
        && lastPhrasePair->equals( phrasePair )) {
      lastPhrasePair->count += phrasePair.count;
      //cout << "only count differed" << endl;
      continue;
    }

    // if new source phrase, process last batch
    if (lastPhrasePair != NULL &&
        lastPhrasePair->GetSource() != phrasePair.GetSource()) {
      //cout << "(1) about to processPhrasePairs" << endl;
      processPhrasePairs( phrasePairsWithSameF, *phraseTableFile, isSingleton );
      //cout << "(1) processPhrasePairs done" << endl;

      phrasePairsWithSameF.clear();
      isSingleton = false;
      lastPhrasePair = NULL;
    } else {
      isSingleton = true;
    }

    // add phrase pairs to list, it's now the last one
    phrasePairsWithSameF.push_back( phrasePair );
    lastPhrasePair = &phrasePairsWithSameF.back();
  }
  //cout << "(2) about to processPhrasePairs" << endl;
  processPhrasePairs( phrasePairsWithSameF, *phraseTableFile, isSingleton );
  //cout << "(2) processPhrasePairs done" << endl;

  phraseTableFile->flush();
  if (phraseTableFile != &cout) {
    delete phraseTableFile;
  }

  // output count of count statistics
  if (goodTuringFlag) {
    writeCountOfCounts( fileNameCountOfCounts );
  }
}

void writeCountOfCounts( const string &fileNameCountOfCounts )
{
  // open file
  OutputFileStream countOfCountsFile;
  bool success = countOfCountsFile.Open(fileNameCountOfCounts.c_str());
  if (!success) {
    cerr << "ERROR: could not open count-of-counts file "
         << fileNameCountOfCounts << endl;
    return;
  }

  // Kneser-Ney needs the total number of phrase pairs
  //countOfCountsFile << totalDistinct << endl;

  // write out counts
  for(int i=1; i<=COC_MAX; i++) {
    countOfCountsFile << countOfCounts[ i ] << endl;
  }
  countOfCountsFile.Close();
}

void processPhrasePairs( vector< MbotAlignment > &phrasePair, ostream &phraseTableFile, bool isSingleton )
{
  if (phrasePair.size() == 0) return;

  // group phrase pairs based on alignments that matter
  // (i.e. that re-arrange non-terminals)
  PhrasePairGroup phrasePairGroup;

  float totalSource = 0;

  //cout << "phrasePair.size() = " << phrasePair.size() << endl;

  // loop through phrase pairs
  for(size_t i=0; i<phrasePair.size(); i++) {
    // add to total count
    MbotAlignment &currPhrasePair = phrasePair[i];

    totalSource += phrasePair[i].count;
    //cout << "totalSource " << totalSource << endl;
    // check for matches
    //cerr << "phrasePairGroup.size() = " << phrasePairGroup.size() << endl;

    MbotAlignmentCollection phraseAlignColl;
    phraseAlignColl.push_back(&currPhrasePair);
    //cout << "push_backed to phraseAlignColl" << endl;
    pair<PhrasePairGroup::iterator, bool> retInsert;
    retInsert = phrasePairGroup.insert(phraseAlignColl);
    //cout << "retInsert = " << retInsert.second << endl;
    if (!retInsert.second) {
      // already exist. Add to that collection instead
      MbotAlignmentCollection &existingColl = const_cast<MbotAlignmentCollection&>(*retInsert.first);
      existingColl.push_back(&currPhrasePair);
    }

  }

  // output the distinct phrase pairs, one at a time
  const PhrasePairGroup::SortedColl &sortedColl = phrasePairGroup.GetSortedColl();
  PhrasePairGroup::SortedColl::const_iterator iter;

  for(iter = sortedColl.begin(); iter != sortedColl.end(); ++iter) {
    const MbotAlignmentCollection &group = **iter;
    //cout << "try to outputPhrasePair" << endl;
    outputPhrasePair( group, totalSource, phrasePairGroup.GetSize(), phraseTableFile, isSingleton );
    //cout << "outputPhrasePair done" << endl;
  }

}

const MbotAlignment &findBestAlignment(const MbotAlignmentCollection &phrasePair )
{
  float bestAlignmentCount = -1;
  MbotAlignment* bestAlignment = NULL;

  for(size_t i=0; i<phrasePair.size(); i++) {
    size_t alignInd;
    if (inverseFlag) {
      // count backwards, so that alignments for ties will be the same for both normal & inverse scores
      alignInd = phrasePair.size() - i - 1;
    } else {
      alignInd = i;
    }

    if (phrasePair[alignInd]->count > bestAlignmentCount) {
      bestAlignmentCount = phrasePair[alignInd]->count;
      bestAlignment = phrasePair[alignInd];
    }
  }

  return *bestAlignment;
}

void outputPhrasePair(const MbotAlignmentCollection &phrasePair, float totalCount, int distinctCount, ostream &phraseTableFile, bool isSingleton )
{
  if (phrasePair.size() == 0) return;

  //cout << "try to find bestAlignment" << endl;
  const MbotAlignment &bestAlignment = findBestAlignment( phrasePair );
  //cout << "done bestAlignment" << endl;

  // compute count
  float count = 0;
  for(size_t i=0; i<phrasePair.size(); i++) {
    count += phrasePair[i]->count;
  }
  //cout << "computed count: " << count << endl;

  // collect count of count statistics
  if (goodTuringFlag) {
    //totalDistinct++;
    int countInt = count + 0.99999;
    if(countInt <= COC_MAX)
      countOfCounts[ countInt ]++;
  }

  // output phrases
  const MBOTPHRASE &phraseS = phrasePair[0]->GetSource();
  const MBOTPHRASE &phraseT = phrasePair[0]->GetTarget();

  //cout << "got phraseS and phraseT" << endl;
  // do not output if hierarchical and count below threshold
  /*if (hierarchicalFlag && count < minCountHierarchical) {
    for(size_t j=0; j<phraseS.size()-1; j++) {
      if (isNonTerminal(vcbS.getWord( phraseS[j] )))
        return;
    }
    }*/

  if (inverseFlag) {
	printInvTargetPhrase(phraseS, phraseT, bestAlignment, phraseTableFile);
	phraseTableFile << " ||| ";
	//cout << "printInvTargetPhrase done" << endl;

	printInvSourcePhrase(phraseS, phraseT, bestAlignment, phraseTableFile);
	phraseTableFile << " ||| ";
    //cout << "printInvSourcePhrase done" << endl;
  }

  if (!inverseFlag) {
	//cout << "try to printSourcePhrase" << endl;
    printSourcePhrase(phraseS, phraseT, bestAlignment, phraseTableFile);
    phraseTableFile << " ||| ";
    //cout << "printSourcePhrase done" << endl;

  	printTargetPhrase(phraseS, phraseT, bestAlignment, phraseTableFile);
  	phraseTableFile << " ||| ";
  	//cout << "printTargetPhrase done" << endl;
  }

  // lexical translation probability
  if (lexFlag) {
	double lexScore;
	if (inverseFlag) {
	  lexScore = computeLexicalTranslationInverse( phraseS, phraseT, bestAlignment );
	}
	else {
	  lexScore = computeLexicalTranslation( phraseS, phraseT, bestAlignment);
	}
	// TODO: check what maybeLogProb is good for and what we want
    //phraseTableFile << maybeLogProb(lexScore );
    phraseTableFile << lexScore;
  }

  // unaligned word penalty
  /*if (unalignedFlag) {
    double penalty = computeUnalignedPenalty( phraseS, phraseT, bestAlignment);
    phraseTableFile << " " << maybeLogProb(penalty );
    }*/

  /*if (singletonFeature) {
    phraseTableFile << " " << (isSingleton ? 1 : 0);
    }*/

  phraseTableFile << " ||| ";

  // alignment info for non-terminals
  if (! inverseFlag) {
	  //cout << "try to output ALIGNMENT" << endl;
      std::vector<std::string> alignment;
      for(size_t i = 0; i < phraseT.size(); i++) {
    	//cout << "mbotcomp " << i << endl;
    	for (size_t j=0; j<phraseT[i].size() -1; j++) {
    	  if (isNonTerminal(vcbT.getWord( phraseT[i][j] ))) {
    		//cout << "isNonTerminal" << endl;
    		if (bestAlignment.alignedToT[i][j].size() != 1) {
    		  cerr << "Error: unequal numbers of non-terminals. Make sure the text does not contain words in square brackets (like [xxx])." << endl;
    		  phraseTableFile.flush();
    		  assert(bestAlignment.alignedToT[i][j].size() == 1);
    		}
    		int sourcePos = *(bestAlignment.alignedToT[i][j].begin());
    		//cout << "source pos:j " << sourcePos << ":" << j << endl;
    		std::stringstream point;
    		point << sourcePos << "-" << j;
    		alignment.push_back(point.str());
    	  }
    	  // ALIGNMENTS FOR TERMINALS ARE NOT SUPPORTED IN MBOT-SYSTEM
    	  /*else {
    		//cout << "isWord" << endl;
      		//std::vector< std::vector< std::set<size_t> >::iterator vecIter;
    		std::set<size_t>:: iterator setIter;
    		std::set<size_t> compBestAlign = bestAlignment.alignedToT[i][j];
    		for(setIter = compBestAlign.begin(); setIter != compBestAlign.end(); setIter++) {
    		  int sourcePos = *setIter;
    		  //cout << "source pos:j " << sourcePos << ":" << j << endl;
    		  //phraseTableFile << sourcePos << "-" << j << " ";
    		  std::stringstream point;
    		  point << sourcePos << "-" << j;
    		  alignment.push_back(point.str());
    		}
    	  }*/
    	}
    	// now print alignments for mbot component, sorted by source index
    	sort(alignment.begin(), alignment.end());
    	//cout << "size alignment: " << alignment.size() << endl;
    	for (size_t k = 0; k < alignment.size(); ++k) {
    	  phraseTableFile << alignment[k] << " ";
    	}
    	if ( i < phraseT.size() -1 )
    	  phraseTableFile << "|| ";
    	alignment.clear();
      }
  }

  // counts
  phraseTableFile << " ||| " << totalCount << " " << count;
  
  phraseTableFile << endl;
}

/*double computeUnalignedPenalty( const PHRASE &phraseS, const PHRASE &phraseT, const MbotAlignment &alignment )
{
  // unaligned word counter
  double unaligned = 1.0;
  // only checking target words - source words are caught when computing inverse
  for(size_t ti=0; ti<alignment.alignedToT.size(); ti++) {
    const set< size_t > & srcIndices = alignment.alignedToT[ ti ];
    if (srcIndices.empty()) {
      unaligned *= 2.718;
    }
  }
  return unaligned;
  }*/

double computeLexicalTranslation( const MBOTPHRASE &phraseS, const MBOTPHRASE &phraseT, const MbotAlignment &alignment )
{
  // lexical translation probability
  double lexScore = 1.0;
  int null = vcbS.getWordID("NULL");
  // loop over mbot-components
  for ( size_t c=0; c<alignment.alignedToT.size(); c++ ) {
	// all target words have to be explained
	for(size_t ti=0; ti<alignment.alignedToT[c].size(); ti++) {
	  const set< size_t > & srcIndices = alignment.alignedToT[ c ][ ti ];
	    if (srcIndices.empty()) {
		  // explain unaligned word by NULL
		  lexScore *= lexTable.permissiveLookup( null, phraseT[ c ][ ti ] );
		} else {
		  // go through all the aligned words to compute average
		  double thisWordScore = 0;
		  for (set< size_t >::const_iterator p(srcIndices.begin()); p != srcIndices.end(); ++p) {
		    thisWordScore += lexTable.permissiveLookup( phraseS[ 0 ][ *p ], phraseT[ c ][ ti ] );
		  }
		  lexScore *= thisWordScore / (double)srcIndices.size();
		}
	 }
  }
  return lexScore;
}

double computeLexicalTranslationInverse( const MBOTPHRASE &phraseS, const MBOTPHRASE &phraseT, const MbotAlignment &alignment )
{
  // lexical translation probability
  double lexScore = 1.0;
  int null = vcbS.getWordID("NULL");

  /*cout << "IN COMPUTELEXICALTRANSLATIONINVERSE" << endl;
  for ( size_t i=0; i< alignment.alignedToT.size(); i++ ) {
	  //cout << "mbot component " << i;
	  for (size_t j=0; j<alignment.alignedToT[i].size(); j++ ) {
		  //cout << " target pos " << j;
		  set< size_t > alignPoints = alignment.alignedToT[i][j];
		  for (set< size_t >::const_iterator p(alignPoints.begin()); p != alignPoints.end(); ++p) {
			  cout << "mbot component " << i;
			  cout << " target pos " << j;
			  cout << " aligned to " << *p << endl;
		  }
	  }
  }*/
  // loop over target words
  for ( size_t ti=0; ti<phraseT[0].size()-1; ti++ ) {
	//cout << "phrase: " << ti << endl;
	// get alignments for target word in (multiple) mbot components
	set< pair< size_t, size_t > > srcIndices;
	//for (size_t c=0; c<numComp; c++) {
	for ( size_t i=0; i< alignment.alignedToT.size(); i++ ) {
	  //cout << "numComp: " << i << endl;
	  //if ( !alignment.alignedToT[ c ][ ti ].empty() ) {
	  //size_t alignSize = alignment.alignedToT[c][ti].size();
	  //cout << "alignment set has size " << alignSize << endl;
	  if ( ti > alignment.alignedToT[i].size() - 1 )
		continue;
	  set< size_t > currSet = alignment.alignedToT[ i ][ ti ];
	  for (set< size_t >::const_iterator p(currSet.begin()); p != currSet.end(); ++p) {
		  //cout << "make_pair: " << i << ":" << *p << endl;
		  srcIndices.insert( make_pair (i,*p) );
		//}
	  }
	}
	if (srcIndices.empty()) {
	  // explain unaligned word by NULL
	  //cout << "srcIndices are empty" << endl;
	  lexScore *= lexTable.permissiveLookup( null, phraseT[ 0 ][ ti ] );
	} else {
	  // go through all the aligned words to compute average
	  double thisWordScore = 0;
	  for (set< pair< size_t, size_t > >::const_iterator p(srcIndices.begin()); p != srcIndices.end(); ++p) {
		pair< size_t, size_t > thisPair = *p;
		//cout << "current srcIndex: " << thisPair.first << ":" << thisPair.second << endl;
		thisWordScore += lexTable.permissiveLookup( phraseS[ thisPair.first ][ thisPair.second ], phraseT[ 0 ][ ti ] );
		//cout << "thisWordScore: " << thisWordScore << endl;
	  }
	  lexScore *= thisWordScore / (double)srcIndices.size();
	}
  }
  return lexScore;
}

void LexicalTable::load( const string &fileName )
{
  cerr << "Loading lexical translation table from " << fileName;
  ifstream inFile;
  inFile.open(fileName.c_str());
  if (inFile.fail()) {
    cerr << " - ERROR: could not open file\n";
    exit(1);
  }
  istream *inFileP = &inFile;

  char line[LINE_MAX_LENGTH];

  int i=0;
  while(true) {
    i++;
    if (i%100000 == 0) cerr << "." << flush;
    SAFE_GETLINE((*inFileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
    if (inFileP->eof()) break;

    vector<string> token = tokenize( line );
    if (token.size() != 3) {
      cerr << "line " << i << " in " << fileName
           << " has wrong number of tokens, skipping:\n"
           << token.size() << " " << token[0] << " " << line << endl;
      continue;
    }

    double prob = atof( token[2].c_str() );
    WORD_ID wordT = vcbT.storeIfNew( token[0] );
    WORD_ID wordS = vcbS.storeIfNew( token[1] );
    ltable[ wordS ][ wordT ] = prob;
  }
  cerr << endl;
}

void printSourcePhrase(const MBOTPHRASE &phraseS, const MBOTPHRASE &phraseT,
                       const MbotAlignment &bestAlignment, ostream &out)
{
  // output source symbols, except root, in rule table format
  for (std::size_t i = 0; i < phraseS[0].size()-1; ++i) {
	bool found = false;
    const std::string &word = vcbS.getWord(phraseS[0][i]);
    if ( !unpairedExtractFormatFlag || !isNonTerminal(word) ) {
      out << word << " ";
      continue;
    }
    // get corresponding target non-terminal and output pair
    // since alignedToS is <mbotcomp <sourcepos <(target pos)>>
    // loop over mbot components first
    for ( std::size_t j=0; j<bestAlignment.alignedToS.size(); j++ ) {
      //cout << "pSP: mbotcomp " << j << endl;
      //cout << "pSP: source pos " << i << endl;
      if ( i > bestAlignment.alignedToS[j].size() -1 )
    	continue;
      std::set<std::size_t> alignmentPoint = bestAlignment.alignedToS[j][i];
      // it's possible that an alignment point is empty, assert later
      //assert(!alignmentPoint.empty());
      if ( alignmentPoint.empty() )
    	continue;
      found = true;
      int k = *(alignmentPoint.begin());
      //cout << "pSP: k " << k << endl;
       if (inverseFlag) {
     	  out << vcbT.getWord(phraseT[j][k]) << word << " ";
       } else {
     	  out << word << vcbT.getWord(phraseT[j][k]) << " ";
       }
    }
    assert(found);
  }
  // output source root symbol
  out << vcbS.getWord(phraseS[0].back());
}

void printInvSourcePhrase(const MBOTPHRASE &phraseS, const MBOTPHRASE &phraseT,
                       const MbotAlignment &bestAlignment, ostream &out)
{
  // output inverse source symbols, except root, in rule table format
  // looping over mbot components
  for (std::size_t i = 0; i < phraseS.size(); ++i) {
	std::vector<WORD_ID> compS = phraseS[i];
	//cout << "pISP: mbot " << i << endl;

	// looping over source pos for each mbot component
	for (std::size_t j = 0; j < compS.size()-1; ++j) {
	  //cout << "pISP: inv source pos " << j << endl;
	  const std::string &word = vcbS.getWord(compS[j]);
	  if ( !unpairedExtractFormatFlag || !isNonTerminal(word)) {
		//cout << "word " << word << endl;
		out << word << " ";
		continue;
	  }
	  //cout << "nonterm " << word << endl;
	  // get corresponding target non-terminal and output pair
	  std::set<std::size_t> alignmentPoint = bestAlignment.alignedToS[i][j];
	  assert(!alignmentPoint.empty());
	  int l = *(alignmentPoint.begin());
	  //cout << "pISP: aligned target pos " << l << endl;
	  out << vcbT.getWord(phraseT[0][l]) << word << " ";

	}
	out << vcbS.getWord(phraseS[i].back());
	if ( i<phraseS.size()-1 )
	  out << " || ";
  }
}

void printTargetPhrase(const MBOTPHRASE &phraseS, const MBOTPHRASE &phraseT,
                       const MbotAlignment &bestAlignment, ostream &out)
{
  // output target symbols, except root, in rule table format
  // looping over the mbot components
  for (std::size_t i = 0; i < phraseT.size(); ++i) {
	std::vector<WORD_ID> compT = phraseT[i];

	// looping over target pos for each mbot component
	for (std::size_t j = 0; j < compT.size()-1; ++j) {
	  const std::string &word = vcbT.getWord(compT[j]);
	  if ( !unpairedExtractFormatFlag || !isNonTerminal(word) ) {
		out << word << " ";
		continue;
	  }
	  // get corresponding source non-terminal and output pair
	  std::set<std::size_t> alignmentPoint = bestAlignment.alignedToT[i][j];
	  assert(!alignmentPoint.empty());
	  int l = *(alignmentPoint.begin());
	  if (inverseFlag) {
	      out << vcbS.getWord(phraseS[0][l]) << word << " ";
	  } else {
	      out << word << vcbS.getWord(phraseS[0][l]) << " ";
	  }
	}
	out << vcbT.getWord(phraseT[i].back());
	if ( i<phraseT.size()-1 )
	  out << " || ";
  }
}

void printInvTargetPhrase(const MBOTPHRASE &phraseS, const MBOTPHRASE &phraseT,
                       const MbotAlignment &bestAlignment, ostream &out)
{
  // output inverse target symbols, except root, in rule table format
  for (std::size_t i = 0; i < phraseT[0].size()-1; ++i) {
	bool found = false;
    const std::string &word = vcbT.getWord(phraseT[0][i]);
    if ( !unpairedExtractFormatFlag || !isNonTerminal(word) ) {
      out << word << " ";
      continue;
    }
    // get corresponding target non-terminal and output pair
    // since alignedToS is <mbotcomp <sourcepos <(target pos)>>
    // loop over mbot components first
    for ( std::size_t j=0; j<bestAlignment.alignedToT.size(); j++ ) {
      //cout << "pInvTP: mbotcomp " << j << endl;
      //cout << "pInvTP: target pos " << i << endl;
      if ( i > bestAlignment.alignedToT[j].size() -1 )
    	continue;
      std::set<std::size_t> alignmentPoint = bestAlignment.alignedToT[j][i];
      // it's possible that an alignment point is empty, assert later
      //assert(!alignmentPoint.empty());
      if ( alignmentPoint.empty() )
    	continue;
      found = true;
      int k = *(alignmentPoint.begin());
      //cout << "pInvTP: k " << k << endl;
      out << vcbS.getWord(phraseS[j][k]) << word << " ";
    }
    assert(found);
  }
  // output inverse target root symbol
  out << vcbT.getWord(phraseT[0].back());
}



std::pair<PhrasePairGroup::Coll::iterator,bool> PhrasePairGroup::insert ( const MbotAlignmentCollection& obj )
{
  std::pair<iterator,bool> ret = m_coll.insert(obj);

  if (ret.second) {
	//cout << "in score-main: insert" << endl;
    // obj inserted. Also add to sorted vector
    const MbotAlignmentCollection &insertedObj = *ret.first;
    m_sortedColl.push_back(&insertedObj);
	//cout << "in score-main: insert DONE" << endl;
  }

  return ret;
}
