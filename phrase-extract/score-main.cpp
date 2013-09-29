/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2009 University of Edinburgh

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

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
#include "ScoreFeature.h"
#include "tables-core.h"
#include "domain.h"
#include "PhraseAlignment.h"
#include "score.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "InternalStructFeature.h"

using namespace std;
using namespace MosesTraining;

#define LINE_MAX_LENGTH 100000

namespace MosesTraining
{
LexicalTable lexTable;
bool inverseFlag = false;
bool hierarchicalFlag = false;
bool pcfgFlag = false;
bool treeFragmentsFlag = false;
bool unpairedExtractFormatFlag = false;
bool conditionOnTargetLhsFlag = false;
bool wordAlignmentFlag = true;
bool goodTuringFlag = false;
bool kneserNeyFlag = false;
bool logProbFlag = false;
int negLogProb = 1;
#define COC_MAX 10
bool lexFlag = true;
bool unalignedFlag = false;
bool unalignedFWFlag = false;
bool singletonFeature = false;
bool crossedNonTerm = false;
int countOfCounts[COC_MAX+1];
int totalDistinct = 0;
float minCountHierarchical = 0;

Vocabulary vcbT;
Vocabulary vcbS;

} // namespace

vector<string> tokenize( const char [] );

void writeCountOfCounts( const string &fileNameCountOfCounts );
void processPhrasePairs( vector< PhraseAlignment > & , ostream &phraseTableFile, bool isSingleton, const ScoreFeatureManager& featureManager, const MaybeLog& maybeLog);
const PhraseAlignment &findBestAlignment(const PhraseAlignmentCollection &phrasePair );
const std::string &findBestTreeFragment(const PhraseAlignmentCollection &phrasePair );
void outputPhrasePair(const PhraseAlignmentCollection &phrasePair, float, int, ostream &phraseTableFile, bool isSingleton, const ScoreFeatureManager& featureManager, const MaybeLog& maybeLog );
double computeLexicalTranslation( const PHRASE &, const PHRASE &, const PhraseAlignment & );
double computeUnalignedPenalty( const PHRASE &, const PHRASE &, const PhraseAlignment & );
set<string> functionWordList;
void loadFunctionWords( const string &fileNameFunctionWords );
double computeUnalignedFWPenalty( const PHRASE &, const PHRASE &, const PhraseAlignment & );
void printSourcePhrase(const PHRASE &, const PHRASE &, const PhraseAlignment &, ostream &);
void printTargetPhrase(const PHRASE &, const PHRASE &, const PhraseAlignment &, ostream &);

