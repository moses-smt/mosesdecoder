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
#include <assert.h>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/unordered_map.hpp>

#include "ScoreFeature.h"
#include "tables-core.h"
#include "ExtractionPhrasePair.h"
#include "score.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"

#include "moses/Util.h"

using namespace boost::algorithm;
using namespace MosesTraining;

namespace MosesTraining
{
LexicalTable lexTable;
bool inverseFlag = false;
bool hierarchicalFlag = false;
bool pcfgFlag = false;
bool phraseOrientationFlag = false;
bool treeFragmentsFlag = false;
bool partsOfSpeechFlag = false;
bool sourceSyntaxLabelsFlag = false;
bool sourceSyntaxLabelCountsLHSFlag = false;
bool targetSyntacticPreferencesFlag = false;
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
bool crossedNonTerm = false;
bool spanLength = false;
bool ruleLength = false;
bool nonTermContext = false;
bool nonTermContextTarget = false;
bool targetConstituentBoundariesFlag = false;

int countOfCounts[COC_MAX+1];
int totalDistinct = 0;
float minCount = 0;
float minCountHierarchical = 0;
bool phraseOrientationPriorsFlag = false;

boost::unordered_map<std::string,float> sourceLHSCounts;
boost::unordered_map<std::string, boost::unordered_map<std::string,float>* > targetLHSAndSourceLHSJointCounts;
std::set<std::string> sourceLabelSet;
std::map<std::string,size_t> sourceLabels;
std::vector<std::string> sourceLabelsByIndex;

std::set<std::string> partsOfSpeechSet;

boost::unordered_map<std::string,float> targetSyntacticPreferencesLHSCounts;
boost::unordered_map<std::string, boost::unordered_map<std::string,float>* > ruleTargetLHSAndTargetSyntacticPreferencesLHSJointCounts;
std::set<std::string> targetSyntacticPreferencesLabelSet;
std::map<std::string,size_t> targetSyntacticPreferencesLabels;
std::vector<std::string> targetSyntacticPreferencesLabelsByIndex;

std::vector<float> orientationClassPriorsL2R(4,0); // mono swap dleft dright
std::vector<float> orientationClassPriorsR2L(4,0); // mono swap dleft dright

Vocabulary vcbT;
Vocabulary vcbS;

} // namespace


void processLine( std::string line,
                  int lineID, bool includeSentenceIdFlag, int &sentenceId,
                  PHRASE *phraseSource, PHRASE *phraseTarget, ALIGNMENT *targetToSourceAlignment,
                  std::string &additionalPropertiesString,
                  float &count, float &pcfgSum );
void writeCountOfCounts( const std::string &fileNameCountOfCounts );
void writeLeftHandSideLabelCounts( const boost::unordered_map<std::string,float> &countsLabelLHS,
                                   const boost::unordered_map<std::string, boost::unordered_map<std::string,float>* > &jointCountsLabelLHS,
                                   const std::string &fileNameLeftHandSideSourceLabelCounts,
                                   const std::string &fileNameLeftHandSideTargetSourceLabelCounts );
void writeLabelSet( const std::set<std::string> &labelSet, const std::string &fileName );
void processPhrasePairs( std::vector< ExtractionPhrasePair* > &phrasePairsWithSameSource, std::ostream &phraseTableFile,
                         const ScoreFeatureManager& featureManager, const MaybeLog& maybeLogProb );
void outputPhrasePair(const ExtractionPhrasePair &phrasePair, float, int, std::ostream &phraseTableFile, const ScoreFeatureManager &featureManager, const MaybeLog &maybeLog );
double computeLexicalTranslation( const PHRASE *phraseSource, const PHRASE *phraseTarget, const ALIGNMENT *alignmentTargetToSource );
double computeUnalignedPenalty( const ALIGNMENT *alignmentTargetToSource );
std::set<std::string> functionWordList;
void loadOrientationPriors(const std::string &fileNamePhraseOrientationPriors, std::vector<float> &orientationClassPriorsL2R, std::vector<float> &orientationClassPriorsR2L);
void loadFunctionWords( const std::string &fileNameFunctionWords );
double computeUnalignedFWPenalty( const PHRASE *phraseTarget, const ALIGNMENT *alignmentTargetToSource );
int calcCrossedNonTerm( const PHRASE *phraseTarget, const ALIGNMENT *alignmentTargetToSource );
void printSourcePhrase( const PHRASE *phraseSource, const PHRASE *phraseTarget, const ALIGNMENT *targetToSourceAlignment, std::ostream &out );
void printTargetPhrase( const PHRASE *phraseSource, const PHRASE *phraseTarget, const ALIGNMENT *targetToSourceAlignment, std::ostream &out );
void invertAlignment( const PHRASE *phraseSource, const PHRASE *phraseTarget, const ALIGNMENT *inTargetToSourceAlignment, ALIGNMENT *outSourceToTargetAlignment );
size_t NumNonTerminal(const PHRASE *phraseSource);