int main(int argc, char* argv[])
{
  cerr << "Score v2.0 written by Philipp Koehn\n"
       << "scoring methods for extracted rules\n";

  ScoreFeatureManager featureManager;
  if (argc < 4) {
    cerr << "syntax: score extract lex phrase-table [--Inverse] [--Hierarchical] [--LogProb] [--NegLogProb] [--NoLex] [--GoodTuring] [--KneserNey] [--NoWordAlignment] [--UnalignedPenalty] [--UnalignedFunctionWordPenalty function-word-file] [--MinCountHierarchical count] [--PCFG] [--TreeFragments] [--UnpairedExtractFormat] [--ConditionOnTargetLHS] [--Singleton] [--CrossedNonTerm] \n";
    cerr << featureManager.usage() << endl;
    exit(1);
  }
  string fileNameExtract = argv[1];
  string fileNameLex = argv[2];
  string fileNamePhraseTable = argv[3];
  string fileNameCountOfCounts;
  char* fileNameFunctionWords = NULL;
  vector<string> featureArgs; //all unknown args passed to feature manager

  for(int i=4; i<argc; i++) {
    if (strcmp(argv[i],"inverse") == 0 || strcmp(argv[i],"--Inverse") == 0) {
      inverseFlag = true;
      cerr << "using inverse mode\n";
    } else if (strcmp(argv[i],"--Hierarchical") == 0) {
      hierarchicalFlag = true;
      cerr << "processing hierarchical rules\n";
    } else if (strcmp(argv[i],"--PCFG") == 0) {
      pcfgFlag = true;
      cerr << "including PCFG scores\n";
    } else if (strcmp(argv[i],"--TreeFragments") == 0) {
      treeFragmentsFlag = true;
      cerr << "including tree fragments from syntactic parse\n";
    } else if (strcmp(argv[i],"--UnpairedExtractFormat") == 0) {
      unpairedExtractFormatFlag = true;
      cerr << "processing unpaired extract format\n";
    } else if (strcmp(argv[i],"--ConditionOnTargetLHS") == 0) {
      conditionOnTargetLhsFlag = true;
      cerr << "processing unpaired extract format\n";
    } else if (strcmp(argv[i],"--NoWordAlignment") == 0) {
      wordAlignmentFlag = false;
      cerr << "omitting word alignment" << endl;
    } else if (strcmp(argv[i],"--NoLex") == 0) {
      lexFlag = false;
      cerr << "not computing lexical translation score\n";
    } else if (strcmp(argv[i],"--GoodTuring") == 0) {
      goodTuringFlag = true;
      fileNameCountOfCounts = string(fileNamePhraseTable) + ".coc";
      cerr << "adjusting phrase translation probabilities with Good Turing discounting\n";
    } else if (strcmp(argv[i],"--KneserNey") == 0) {
      kneserNeyFlag = true;
      fileNameCountOfCounts = string(fileNamePhraseTable) + ".coc";
      cerr << "adjusting phrase translation probabilities with Kneser Ney discounting\n";
    } else if (strcmp(argv[i],"--UnalignedPenalty") == 0) {
      unalignedFlag = true;
      cerr << "using unaligned word penalty\n";
    } else if (strcmp(argv[i],"--UnalignedFunctionWordPenalty") == 0) {
      unalignedFWFlag = true;
      if (i+1==argc) {
        cerr << "ERROR: specify count of count files for Kneser Ney discounting!\n";
        exit(1);
      }
      fileNameFunctionWords = argv[++i];
      cerr << "using unaligned function word penalty with function words from " << fileNameFunctionWords << endl;
    }  else if (strcmp(argv[i],"--LogProb") == 0) {
      logProbFlag = true;
      cerr << "using log-probabilities\n";
    } else if (strcmp(argv[i],"--NegLogProb") == 0) {
      logProbFlag = true;
      negLogProb = -1;
      cerr << "using negative log-probabilities\n";
    } else if (strcmp(argv[i],"--MinCountHierarchical") == 0) {
      minCountHierarchical = atof(argv[++i]);
      cerr << "dropping all phrase pairs occurring less than " << minCountHierarchical << " times\n";
      minCountHierarchical -= 0.00001; // account for rounding
    } else if (strcmp(argv[i],"--Singleton") == 0) {
      singletonFeature = true;
      cerr << "binary singleton feature\n";
    } else if (strcmp(argv[i],"--CrossedNonTerm") == 0) {
      crossedNonTerm = true;
      cerr << "crossed non-term reordering feature\n";
    } else {
      featureArgs.push_back(argv[i]);
      ++i;
      for (; i < argc &&  strncmp(argv[i], "--", 2); ++i) {
        featureArgs.push_back(argv[i]);
      }
      if (i != argc) --i; //roll back, since we found another -- argument
    }
  }

  MaybeLog maybeLogProb(logProbFlag, negLogProb);

  //configure extra features
  if (!inverseFlag) featureManager.configure(featureArgs);

  // lexical translation table
  if (lexFlag)
    lexTable.load( fileNameLex );

  // function word list
  if (unalignedFWFlag)
    loadFunctionWords( fileNameFunctionWords );

  // compute count of counts for Good Turing discounting
  if (goodTuringFlag || kneserNeyFlag) {
    for(int i=1; i<=COC_MAX; i++) countOfCounts[i] = 0;
  }

  // sorted phrase extraction file
  Moses::InputFileStream extractFile(fileNameExtract);

  if (extractFile.fail()) {
    cerr << "ERROR: could not open extract file " << fileNameExtract << endl;
    exit(1);
  }
  istream &extractFileP = extractFile;

  // output file: phrase translation table
  ostream *phraseTableFile;

  if (fileNamePhraseTable == "-") {
    phraseTableFile = &cout;
  } else {
    Moses::OutputFileStream *outputFile = new Moses::OutputFileStream();
    bool success = outputFile->Open(fileNamePhraseTable);
    if (!success) {
      cerr << "ERROR: could not open file phrase table file "
           << fileNamePhraseTable << endl;
      exit(1);
    }
    phraseTableFile = outputFile;
  }

  // loop through all extracted phrase translations
  float lastCount = 0.0f;
  float lastPcfgSum = 0.0f;
  vector< PhraseAlignment > phrasePairsWithSameF;
  bool isSingleton = true;
  int i=0;
  char line[LINE_MAX_LENGTH],lastLine[LINE_MAX_LENGTH];
  lastLine[0] = '\0';
  PhraseAlignment *lastPhrasePair = NULL;
  while(true) {
    if (extractFileP.eof()) break;
    if (++i % 100000 == 0) cerr << "." << flush;
    SAFE_GETLINE((extractFileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
    if (extractFileP.eof())	break;

    // identical to last line? just add count
    if (strcmp(line,lastLine) == 0) {
      lastPhrasePair->count += lastCount;
      lastPhrasePair->pcfgSum += lastPcfgSum;
      continue;
    }
    strcpy( lastLine, line );

    // create new phrase pair
    PhraseAlignment phrasePair;
    phrasePair.create( line, i, featureManager.includeSentenceId());
    lastCount = phrasePair.count;
    lastPcfgSum = phrasePair.pcfgSum;

    // only differs in count? just add count
    if (lastPhrasePair != NULL
        && lastPhrasePair->equals( phrasePair )
        && featureManager.equals(*lastPhrasePair, phrasePair)) {
      lastPhrasePair->count += phrasePair.count;
      lastPhrasePair->pcfgSum += phrasePair.pcfgSum;
      continue;
    }

    // if new source phrase, process last batch
    if (lastPhrasePair != NULL &&
        lastPhrasePair->GetSource() != phrasePair.GetSource()) {
      processPhrasePairs( phrasePairsWithSameF, *phraseTableFile, isSingleton, featureManager, maybeLogProb );

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
  processPhrasePairs( phrasePairsWithSameF, *phraseTableFile, isSingleton, featureManager, maybeLogProb );

  phraseTableFile->flush();
  if (phraseTableFile != &cout) {
    delete phraseTableFile;
  }

  // output count of count statistics
  if (goodTuringFlag || kneserNeyFlag) {
    writeCountOfCounts( fileNameCountOfCounts );
  }
}

void writeCountOfCounts( const string &fileNameCountOfCounts )
{
  // open file
  Moses::OutputFileStream countOfCountsFile;
  bool success = countOfCountsFile.Open(fileNameCountOfCounts.c_str());
  if (!success) {
    cerr << "ERROR: could not open count-of-counts file "
         << fileNameCountOfCounts << endl;
    return;
  }

  // Kneser-Ney needs the total number of phrase pairs
  countOfCountsFile << totalDistinct << endl;

  // write out counts
  for(int i=1; i<=COC_MAX; i++) {
    countOfCountsFile << countOfCounts[ i ] << endl;
  }
  countOfCountsFile.Close();
}

void processPhrasePairs( vector< PhraseAlignment > &phrasePair, ostream &phraseTableFile, bool isSingleton, const ScoreFeatureManager& featureManager, const MaybeLog& maybeLogProb )
{
  if (phrasePair.size() == 0) return;

  // group phrase pairs based on alignments that matter
  // (i.e. that re-arrange non-terminals)
  PhrasePairGroup phrasePairGroup;

  float totalSource = 0;

  //cerr << "phrasePair.size() = " << phrasePair.size() << endl;

  // loop through phrase pairs
  for(size_t i=0; i<phrasePair.size(); i++) {
    // add to total count
    PhraseAlignment &currPhrasePair = phrasePair[i];

    totalSource += phrasePair[i].count;

    // check for matches
    //cerr << "phrasePairGroup.size() = " << phrasePairGroup.size() << endl;

    PhraseAlignmentCollection phraseAlignColl;
    phraseAlignColl.push_back(&currPhrasePair);
    pair<PhrasePairGroup::iterator, bool> retInsert;
    retInsert = phrasePairGroup.insert(phraseAlignColl);
    if (!retInsert.second) {
      // already exist. Add to that collection instead
      PhraseAlignmentCollection &existingColl = const_cast<PhraseAlignmentCollection&>(*retInsert.first);
      existingColl.push_back(&currPhrasePair);
    }

  }

  // output the distinct phrase pairs, one at a time
  const PhrasePairGroup::SortedColl &sortedColl = phrasePairGroup.GetSortedColl();
  PhrasePairGroup::SortedColl::const_iterator iter;

  for(iter = sortedColl.begin(); iter != sortedColl.end(); ++iter) {
    const PhraseAlignmentCollection &group = **iter;
    outputPhrasePair( group, totalSource, phrasePairGroup.GetSize(), phraseTableFile, isSingleton, featureManager, maybeLogProb );
  }

}

const PhraseAlignment &findBestAlignment(const PhraseAlignmentCollection &phrasePair )
{
  float bestAlignmentCount = -1;
  PhraseAlignment* bestAlignment = NULL;

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

const std::string &findBestTreeFragment(const PhraseAlignmentCollection &phrasePair )
{
  float bestTreeFragmentCount = -1;
  PhraseAlignment *bestTreeFragment = NULL;

  for(size_t i=0; i<phrasePair.size(); i++) {
    size_t treeFragmentInd;
    if (inverseFlag) {
      // count backwards, so that alignments for ties will be the same for both normal & inverse scores
      treeFragmentInd = phrasePair.size() - i - 1;
    } else {
      treeFragmentInd = i;
    }

    if (phrasePair[treeFragmentInd]->count > bestTreeFragmentCount) {
      bestTreeFragmentCount = phrasePair[treeFragmentInd]->count;
      bestTreeFragment = phrasePair[treeFragmentInd];
    }
  }

  return bestTreeFragment->treeFragment;
}

bool calcCrossedNonTerm(size_t sourcePos, size_t targetPos, const std::vector< std::set<size_t> > &alignedToS)
{
  for (size_t currSource = 0; currSource < alignedToS.size(); ++currSource) {
    if (currSource == sourcePos) {
      // skip
    } else {
      const std::set<size_t> &targetSet = alignedToS[currSource];
      std::set<size_t>::const_iterator iter;
      for (iter = targetSet.begin(); iter != targetSet.end(); ++iter) {
        size_t currTarget = *iter;

        if ((currSource < sourcePos && currTarget > targetPos)
            || (currSource > sourcePos && currTarget < targetPos)
           ) {
          return true;
        }
      }

    }
  }

  return false;
}

int calcCrossedNonTerm(const PHRASE &phraseS, const PhraseAlignment &bestAlignment)
{
  const std::vector< std::set<size_t> > &alignedToS = bestAlignment.alignedToS;

  for (size_t sourcePos = 0; sourcePos < alignedToS.size(); ++sourcePos) {
    const std::set<size_t> &targetSet = alignedToS[sourcePos];

    WORD_ID wordId = phraseS[sourcePos];
    const WORD &word = vcbS.getWord(wordId);
    bool isNonTerm = isNonTerminal(word);

    if (isNonTerm) {
      assert(targetSet.size() == 1);
      size_t targetPos = *targetSet.begin();
      bool ret = calcCrossedNonTerm(sourcePos, targetPos, alignedToS);
      if (ret)
        return 1;
    }
  }

  return 0;
}

void outputPhrasePair(const PhraseAlignmentCollection &phrasePair, float totalCount, int distinctCount, ostream &phraseTableFile, bool isSingleton, const ScoreFeatureManager& featureManager,
                      const MaybeLog& maybeLogProb )
{
  if (phrasePair.size() == 0) return;

  const PhraseAlignment &bestAlignment = findBestAlignment( phrasePair );

  // compute count
  float count = 0;
  for(size_t i=0; i<phrasePair.size(); i++) {
    count += phrasePair[i]->count;
  }

  map< string, float > domainCount;

  // collect count of count statistics
  if (goodTuringFlag || kneserNeyFlag) {
    totalDistinct++;
    int countInt = count + 0.99999;
    if(countInt <= COC_MAX)
      countOfCounts[ countInt ]++;
  }

  // compute PCFG score
  float pcfgScore = 0;
  if (pcfgFlag && !inverseFlag) {
    float pcfgSum = 0;
    for(size_t i=0; i<phrasePair.size(); ++i) {
      pcfgSum += phrasePair[i]->pcfgSum;
    }
    pcfgScore = pcfgSum / count;
  }

  // output phrases
  const PHRASE &phraseS = phrasePair[0]->GetSource();
  const PHRASE &phraseT = phrasePair[0]->GetTarget();

  // do not output if hierarchical and count below threshold
  if (hierarchicalFlag && count < minCountHierarchical) {
    for(size_t j=0; j<phraseS.size()-1; j++) {
      if (isNonTerminal(vcbS.getWord( phraseS[j] )))
        return;
    }
  }

  // source phrase (unless inverse)
  if (! inverseFlag) {
    printSourcePhrase(phraseS, phraseT, bestAlignment, phraseTableFile);
    phraseTableFile << " ||| ";
  }

  // target phrase
  printTargetPhrase(phraseS, phraseT, bestAlignment, phraseTableFile);
  phraseTableFile << " ||| ";

  // source phrase (if inverse)
  if (inverseFlag) {
    printSourcePhrase(phraseS, phraseT, bestAlignment, phraseTableFile);
    phraseTableFile << " ||| ";
  }

  // lexical translation probability
  if (lexFlag) {
    double lexScore = computeLexicalTranslation( phraseS, phraseT, bestAlignment);
    phraseTableFile << maybeLogProb(lexScore );
  }

  // unaligned word penalty
  if (unalignedFlag) {
    double penalty = computeUnalignedPenalty( phraseS, phraseT, bestAlignment);
    phraseTableFile << " " << maybeLogProb(penalty );
  }

  // unaligned function word penalty
  if (unalignedFWFlag) {
    double penalty = computeUnalignedFWPenalty( phraseS, phraseT, bestAlignment);
    phraseTableFile << " " << maybeLogProb(penalty );
  }

  if (singletonFeature) {
    phraseTableFile << " " << (isSingleton ? 1 : 0);
  }

  if (crossedNonTerm && !inverseFlag) {
    phraseTableFile << " " << calcCrossedNonTerm(phraseS, bestAlignment);
  }

  // target-side PCFG score
  if (pcfgFlag && !inverseFlag) {
    phraseTableFile << " " << maybeLogProb(pcfgScore );
  }

  // extra features
  ScoreFeatureContext context(phrasePair, count, maybeLogProb);
  vector<float> extraDense;
  map<string,float> extraSparse;
  featureManager.addFeatures(context, extraDense, extraSparse);
  for (size_t i = 0; i < extraDense.size(); ++i) {
    phraseTableFile << " " << extraDense[i];
  }

  for (map<string,float>::const_iterator i = extraSparse.begin();
       i != extraSparse.end(); ++i) {
    phraseTableFile << " " << i->first << " " << i->second;
  }

  phraseTableFile << " ||| ";

  // alignment info for non-terminals
  if (! inverseFlag) {
    if (hierarchicalFlag) {
      // always output alignment if hiero style, but only for non-terms
      // (eh: output all alignments, needed for some feature functions)
      assert(phraseT.size() == bestAlignment.alignedToT.size() + 1);
      std::vector<std::string> alignment;
      for(size_t j = 0; j < phraseT.size() - 1; j++) {
        if (isNonTerminal(vcbT.getWord( phraseT[j] ))) {
          if (bestAlignment.alignedToT[ j ].size() != 1) {
            cerr << "Error: unequal numbers of non-terminals. Make sure the text does not contain words in square brackets (like [xxx])." << endl;
            phraseTableFile.flush();
            assert(bestAlignment.alignedToT[ j ].size() == 1);
          }
          int sourcePos = *(bestAlignment.alignedToT[ j ].begin());
          //phraseTableFile << sourcePos << "-" << j << " ";
          std::stringstream point;
          point << sourcePos << "-" << j;
          alignment.push_back(point.str());
        } else {
          set<size_t>::iterator setIter;
          for(setIter = (bestAlignment.alignedToT[j]).begin(); setIter != (bestAlignment.alignedToT[j]).end(); setIter++) {
            int sourcePos = *setIter;
            //phraseTableFile << sourcePos << "-" << j << " ";
            std::stringstream point;
            point << sourcePos << "-" << j;
            alignment.push_back(point.str());
          }
        }
      }
      // now print all alignments, sorted by source index
      sort(alignment.begin(), alignment.end());
      for (size_t i = 0; i < alignment.size(); ++i) {
        phraseTableFile << alignment[i] << " ";
      }
    } else if (wordAlignmentFlag) {
      // alignment info in pb model
      for(size_t j=0; j<bestAlignment.alignedToT.size(); j++) {
        const set< size_t > &aligned = bestAlignment.alignedToT[j];
        for (set< size_t >::const_iterator p(aligned.begin()); p != aligned.end(); ++p) {
          phraseTableFile << *p << "-" << j << " ";
        }
      }
    }
  }


  // counts

  phraseTableFile << " ||| " << totalCount << " " << count;
  if (kneserNeyFlag)
    phraseTableFile << " " << distinctCount;

  // tree fragments
  if (treeFragmentsFlag && !inverseFlag) {
    const std::string &bestTreeFragment = findBestTreeFragment( phrasePair );
    if ( !bestTreeFragment.empty() )
      phraseTableFile << " ||| {{Tree " << bestTreeFragment << "}}";
  }


  phraseTableFile << endl;
}

double computeUnalignedPenalty( const PHRASE &phraseS, const PHRASE &phraseT, const PhraseAlignment &alignment )
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
}

double computeUnalignedFWPenalty( const PHRASE &phraseS, const PHRASE &phraseT, const PhraseAlignment &alignment )
{
  // unaligned word counter
  double unaligned = 1.0;
  // only checking target words - source words are caught when computing inverse
  for(size_t ti=0; ti<alignment.alignedToT.size(); ti++) {
    const set< size_t > & srcIndices = alignment.alignedToT[ ti ];
    if (srcIndices.empty() && functionWordList.find( vcbT.getWord( phraseT[ ti ] ) ) != functionWordList.end()) {
      unaligned *= 2.718;
    }
  }
  return unaligned;
}

void loadFunctionWords( const string &fileName )
{
  cerr << "Loading function word list from " << fileName;
  ifstream inFile;
  inFile.open(fileName.c_str());
  if (inFile.fail()) {
    cerr << " - ERROR: could not open file\n";
    exit(1);
  }
  istream *inFileP = &inFile;

  char line[LINE_MAX_LENGTH];
  while(true) {
    SAFE_GETLINE((*inFileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
    if (inFileP->eof()) break;
    vector<string> token = tokenize( line );
    if (token.size() > 0)
      functionWordList.insert( token[0] );
  }
  inFile.close();

  cerr << " - read " << functionWordList.size() << " function words\n";
  inFile.close();
}

double computeLexicalTranslation( const PHRASE &phraseS, const PHRASE &phraseT, const PhraseAlignment &alignment )
{
  // lexical translation probability
  double lexScore = 1.0;
  int null = vcbS.getWordID("NULL");
  // all target words have to be explained
  for(size_t ti=0; ti<alignment.alignedToT.size(); ti++) {
    const set< size_t > & srcIndices = alignment.alignedToT[ ti ];
    if (srcIndices.empty()) {
      // explain unaligned word by NULL
      lexScore *= lexTable.permissiveLookup( null, phraseT[ ti ] );
    } else {
      // go through all the aligned words to compute average
      double thisWordScore = 0;
      for (set< size_t >::const_iterator p(srcIndices.begin()); p != srcIndices.end(); ++p) {
        thisWordScore += lexTable.permissiveLookup( phraseS[ *p ], phraseT[ ti ] );
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

void printSourcePhrase(const PHRASE &phraseS, const PHRASE &phraseT,
                       const PhraseAlignment &bestAlignment, ostream &out)
{
  // output source symbols, except root, in rule table format
  for (std::size_t i = 0; i < phraseS.size()-1; ++i) {
    const std::string &word = vcbS.getWord(phraseS[i]);
    if (!unpairedExtractFormatFlag || !isNonTerminal(word)) {
      out << word << " ";
      continue;
    }
    // get corresponding target non-terminal and output pair
    std::set<std::size_t> alignmentPoints = bestAlignment.alignedToS[i];
    assert(alignmentPoints.size() == 1);
    int j = *(alignmentPoints.begin());
    if (inverseFlag) {
      out << vcbT.getWord(phraseT[j]) << word << " ";
    } else {
      out << word << vcbT.getWord(phraseT[j]) << " ";
    }
  }
  // output source root symbol
  if (conditionOnTargetLhsFlag && !inverseFlag) {
    out << "[X]";
  } else {
    out << vcbS.getWord(phraseS.back());
  }
}

void printTargetPhrase(const PHRASE &phraseS, const PHRASE &phraseT,
                       const PhraseAlignment &bestAlignment, ostream &out)
{
  // output target symbols, except root, in rule table format
  for (std::size_t i = 0; i < phraseT.size()-1; ++i) {
    const std::string &word = vcbT.getWord(phraseT[i]);
    if (!unpairedExtractFormatFlag || !isNonTerminal(word)) {
      out << word << " ";
      continue;
    }
    // get corresponding source non-terminal and output pair
    std::set<std::size_t> alignmentPoints = bestAlignment.alignedToT[i];
    assert(alignmentPoints.size() == 1);
    int j = *(alignmentPoints.begin());
    if (inverseFlag) {
      out << word << vcbS.getWord(phraseS[j]) << " ";
    } else {
      out << vcbS.getWord(phraseS[j]) << word << " ";
    }
  }
  // output target root symbol
  if (conditionOnTargetLhsFlag) {
    if (inverseFlag) {
      out << "[X]";
    } else {
      out << vcbS.getWord(phraseS.back());
    }
  } else {
    out << vcbT.getWord(phraseT.back());
  }
}

std::pair<PhrasePairGroup::Coll::iterator,bool> PhrasePairGroup::insert ( const PhraseAlignmentCollection& obj )
{
  std::pair<iterator,bool> ret = m_coll.insert(obj);

  if (ret.second) {
    // obj inserted. Also add to sorted vector
    const PhraseAlignmentCollection &insertedObj = *ret.first;
    m_sortedColl.push_back(&insertedObj);
  }

  return ret;
}