int main(int argc, char* argv[])
{
  std::cerr << "Score v2.1 -- "
            << "scoring methods for extracted rules" << std::endl;

  ScoreFeatureManager featureManager;
  if (argc < 4) {
    std::cerr <<
              "syntax: score extract lex phrase-table "
              "[--Inverse] "
              "[--Hierarchical] "
              "[--LogProb] "
              "[--NegLogProb] "
              "[--NoLex] "
              "[--GoodTuring] "
              "[--KneserNey] "
              "[--NoWordAlignment] "
              "[--UnalignedPenalty] "
              "[--UnalignedFunctionWordPenalty function-word-file] "
              "[--MinCountHierarchical count] "
              "[--PartsOfSpeech] "
              "[--PCFG] "
              "[--TreeFragments] "
              "[--SourceLabels] "
              "[--SourceLabelCountsLHS] "
              "[--TargetSyntacticPreferences] "
              "[--UnpairedExtractFormat] "
              "[--ConditionOnTargetLHS] "
              "[--CrossedNonTerm]"
              << std::endl;
    std::cerr << featureManager.usage() << std::endl;
    exit(1);
  }
  std::string fileNameExtract = argv[1];
  std::string fileNameLex = argv[2];
  std::string fileNamePhraseTable = argv[3];
  std::string fileNameSourceLabelSet;
  std::string fileNamePartsOfSpeechSet;
  std::string fileNameCountOfCounts;
  std::string fileNameFunctionWords;
  std::string fileNameLeftHandSideSourceLabelCounts;
  std::string fileNameLeftHandSideTargetSourceLabelCounts;
  std::string fileNameTargetSyntacticPreferencesLabelSet;
  std::string fileNameLeftHandSideTargetSyntacticPreferencesLabelCounts;
  std::string fileNameLeftHandSideRuleTargetTargetSyntacticPreferencesLabelCounts;
  std::string fileNamePhraseOrientationPriors;
  // All unknown args are passed to feature manager.
  std::vector<std::string> featureArgs;

  for(int i=4; i<argc; i++) {
    if (strcmp(argv[i],"inverse") == 0 || strcmp(argv[i],"--Inverse") == 0) {
      inverseFlag = true;
      std::cerr << "using inverse mode" << std::endl;
    } else if (strcmp(argv[i],"--Hierarchical") == 0) {
      hierarchicalFlag = true;
      std::cerr << "processing hierarchical rules" << std::endl;
    } else if (strcmp(argv[i],"--PCFG") == 0) {
      pcfgFlag = true;
      std::cerr << "including PCFG scores" << std::endl;
    } else if (strcmp(argv[i],"--PhraseOrientation") == 0) {
      phraseOrientationFlag = true;
      std::cerr << "including phrase orientation information" << std::endl;
    } else if (strcmp(argv[i],"--TreeFragments") == 0) {
      treeFragmentsFlag = true;
      std::cerr << "including tree fragment information from syntactic parse" << std::endl;
    } else if (strcmp(argv[i],"--PartsOfSpeech") == 0) {
      partsOfSpeechFlag = true;
      std::cerr << "including parts-of-speech information from syntactic parse" << std::endl;
      fileNamePartsOfSpeechSet = std::string(fileNamePhraseTable) + ".partsOfSpeech";
      std::cerr << "writing parts-of-speech set to file " << fileNamePartsOfSpeechSet << std::endl;
    } else if (strcmp(argv[i],"--SourceLabels") == 0) {
      sourceSyntaxLabelsFlag = true;
      std::cerr << "including source label information" << std::endl;
      fileNameSourceLabelSet = std::string(fileNamePhraseTable) + ".syntaxLabels.src";
      std::cerr << "writing source syntax label set to file " << fileNameSourceLabelSet << std::endl;
    } else if (strcmp(argv[i],"--SourceLabelCountsLHS") == 0) {
      sourceSyntaxLabelCountsLHSFlag = true;
      fileNameLeftHandSideSourceLabelCounts = std::string(fileNamePhraseTable) + ".src.lhs";
      fileNameLeftHandSideTargetSourceLabelCounts = std::string(fileNamePhraseTable) + ".tgt-src.lhs";
      std::cerr << "counting left-hand side source labels and writing them to files " << fileNameLeftHandSideSourceLabelCounts << " and " << fileNameLeftHandSideTargetSourceLabelCounts << std::endl;
    } else if (strcmp(argv[i],"--TargetSyntacticPreferences") == 0) {
      targetSyntacticPreferencesFlag = true;
      std::cerr << "including target syntactic preferences information" << std::endl;
      fileNameTargetSyntacticPreferencesLabelSet = std::string(fileNamePhraseTable) + ".syntaxLabels.tgtpref";
      std::cerr << "writing target syntactic preferences label set to file " << fileNameTargetSyntacticPreferencesLabelSet << std::endl;
      fileNameLeftHandSideTargetSyntacticPreferencesLabelCounts = std::string(fileNamePhraseTable) + ".tgtpref.lhs";
      fileNameLeftHandSideRuleTargetTargetSyntacticPreferencesLabelCounts = std::string(fileNamePhraseTable) + ".tgt-tgtpref.lhs";
      std::cerr << "counting left-hand side target syntactic preferences labels and writing them to files "
                << fileNameLeftHandSideTargetSyntacticPreferencesLabelCounts
                << " and "
                << fileNameLeftHandSideRuleTargetTargetSyntacticPreferencesLabelCounts
                << std::endl;
    } else if (strcmp(argv[i],"--UnpairedExtractFormat") == 0) {
      unpairedExtractFormatFlag = true;
      std::cerr << "processing unpaired extract format" << std::endl;
    } else if (strcmp(argv[i],"--ConditionOnTargetLHS") == 0) {
      conditionOnTargetLhsFlag = true;
      std::cerr << "processing unpaired extract format" << std::endl;
    } else if (strcmp(argv[i],"--NoWordAlignment") == 0) {
      wordAlignmentFlag = false;
      std::cerr << "omitting word alignment" << std::endl;
    } else if (strcmp(argv[i],"--NoLex") == 0) {
      lexFlag = false;
      std::cerr << "not computing lexical translation score" << std::endl;
    } else if (strcmp(argv[i],"--GoodTuring") == 0) {
      goodTuringFlag = true;
      fileNameCountOfCounts = std::string(fileNamePhraseTable) + ".coc";
      std::cerr << "adjusting phrase translation probabilities with Good Turing discounting" << std::endl;
    } else if (strcmp(argv[i],"--KneserNey") == 0) {
      kneserNeyFlag = true;
      fileNameCountOfCounts = std::string(fileNamePhraseTable) + ".coc";
      std::cerr << "adjusting phrase translation probabilities with Kneser Ney discounting" << std::endl;
    } else if (strcmp(argv[i],"--UnalignedPenalty") == 0) {
      unalignedFlag = true;
      std::cerr << "using unaligned word penalty" << std::endl;
    } else if (strcmp(argv[i],"--UnalignedFunctionWordPenalty") == 0) {
      unalignedFWFlag = true;
      if (i+1==argc) {
        std::cerr << "ERROR: specify function words file for unaligned function word penalty!" << std::endl;
        exit(1);
      }
      fileNameFunctionWords = argv[++i];
      std::cerr << "using unaligned function word penalty with function words from " << fileNameFunctionWords << std::endl;
    }  else if (strcmp(argv[i],"--LogProb") == 0) {
      logProbFlag = true;
      std::cerr << "using log-probabilities" << std::endl;
    } else if (strcmp(argv[i],"--NegLogProb") == 0) {
      logProbFlag = true;
      negLogProb = -1;
      std::cerr << "using negative log-probabilities" << std::endl;
    } else if (strcmp(argv[i],"--MinCount") == 0) {
      minCount = std::atof( argv[++i] );
      std::cerr << "dropping all phrase pairs occurring less than " << minCount << " times" << std::endl;
      minCount -= 0.00001; // account for rounding
    } else if (strcmp(argv[i],"--MinCountHierarchical") == 0) {
      minCountHierarchical = std::atof( argv[++i] );
      std::cerr << "dropping all hierarchical phrase pairs occurring less than " << minCountHierarchical << " times" << std::endl;
      minCountHierarchical -= 0.00001; // account for rounding
    } else if (strcmp(argv[i],"--CrossedNonTerm") == 0) {
      crossedNonTerm = true;
      std::cerr << "crossed non-term reordering feature" << std::endl;
    } else if (strcmp(argv[i],"--PhraseOrientationPriors") == 0) {
      phraseOrientationPriorsFlag = true;
      if (i+1==argc) {
        std::cerr << "ERROR: specify priors file for phrase orientation!" << std::endl;
        exit(1);
      }
      fileNamePhraseOrientationPriors = argv[++i];
      std::cerr << "smoothing phrase orientation with priors from " << fileNamePhraseOrientationPriors << std::endl;
    } else if (strcmp(argv[i],"--SpanLength") == 0) {
      spanLength = true;
      std::cerr << "span length feature" << std::endl;
    } else if (strcmp(argv[i],"--RuleLength") == 0) {
      ruleLength = true;
      std::cerr << "rule length feature" << std::endl;
    } else if (strcmp(argv[i],"--NonTermContext") == 0) {
      nonTermContext = true;
      std::cerr << "non-term context" << std::endl;
    } else if (strcmp(argv[i],"--NonTermContextTarget") == 0) {
      nonTermContextTarget = true;
      std::cerr << "non-term context (target)" << std::endl;
    } else if (strcmp(argv[i],"--TargetConstituentBoundaries") == 0) {
      targetConstituentBoundariesFlag = true;
      std::cerr << "including target constituent boundaries information" << std::endl;
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

  // configure extra features
  if (!inverseFlag) {
    featureManager.configure(featureArgs);
  }

  // lexical translation table
  if (lexFlag) {
    lexTable.load( fileNameLex );
  }

  // function word list
  if (unalignedFWFlag) {
    loadFunctionWords( fileNameFunctionWords );
  }

  // compute count of counts for Good Turing discounting
  if (goodTuringFlag || kneserNeyFlag) {
    for(int i=1; i<=COC_MAX; i++) countOfCounts[i] = 0;
  }

  if (phraseOrientationPriorsFlag) {
    loadOrientationPriors(fileNamePhraseOrientationPriors,orientationClassPriorsL2R,orientationClassPriorsR2L);
  }

  // sorted phrase extraction file
  Moses::InputFileStream extractFile(fileNameExtract);

  if (extractFile.fail()) {
    std::cerr << "ERROR: could not open extract file " << fileNameExtract << std::endl;
    exit(1);
  }

  // output file: phrase translation table
  std::ostream *phraseTableFile;

  if (fileNamePhraseTable == "-") {
    phraseTableFile = &std::cout;
  } else {
    Moses::OutputFileStream *outputFile = new Moses::OutputFileStream();
    bool success = outputFile->Open(fileNamePhraseTable);
    if (!success) {
      std::cerr << "ERROR: could not open file phrase table file "
                << fileNamePhraseTable << std::endl;
      exit(1);
    }
    phraseTableFile = outputFile;
  }

  // loop through all extracted phrase translations
  std::string line, lastLine;
  ExtractionPhrasePair *phrasePair = NULL;
  std::vector< ExtractionPhrasePair* > phrasePairsWithSameSource;
  std::vector< ExtractionPhrasePair* > phrasePairsWithSameSourceAndTarget; // required for hierarchical rules only, as non-terminal alignments might make the phrases incompatible

  int tmpSentenceId;
  PHRASE *tmpPhraseSource, *tmpPhraseTarget;
  ALIGNMENT *tmpTargetToSourceAlignment;
  std::string tmpAdditionalPropertiesString;
  float tmpCount=0.0f, tmpPcfgSum=0.0f;

  int i=0;
  if ( getline(extractFile, line) ) {
    ++i;
    tmpPhraseSource = new PHRASE();
    tmpPhraseTarget = new PHRASE();
    tmpTargetToSourceAlignment = new ALIGNMENT();
    processLine( std::string(line),
                 i, featureManager.includeSentenceId(), tmpSentenceId,
                 tmpPhraseSource, tmpPhraseTarget, tmpTargetToSourceAlignment,
                 tmpAdditionalPropertiesString,
                 tmpCount, tmpPcfgSum);
    phrasePair = new ExtractionPhrasePair( tmpPhraseSource, tmpPhraseTarget,
                                           tmpTargetToSourceAlignment,
                                           tmpCount, tmpPcfgSum );
    phrasePair->AddProperties( tmpAdditionalPropertiesString, tmpCount );
    featureManager.addPropertiesToPhrasePair( *phrasePair, tmpCount, tmpSentenceId );
    phrasePairsWithSameSource.push_back( phrasePair );
    if ( hierarchicalFlag ) {
      phrasePairsWithSameSourceAndTarget.push_back( phrasePair );
    }
    lastLine = line;
  }

  while ( getline(extractFile, line) ) {

    // Print progress dots to stderr.
    if ( ++i % 100000 == 0 ) {
      std::cerr << "." << std::flush;
    }

    // identical to last line? just add count
    if (line == lastLine) {
      phrasePair->IncrementPrevious(tmpCount,tmpPcfgSum);
      continue;
    } else {
      lastLine = line;
    }

    tmpPhraseSource = new PHRASE();
    tmpPhraseTarget = new PHRASE();
    tmpTargetToSourceAlignment = new ALIGNMENT();
    tmpAdditionalPropertiesString.clear();
    processLine( std::string(line),
                 i, featureManager.includeSentenceId(), tmpSentenceId,
                 tmpPhraseSource, tmpPhraseTarget, tmpTargetToSourceAlignment,
                 tmpAdditionalPropertiesString,
                 tmpCount, tmpPcfgSum);

    bool matchesPrevious = false;
    bool sourceMatch = true;
    bool targetMatch = true;
    bool alignmentMatch = true; // be careful with these,
    // ExtractionPhrasePair::Matches() checks them in order and does not continue with the others
    // once the first of them has been found to have to be set to false

    if ( hierarchicalFlag ) {
      for ( std::vector< ExtractionPhrasePair* >::const_iterator iter = phrasePairsWithSameSourceAndTarget.begin();
            iter != phrasePairsWithSameSourceAndTarget.end(); ++iter ) {
        if ( (*iter)->Matches( tmpPhraseSource, tmpPhraseTarget, tmpTargetToSourceAlignment,
                               sourceMatch, targetMatch, alignmentMatch ) ) {
          matchesPrevious = true;
          phrasePair = (*iter);
          break;
        }
      }
    } else {
      if ( phrasePair->Matches( tmpPhraseSource, tmpPhraseTarget, tmpTargetToSourceAlignment,
                                sourceMatch, targetMatch, alignmentMatch ) ) {
        matchesPrevious = true;
      }
    }

    if ( matchesPrevious ) {
      delete tmpPhraseSource;
      delete tmpPhraseTarget;
      if ( !phrasePair->Add( tmpTargetToSourceAlignment,
                             tmpCount, tmpPcfgSum ) ) {
        delete tmpTargetToSourceAlignment;
      }
      phrasePair->AddProperties( tmpAdditionalPropertiesString, tmpCount );
      featureManager.addPropertiesToPhrasePair( *phrasePair, tmpCount, tmpSentenceId );
    } else {

      if ( !phrasePairsWithSameSource.empty() &&
           !sourceMatch ) {
        processPhrasePairs( phrasePairsWithSameSource, *phraseTableFile, featureManager, maybeLogProb );
        for ( std::vector< ExtractionPhrasePair* >::const_iterator iter=phrasePairsWithSameSource.begin();
              iter!=phrasePairsWithSameSource.end(); ++iter) {
          delete *iter;
        }
        phrasePairsWithSameSource.clear();
        if ( hierarchicalFlag ) {
          phrasePairsWithSameSourceAndTarget.clear();
        }
      }

      if ( hierarchicalFlag ) {
        if ( !phrasePairsWithSameSourceAndTarget.empty() &&
             !targetMatch ) {
          phrasePairsWithSameSourceAndTarget.clear();
        }
      }

      phrasePair = new ExtractionPhrasePair( tmpPhraseSource, tmpPhraseTarget,
                                             tmpTargetToSourceAlignment,
                                             tmpCount, tmpPcfgSum );
      phrasePair->AddProperties( tmpAdditionalPropertiesString, tmpCount );
      featureManager.addPropertiesToPhrasePair( *phrasePair, tmpCount, tmpSentenceId );
      phrasePairsWithSameSource.push_back(phrasePair);

      if ( hierarchicalFlag ) {
        phrasePairsWithSameSourceAndTarget.push_back(phrasePair);
      }
    }

  }

  // We've been printing progress dots to stderr.  End the line.
  std::cerr << std::endl;

  processPhrasePairs( phrasePairsWithSameSource, *phraseTableFile, featureManager, maybeLogProb );
  for ( std::vector< ExtractionPhrasePair* >::const_iterator iter=phrasePairsWithSameSource.begin();
        iter!=phrasePairsWithSameSource.end(); ++iter) {
    delete *iter;
  }
  phrasePairsWithSameSource.clear();


  phraseTableFile->flush();
  if (phraseTableFile != &std::cout) {
    delete phraseTableFile;
  }

  // output count of count statistics
  if (goodTuringFlag || kneserNeyFlag) {
    writeCountOfCounts( fileNameCountOfCounts );
  }

  // source syntax labels
  if (sourceSyntaxLabelsFlag && !inverseFlag) {
    writeLabelSet( sourceLabelSet, fileNameSourceLabelSet );
  }
  if (sourceSyntaxLabelsFlag && sourceSyntaxLabelCountsLHSFlag && !inverseFlag) {
    writeLeftHandSideLabelCounts( sourceLHSCounts,
                                  targetLHSAndSourceLHSJointCounts,
                                  fileNameLeftHandSideSourceLabelCounts,
                                  fileNameLeftHandSideTargetSourceLabelCounts );
  }

  // parts-of-speech
  if (partsOfSpeechFlag && !inverseFlag) {
    writeLabelSet( partsOfSpeechSet, fileNamePartsOfSpeechSet );
  }

  // target syntactic preferences labels
  if (targetSyntacticPreferencesFlag && !inverseFlag) {
    writeLabelSet( targetSyntacticPreferencesLabelSet, fileNameTargetSyntacticPreferencesLabelSet );
    writeLeftHandSideLabelCounts( targetSyntacticPreferencesLHSCounts,
                                  ruleTargetLHSAndTargetSyntacticPreferencesLHSJointCounts,
                                  fileNameLeftHandSideTargetSyntacticPreferencesLabelCounts,
                                  fileNameLeftHandSideRuleTargetTargetSyntacticPreferencesLabelCounts );
  }
}


void processLine( std::string line,
                  int lineID, bool includeSentenceIdFlag, int &sentenceId,
                  PHRASE *phraseSource, PHRASE *phraseTarget, ALIGNMENT *targetToSourceAlignment,
                  std::string &additionalPropertiesString,
                  float &count, float &pcfgSum )
{
  size_t foundAdditionalProperties = line.rfind("|||");
  foundAdditionalProperties = line.find("{{",foundAdditionalProperties);
  if (foundAdditionalProperties != std::string::npos) {
    additionalPropertiesString = line.substr(foundAdditionalProperties);
    line = line.substr(0,foundAdditionalProperties);
  } else {
    additionalPropertiesString.clear();
  }

  phraseSource->clear();
  phraseTarget->clear();
  targetToSourceAlignment->clear();

  std::vector<std::string> token;
  Moses::Tokenize( token, line );
  int item = 1;
  for ( size_t j=0; j<token.size(); ++j ) {
    if (token[j] == "|||") {
      ++item;
    } else if (item == 1) { // source phrase
      phraseSource->push_back( vcbS.storeIfNew( token[j] ) );
    } else if (item == 2) { // target phrase
      phraseTarget->push_back( vcbT.storeIfNew( token[j] ) );
    } else if (item == 3) { // alignment
      int s,t;
      sscanf(token[j].c_str(), "%d-%d", &s, &t);
      if ((size_t)t >= phraseTarget->size() || (size_t)s >= phraseSource->size()) {
        std::cerr << "WARNING: phrase pair " << lineID
                  << " has alignment point (" << s << ", " << t << ")"
                  << " out of bounds (" << phraseSource->size() << ", " << phraseTarget->size() << ")"
                  << std::endl;
      } else {
        // first alignment point? -> initialize
        if ( targetToSourceAlignment->size() == 0 ) {
          size_t numberOfTargetSymbols = (hierarchicalFlag ? phraseTarget->size()-1 : phraseTarget->size());
          targetToSourceAlignment->resize(numberOfTargetSymbols);
        }
        // add alignment point
        targetToSourceAlignment->at(t).insert(s);
      }
    } else if (includeSentenceIdFlag && item == 4) { // optional sentence id
      sscanf(token[j].c_str(), "%d", &sentenceId);
    } else if (item + (includeSentenceIdFlag?-1:0) == 4) { // count
      sscanf(token[j].c_str(), "%f", &count);
    } else if (item + (includeSentenceIdFlag?-1:0) == 5) { // target syntax PCFG score
      float pcfgScore = std::atof( token[j].c_str() );
      pcfgSum = pcfgScore * count;
    }
  }

  if ( targetToSourceAlignment->size() == 0 ) {
    size_t numberOfTargetSymbols = (hierarchicalFlag ? phraseTarget->size()-1 : phraseTarget->size());
    targetToSourceAlignment->resize(numberOfTargetSymbols);
  }

  if (item + (includeSentenceIdFlag?-1:0) == 3) {
    count = 1.0;
  }
  if (item < 3 || item > (includeSentenceIdFlag?7:6)) {
    std::cerr << "ERROR: faulty line " << lineID << ": " << line << std::endl;
  }

}


void writeCountOfCounts( const std::string &fileNameCountOfCounts )
{
  // open file
  Moses::OutputFileStream countOfCountsFile;
  bool success = countOfCountsFile.Open(fileNameCountOfCounts);
  if (!success) {
    std::cerr << "ERROR: could not open count-of-counts file "
              << fileNameCountOfCounts << std::endl;
    return;
  }

  // Kneser-Ney needs the total number of phrase pairs
  countOfCountsFile << totalDistinct << std::endl;

  // write out counts
  for(int i=1; i<=COC_MAX; i++) {
    countOfCountsFile << countOfCounts[ i ] << std::endl;
  }
  countOfCountsFile.Close();
}


void writeLeftHandSideLabelCounts( const boost::unordered_map<std::string,float> &countsLabelLHS,
                                   const boost::unordered_map<std::string, boost::unordered_map<std::string,float>* > &jointCountsLabelLHS,
                                   const std::string &fileNameLeftHandSideSourceLabelCounts,
                                   const std::string &fileNameLeftHandSideTargetSourceLabelCounts )
{
  // open file
  Moses::OutputFileStream leftHandSideSourceLabelCounts;
  bool success = leftHandSideSourceLabelCounts.Open(fileNameLeftHandSideSourceLabelCounts);
  if (!success) {
    std::cerr << "ERROR: could not open left-hand side label counts file "
              << fileNameLeftHandSideSourceLabelCounts << std::endl;
    return;
  }

  // write source left-hand side counts
  for (boost::unordered_map<std::string,float>::const_iterator iter=sourceLHSCounts.begin();
       iter!=sourceLHSCounts.end(); ++iter) {
    leftHandSideSourceLabelCounts << iter->first << " " << iter->second << std::endl;
  }

  leftHandSideSourceLabelCounts.Close();

  // open file
  Moses::OutputFileStream leftHandSideTargetSourceLabelCounts;
  success = leftHandSideTargetSourceLabelCounts.Open(fileNameLeftHandSideTargetSourceLabelCounts);
  if (!success) {
    std::cerr << "ERROR: could not open left-hand side label joint counts file "
              << fileNameLeftHandSideTargetSourceLabelCounts << std::endl;
    return;
  }

  // write source left-hand side / target left-hand side joint counts
  for (boost::unordered_map<std::string, boost::unordered_map<std::string,float>* >::const_iterator iter=targetLHSAndSourceLHSJointCounts.begin();
       iter!=targetLHSAndSourceLHSJointCounts.end(); ++iter) {
    for (boost::unordered_map<std::string,float>::const_iterator iter2=(iter->second)->begin();
         iter2!=(iter->second)->end(); ++iter2) {
      leftHandSideTargetSourceLabelCounts << iter->first << " "<< iter2->first << " " << iter2->second << std::endl;
    }
  }

  leftHandSideTargetSourceLabelCounts.Close();
}


void writeLabelSet( const std::set<std::string> &labelSet, const std::string &fileName )
{
  // open file
  Moses::OutputFileStream out;
  bool success = out.Open(fileName);
  if (!success) {
    std::cerr << "ERROR: could not open file "
              << fileName << " for writing" << std::endl;
    return;
  }

  for (std::set<std::string>::const_iterator iter=labelSet.begin();
       iter!=labelSet.end(); ++iter) {
    out << *iter << std::endl;
  }

  out.Close();
}


void processPhrasePairs( std::vector< ExtractionPhrasePair* > &phrasePairsWithSameSource, std::ostream &phraseTableFile,
                         const ScoreFeatureManager& featureManager, const MaybeLog& maybeLogProb )
{
  if (phrasePairsWithSameSource.size() == 0) {
    return;
  }

  float totalSource = 0;

  //std::cerr << "phrasePairs.size() = " << phrasePairs.size() << std::endl;

  // loop through phrase pairs
  for ( std::vector< ExtractionPhrasePair* >::const_iterator iter=phrasePairsWithSameSource.begin();
        iter!=phrasePairsWithSameSource.end(); ++iter) {
    // add to total count
    totalSource += (*iter)->GetCount();
  }

  // output the distinct phrase pairs, one at a time
  for ( std::vector< ExtractionPhrasePair* >::const_iterator iter=phrasePairsWithSameSource.begin();
        iter!=phrasePairsWithSameSource.end(); ++iter) {
    // add to total count
    outputPhrasePair( **iter, totalSource, phrasePairsWithSameSource.size(), phraseTableFile, featureManager, maybeLogProb );
  }
}

void outputPhrasePair(const ExtractionPhrasePair &phrasePair,
                      float totalCount, int distinctCount,
                      std::ostream &phraseTableFile,
                      const ScoreFeatureManager& featureManager,
                      const MaybeLog& maybeLogProb )
{
  assert(phrasePair.IsValid());

  const ALIGNMENT *bestAlignmentT2S = phrasePair.FindBestAlignmentTargetToSource();
  float count = phrasePair.GetCount();

  std::map< std::string, float > domainCount;

  // collect count of count statistics
  if (goodTuringFlag || kneserNeyFlag) {
    totalDistinct++;
    int countInt = count + 0.99999;
    if ((countInt <= COC_MAX) &&
        (countInt > 0))
      countOfCounts[ countInt ]++;
  }

  // output phrases
  const PHRASE *phraseSource = phrasePair.GetSource();
  const PHRASE *phraseTarget = phrasePair.GetTarget();

  // do not output if count below threshold
  if (count < minCount) {
    return;
  }

  // do not output if hierarchical and count below threshold
  if (hierarchicalFlag && count < minCountHierarchical) {
    for(size_t j=0; j<phraseSource->size()-1; ++j) {
      if (isNonTerminal(vcbS.getWord( phraseSource->at(j) )))
        return;
    }
  }

  // compute PCFG score
  float pcfgScore = 0;
  if (pcfgFlag && !inverseFlag) {
    pcfgScore = phrasePair.GetPcfgScore() / count;
  }

  // source phrase (unless inverse)
  if (!inverseFlag) {
    printSourcePhrase(phraseSource, phraseTarget, bestAlignmentT2S, phraseTableFile);
    phraseTableFile << " ||| ";
  }

  // target phrase
  printTargetPhrase(phraseSource, phraseTarget, bestAlignmentT2S, phraseTableFile);
  phraseTableFile << " ||| ";

  // source phrase (if inverse)
  if (inverseFlag) {
    printSourcePhrase(phraseSource, phraseTarget, bestAlignmentT2S, phraseTableFile);
    phraseTableFile << " ||| ";
  }

  // alignment
  if ( hierarchicalFlag ) {
    // always output alignment if hiero style
    assert(phraseTarget->size() == bestAlignmentT2S->size()+1);
    std::vector<std::string> alignment;
    for ( size_t j = 0; j < phraseTarget->size() - 1; ++j ) {
      if ( isNonTerminal(vcbT.getWord( phraseTarget->at(j) ))) {
        if ( bestAlignmentT2S->at(j).size() != 1 ) {
          std::cerr << "Error: unequal numbers of non-terminals. Make sure the text does not contain words in square brackets (like [xxx])." << std::endl;
          phraseTableFile.flush();
          assert(bestAlignmentT2S->at(j).size() == 1);
        }
        size_t sourcePos = *(bestAlignmentT2S->at(j).begin());
        //phraseTableFile << sourcePos << "-" << j << " ";
        std::stringstream point;
        point << sourcePos << "-" << j;
        alignment.push_back(point.str());
      } else {
        for ( std::set<size_t>::iterator setIter = (bestAlignmentT2S->at(j)).begin();
              setIter != (bestAlignmentT2S->at(j)).end(); ++setIter ) {
          size_t sourcePos = *setIter;
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
  } else if ( !inverseFlag && wordAlignmentFlag) {
    // alignment info in pb model
    for (size_t j = 0; j < bestAlignmentT2S->size(); ++j) {
      for ( std::set<size_t>::iterator setIter = (bestAlignmentT2S->at(j)).begin();
            setIter != (bestAlignmentT2S->at(j)).end(); ++setIter ) {
        size_t sourcePos = *setIter;
        phraseTableFile << sourcePos << "-" << j << " ";
      }
    }
  }

  phraseTableFile << " ||| ";

  // lexical translation probability
  if (lexFlag) {
    double lexScore = computeLexicalTranslation( phraseSource, phraseTarget, bestAlignmentT2S );
    phraseTableFile << maybeLogProb( lexScore );
  }

  // unaligned word penalty
  if (unalignedFlag) {
    double penalty = computeUnalignedPenalty( bestAlignmentT2S );
    phraseTableFile << " " << maybeLogProb( penalty );
  }

  // unaligned function word penalty
  if (unalignedFWFlag) {
    double penalty = computeUnalignedFWPenalty( phraseTarget, bestAlignmentT2S );
    phraseTableFile << " " << maybeLogProb( penalty );
  }

  if (crossedNonTerm && !inverseFlag) {
    phraseTableFile << " " << calcCrossedNonTerm( phraseTarget, bestAlignmentT2S );
  }

  // target-side PCFG score
  if (pcfgFlag && !inverseFlag) {
    phraseTableFile << " " << maybeLogProb( pcfgScore );
  }

  // extra features
  ScoreFeatureContext context(phrasePair, maybeLogProb);
  std::vector<float> extraDense;
  std::map<std::string,float> extraSparse;
  featureManager.addFeatures(context, extraDense, extraSparse);
  for (size_t i = 0; i < extraDense.size(); ++i) {
    phraseTableFile << " " << extraDense[i];
  }

  for (std::map<std::string,float>::const_iterator i = extraSparse.begin();
       i != extraSparse.end(); ++i) {
    phraseTableFile << " " << i->first << " " << i->second;
  }

  // counts
  phraseTableFile << " ||| " << totalCount << " " << count;
  if (kneserNeyFlag)
    phraseTableFile << " " << distinctCount;

  phraseTableFile << " |||";

  // tree fragments
  if (treeFragmentsFlag && !inverseFlag) {
    const std::string *bestTreeFragment = phrasePair.FindBestPropertyValue("Tree");
    if (bestTreeFragment) {
      phraseTableFile << " {{Tree " << *bestTreeFragment << "}}";
    }
  }

  // parts-of-speech
  if (partsOfSpeechFlag && !inverseFlag) {
    phrasePair.UpdateVocabularyFromValueTokens("POS", partsOfSpeechSet);
    const std::string *bestPartOfSpeech = phrasePair.FindBestPropertyValue("POS");
    if (bestPartOfSpeech) {
      phraseTableFile << " {{POS " << *bestPartOfSpeech << "}}";
    }
  }

  // syntax labels
  if ((sourceSyntaxLabelsFlag || targetSyntacticPreferencesFlag) && !inverseFlag) {
    unsigned nNTs = 1;
    for(size_t j=0; j<phraseSource->size()-1; ++j) {
      if (isNonTerminal(vcbS.getWord( phraseSource->at(j) )))
        ++nNTs;
    }
    // source syntax labels
    if (sourceSyntaxLabelsFlag) {
      std::string sourceLabelCounts;
      sourceLabelCounts = phrasePair.CollectAllLabelsSeparateLHSAndRHS("SourceLabels",
                          sourceLabelSet,
                          sourceLHSCounts,
                          targetLHSAndSourceLHSJointCounts,
                          vcbT);
      if ( !sourceLabelCounts.empty() ) {
        phraseTableFile << " {{SourceLabels "
                        << phraseSource->size() // for convenience: number of symbols in this rule (incl. left hand side NT)
                        << " "
                        << count // rule count
                        << sourceLabelCounts
                        << "}}";
      }
    }
    // target syntactic preferences labels
    if (targetSyntacticPreferencesFlag) {
      std::string targetSyntacticPreferencesLabelCounts;
      targetSyntacticPreferencesLabelCounts = phrasePair.CollectAllLabelsSeparateLHSAndRHS("TargetPreferences",
                                              targetSyntacticPreferencesLabelSet,
                                              targetSyntacticPreferencesLHSCounts,
                                              ruleTargetLHSAndTargetSyntacticPreferencesLHSJointCounts,
                                              vcbT);
      if (!targetSyntacticPreferencesLabelCounts.empty()) {
        phraseTableFile << " {{TargetPreferences "
                        << nNTs // for convenience: number of non-terminal symbols in this rule (incl. left hand side NT)
                        << " "
                        << count // rule count
                        << targetSyntacticPreferencesLabelCounts
                        << "}}";
      }
    }
  }

  // phrase orientation
  if (phraseOrientationFlag && !inverseFlag) {
    phraseTableFile << " {{Orientation ";
    phrasePair.CollectAllPhraseOrientations("Orientation",orientationClassPriorsL2R,orientationClassPriorsR2L,0.5,phraseTableFile);
    phraseTableFile << "}}";
  }

  if (spanLength && !inverseFlag) {
    std::string propValue = phrasePair.CollectAllPropertyValues("SpanLength");
    if (!propValue.empty()) {
      phraseTableFile << " {{SpanLength " << propValue << "}}";
    }
  }

  if (ruleLength && !inverseFlag) {
    std::string propValue = phrasePair.CollectAllPropertyValues("RuleLength");
    if (!propValue.empty()) {
      phraseTableFile << " {{RuleLength " << propValue << "}}";
    }
  }

  if (nonTermContext && !inverseFlag) {
    std::string propValue = phrasePair.CollectAllPropertyValues("NonTermContext");
    if (!propValue.empty() && propValue.size() < 50000) {
      size_t nNTs = NumNonTerminal(phraseSource);
      phraseTableFile << " {{NonTermContext " << nNTs << " " << propValue << "}}";
    }
  }

  if (nonTermContextTarget && !inverseFlag) {
    std::string propValue = phrasePair.CollectAllPropertyValues("NonTermContextTarget");
    if (!propValue.empty() && propValue.size() < 50000) {
      size_t nNTs = NumNonTerminal(phraseSource);
      phraseTableFile << " {{NonTermContextTarget " << nNTs << " " << propValue << "}}";
    }
  }

  // target constituent boundaries
  if (targetConstituentBoundariesFlag && !inverseFlag) {
    const std::string targetConstituentBoundariesLeftValues = phrasePair.CollectAllPropertyValues("TargetConstituentBoundariesLeft");
    if (!targetConstituentBoundariesLeftValues.empty()) {
      phraseTableFile << " {{TargetConstituentBoundariesLeft " << targetConstituentBoundariesLeftValues << "}}";
    }
    const std::string targetConstituentBoundariesRightAdjacentValues = phrasePair.CollectAllPropertyValues("TargetConstituentBoundariesRightAdjacent");
    if (!targetConstituentBoundariesRightAdjacentValues.empty()) {
      phraseTableFile << " {{TargetConstituentBoundariesRightAdjacent " << targetConstituentBoundariesRightAdjacentValues << "}}";
    }
  }

  phraseTableFile << std::endl;
}

size_t NumNonTerminal(const PHRASE *phraseSource)
{
  size_t nNTs = 0;
  for(size_t j=0; j<phraseSource->size()-1; ++j) {
    if (isNonTerminal(vcbS.getWord( phraseSource->at(j) )))
      ++nNTs;
  }
  return nNTs;
}

void loadOrientationPriors(const std::string &fileNamePhraseOrientationPriors,
                           std::vector<float> &orientationClassPriorsL2R,
                           std::vector<float> &orientationClassPriorsR2L)
{
  assert(orientationClassPriorsL2R.size()==4 && orientationClassPriorsR2L.size()==4); // mono swap dleft dright

  std::cerr << "Loading phrase orientation priors from " << fileNamePhraseOrientationPriors;
  Moses::InputFileStream inFile(fileNamePhraseOrientationPriors);
  if (inFile.fail()) {
    std::cerr << " - ERROR: could not open file" << std::endl;
    exit(1);
  }

  std::string line;
  size_t linesRead = 0;
  float l2rSum = 0;
  float r2lSum = 0;
  while (getline(inFile, line)) {
    std::istringstream tokenizer(line);
    std::string key;
    tokenizer >> key;

    bool l2rFlag = false;
    bool r2lFlag = false;
    if (starts_with(key, "L2R_")) {
      l2rFlag = true;
    }
    if (starts_with(key, "R2L_")) {
      r2lFlag = true;
    }
    if (!l2rFlag && !r2lFlag) {
      std::cerr << " - ERROR: malformed line in orientation priors file" << std::endl;
    }
    key.erase(0,4);

    int orientationClassId = -1;
    if (!key.compare("mono")) {
      orientationClassId = 0;
    }
    if (!key.compare("swap")) {
      orientationClassId = 1;
    }
    if (!key.compare("dleft")) {
      orientationClassId = 2;
    }
    if (!key.compare("dright")) {
      orientationClassId = 3;
    }
    if (orientationClassId == -1) {
      std::cerr << " - ERROR: malformed line in orientation priors file" << std::endl;
    }

    float count;
    tokenizer >> count;

    if (l2rFlag) {
      orientationClassPriorsL2R[orientationClassId] += count;
      l2rSum += count;
    }
    if (r2lFlag) {
      orientationClassPriorsR2L[orientationClassId] += count;
      r2lSum += count;
    }

    ++linesRead;
  }

  // normalization: return prior probabilities, not counts
  if (l2rSum != 0) {
    for (std::vector<float>::iterator orientationClassPriorsL2RIt = orientationClassPriorsL2R.begin();
         orientationClassPriorsL2RIt != orientationClassPriorsL2R.end(); ++orientationClassPriorsL2RIt) {
      *orientationClassPriorsL2RIt /= l2rSum;
    }
  }
  if (r2lSum != 0) {
    for (std::vector<float>::iterator orientationClassPriorsR2LIt = orientationClassPriorsR2L.begin();
         orientationClassPriorsR2LIt != orientationClassPriorsR2L.end(); ++orientationClassPriorsR2LIt) {
      *orientationClassPriorsR2LIt /= r2lSum;
    }
  }

  std::cerr << " - read " << linesRead << " lines from orientation priors file" << std::endl;
  inFile.Close();
}



bool calcCrossedNonTerm( size_t targetPos, size_t sourcePos, const ALIGNMENT *alignmentTargetToSource )
{
  for (size_t currTarget = 0; currTarget < alignmentTargetToSource->size(); ++currTarget) {
    if (currTarget == targetPos) {
      // skip
    } else {
      const std::set<size_t> &sourceSet = alignmentTargetToSource->at(currTarget);
      for (std::set<size_t>::const_iterator iter = sourceSet.begin();
           iter != sourceSet.end(); ++iter) {
        size_t currSource = *iter;

        if ((currTarget < targetPos && currSource > sourcePos)
            || (currTarget > targetPos && currSource < sourcePos)
           ) {
          return true;
        }
      }

    }
  }

  return false;
}

int calcCrossedNonTerm( const PHRASE *phraseTarget, const ALIGNMENT *alignmentTargetToSource )
{
  assert(phraseTarget->size() >= alignmentTargetToSource->size() );

  for (size_t targetPos = 0; targetPos < alignmentTargetToSource->size(); ++targetPos) {

    if ( isNonTerminal(vcbT.getWord( phraseTarget->at(targetPos) ))) {
      const std::set<size_t> &alignmentPoints = alignmentTargetToSource->at(targetPos);
      assert( alignmentPoints.size() == 1 );
      size_t sourcePos = *alignmentPoints.begin();
      bool ret = calcCrossedNonTerm(targetPos, sourcePos, alignmentTargetToSource);
      if (ret)
        return 1;
    }
  }

  return 0;
}


double computeUnalignedPenalty( const ALIGNMENT *alignmentTargetToSource )
{
  // unaligned word counter
  double unaligned = 1.0;
  // only checking target words - source words are caught when computing inverse
  for(size_t ti=0; ti<alignmentTargetToSource->size(); ++ti) {
    const std::set< size_t > & srcIndices = alignmentTargetToSource->at(ti);
    if (srcIndices.empty()) {
      unaligned *= 2.718;
    }
  }
  return unaligned;
}


double computeUnalignedFWPenalty( const PHRASE *phraseTarget, const ALIGNMENT *alignmentTargetToSource )
{
  // unaligned word counter
  double unaligned = 1.0;
  // only checking target words - source words are caught when computing inverse
  for(size_t ti=0; ti<alignmentTargetToSource->size(); ++ti) {
    const std::set< size_t > & srcIndices = alignmentTargetToSource->at(ti);
    if (srcIndices.empty() && functionWordList.find( vcbT.getWord( phraseTarget->at(ti) ) ) != functionWordList.end()) {
      unaligned *= 2.718;
    }
  }
  return unaligned;
}

void loadFunctionWords( const std::string &fileName )
{
  std::cerr << "Loading function word list from " << fileName;
  Moses::InputFileStream inFile(fileName);
  if (inFile.fail()) {
    std::cerr << " - ERROR: could not open file" << std::endl;
    exit(1);
  }

  std::string line;
  while(getline(inFile, line)) {
    std::vector<std::string> token;
    Moses::Tokenize( token, line );
    if (token.size() > 0)
      functionWordList.insert( token[0] );
  }

  std::cerr << " - read " << functionWordList.size() << " function words" << std::endl;
  inFile.Close();
}


double computeLexicalTranslation( const PHRASE *phraseSource, const PHRASE *phraseTarget, const ALIGNMENT *alignmentTargetToSource )
{
  // lexical translation probability
  double lexScore = 1.0;
  int null = vcbS.getWordID("NULL");
  // all target words have to be explained
  for(size_t ti=0; ti<alignmentTargetToSource->size(); ti++) {
    const std::set< size_t > & srcIndices = alignmentTargetToSource->at(ti);
    if (srcIndices.empty()) {
      // explain unaligned word by NULL
      lexScore *= lexTable.permissiveLookup( null, phraseTarget->at(ti) );
    } else {
      // go through all the aligned words to compute average
      double thisWordScore = 0;
      for (std::set< size_t >::const_iterator p(srcIndices.begin()); p != srcIndices.end(); ++p) {
        thisWordScore += lexTable.permissiveLookup( phraseSource->at(*p), phraseTarget->at(ti) );
      }
      lexScore *= thisWordScore / (double)srcIndices.size();
    }
  }
  return lexScore;
}


void LexicalTable::load( const std::string &fileName )
{
  std::cerr << "Loading lexical translation table from " << fileName;
  Moses::InputFileStream inFile(fileName);
  if (inFile.fail()) {
    std::cerr << " - ERROR: could not open file" << std::endl;
    exit(1);
  }

  std::string line;
  int i=0;
  while(getline(inFile, line)) {
    i++;
    if (i%100000 == 0) std::cerr << "." << std::flush;

    std::vector<std::string> token;
    Moses::Tokenize( token, line );
    if (token.size() != 3) {
      std::cerr << "line " << i << " in " << fileName
                << " has wrong number of tokens, skipping:" << std::endl
                << token.size() << " " << token[0] << " " << line << std::endl;
      continue;
    }

    double prob = std::atof( token[2].c_str() );
    WORD_ID wordT = vcbT.storeIfNew( token[0] );
    WORD_ID wordS = vcbS.storeIfNew( token[1] );
    ltable[ wordS ][ wordT ] = prob;
  }
  std::cerr << std::endl;
}


void printSourcePhrase(const PHRASE *phraseSource, const PHRASE *phraseTarget,
                       const ALIGNMENT *targetToSourceAlignment, std::ostream &out)
{
  // get corresponding target non-terminal and output pair
  ALIGNMENT *sourceToTargetAlignment = new ALIGNMENT();
  invertAlignment(phraseSource, phraseTarget, targetToSourceAlignment, sourceToTargetAlignment);
  // output source symbols, except root, in rule table format
  for (std::size_t i = 0; i < phraseSource->size()-1; ++i) {
    const std::string &word = vcbS.getWord(phraseSource->at(i));
    if (!unpairedExtractFormatFlag || !isNonTerminal(word)) {
      out << word << " ";
      continue;
    }
    const std::set<std::size_t> &alignmentPoints = sourceToTargetAlignment->at(i);
    assert(alignmentPoints.size() == 1);
    size_t j = *(alignmentPoints.begin());
    if (inverseFlag) {
      out << vcbT.getWord(phraseTarget->at(j)) << word << " ";
    } else {
      out << word << vcbT.getWord(phraseTarget->at(j)) << " ";
    }
  }
  // output source root symbol
  if (conditionOnTargetLhsFlag && !inverseFlag) {
    out << "[X]";
  } else {
    out << vcbS.getWord(phraseSource->back());
  }
  delete sourceToTargetAlignment;
}


void printTargetPhrase(const PHRASE *phraseSource, const PHRASE *phraseTarget,
                       const ALIGNMENT *targetToSourceAlignment, std::ostream &out)
{
  // output target symbols, except root, in rule table format
  for (std::size_t i = 0; i < phraseTarget->size()-1; ++i) {
    const std::string &word = vcbT.getWord(phraseTarget->at(i));
    if (!unpairedExtractFormatFlag || !isNonTerminal(word)) {
      out << word << " ";
      continue;
    }
    // get corresponding source non-terminal and output pair
    std::set<std::size_t> alignmentPoints = targetToSourceAlignment->at(i);
    assert(alignmentPoints.size() == 1);
    int j = *(alignmentPoints.begin());
    if (inverseFlag) {
      out << word << vcbS.getWord(phraseSource->at(j)) << " ";
    } else {
      out << vcbS.getWord(phraseSource->at(j)) << word << " ";
    }
  }
  // output target root symbol
  if (conditionOnTargetLhsFlag) {
    if (inverseFlag) {
      out << "[X]";
    } else {
      out << vcbS.getWord(phraseSource->back());
    }
  } else {
    out << vcbT.getWord(phraseTarget->back());
  }
}


void invertAlignment(const PHRASE *phraseSource, const PHRASE *phraseTarget,
                     const ALIGNMENT *inTargetToSourceAlignment, ALIGNMENT *outSourceToTargetAlignment)
{
// typedef std::vector< std::set<size_t> > ALIGNMENT;

  outSourceToTargetAlignment->clear();
  size_t numberOfSourceSymbols = (hierarchicalFlag ? phraseSource->size()-1 : phraseSource->size());
  outSourceToTargetAlignment->resize(numberOfSourceSymbols);
  // add alignment point
  for (size_t targetPosition = 0; targetPosition < inTargetToSourceAlignment->size(); ++targetPosition) {
    for ( std::set<size_t>::iterator setIter = (inTargetToSourceAlignment->at(targetPosition)).begin();
          setIter != (inTargetToSourceAlignment->at(targetPosition)).end(); ++setIter ) {
      size_t sourcePosition = *setIter;
      outSourceToTargetAlignment->at(sourcePosition).insert(targetPosition);
    }
  }
}

