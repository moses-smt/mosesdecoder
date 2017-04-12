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

#include <algorithm>
#include <assert.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <limits>

#ifdef WIN32
// Include Visual Leak Detector
//#include <vld.h>
#endif

#include "ExtractedRule.h"
#include "Hole.h"
#include "HoleCollection.h"
#include "RuleExist.h"
#include "SentenceAlignmentWithSyntax.h"
#include "SyntaxNode.h"
#include "tables-core.h"
#include "XmlTree.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "PhraseOrientation.h"

using namespace std;
using namespace MosesTraining;

typedef vector< int > LabelIndex;
typedef map< int, int > WordIndex;

class ExtractTask
{
private:
  SentenceAlignmentWithSyntax &m_sentence;
  const RuleExtractionOptions &m_options;
  Moses::OutputFileStream& m_extractFile;
  Moses::OutputFileStream& m_extractFileInv;
  Moses::OutputFileStream& m_extractFileContext;
  Moses::OutputFileStream& m_extractFileContextInv;
  PhraseOrientation m_phraseOrientation;

  vector< ExtractedRule > m_extractedRules;

  // main functions
  void extractRules();
  void addRuleToCollection(ExtractedRule &rule);
  void consolidateRules();
  void writeRulesToFile();

  // subs
  void addRule( int, int, int, int, int, RuleExist &ruleExist);
  void addHieroRule( int startT, int endT, int startS, int endS
                     , RuleExist &ruleExist, HoleCollection &holeColl, int numHoles, int initStartF, int wordCountT, int wordCountS);
  void saveHieroPhrase( int startT, int endT, int startS, int endS
                        , HoleCollection &holeColl, LabelIndex &labelIndex, int countS);
  string saveTargetHieroPhrase( int startT, int endT, int startS, int endS
                                , WordIndex &indexT, HoleCollection &holeColl, const LabelIndex &labelIndex, double &logPCFGScore, int countS);
  string saveSourceHieroPhrase( int startT, int endT, int startS, int endS
                                , HoleCollection &holeColl, const LabelIndex &labelIndex);
  void preprocessSourceHieroPhrase( int startT, int endT, int startS, int endS
                                    , WordIndex &indexS, HoleCollection &holeColl, const LabelIndex &labelIndex);
  void saveHieroAlignment(  int startT, int endT, int startS, int endS
                            , const WordIndex &indexS, const WordIndex &indexT, HoleCollection &holeColl, ExtractedRule &rule);
  void saveTargetSyntacticPreference( const HoleCollection &holeColl, const LabelIndex &labelIndex, ExtractedRule &rule);
  void saveAllHieroPhrases( int startT, int endT, int startS, int endS, HoleCollection &holeColl, int countS);

  inline string IntToString( int i ) {
    stringstream out;
    out << i;
    return out.str();
  }

public:
  ExtractTask(SentenceAlignmentWithSyntax &sentence, const RuleExtractionOptions &options, Moses::OutputFileStream &extractFile, Moses::OutputFileStream &extractFileInv, Moses::OutputFileStream &extractFileContext, Moses::OutputFileStream &extractFileContextInv):
    m_sentence(sentence),
    m_options(options),
    m_extractFile(extractFile),
    m_extractFileInv(extractFileInv),
    m_extractFileContext(extractFileContext),
    m_extractFileContextInv(extractFileContextInv) {}
  void Run();

};

// stats for glue grammar and unknown word label probabilities
void collectWordLabelCounts(SentenceAlignmentWithSyntax &sentence );
void writeGlueGrammar(const string &, RuleExtractionOptions &options, set< string > &targetLabelCollection, map< string, int > &targetTopLabelCollection);
void writeUnknownWordLabel(const string &);
void writePhraseOrientationPriors(const string &);

double getPcfgScore(const SyntaxNode &);


int main(int argc, char* argv[])
{
  cerr << "extract-rules, written by Philipp Koehn\n"
       << "rule extraction from an aligned parallel corpus\n";

  RuleExtractionOptions options;
  int sentenceOffset = 0;
#ifdef WITH_THREADS
  int thread_count = 1;
#endif
  if (argc < 5) {
    cerr << "syntax: extract-rules corpus.target corpus.source corpus.align extract ["

         << " --GlueGrammar FILE"
         << " | --UnknownWordLabel FILE"
         << " | --OnlyDirect"
         << " | --MaxSpan[" << options.maxSpan << "]"
         << " | --MinHoleTarget[" << options.minHoleTarget << "]"
         << " | --MinHoleSource[" << options.minHoleSource << "]"
         << " | --MinWords[" << options.minWords << "]"
         << " | --MaxSymbolsTarget[" << options.maxSymbolsTarget << "]"
         << " | --MaxSymbolsSource[" << options.maxSymbolsSource << "]"
         << " | --MaxNonTerm[" << options.maxNonTerm << "]"
         << " | --MaxScope[" << options.maxScope << "]"
         << " | --SourceSyntax | --TargetSyntax"
         << " | --AllowOnlyUnalignedWords | --DisallowNonTermConsecTarget |--NonTermConsecSource |  --NoNonTermFirstWord | --NoFractionalCounting"
         << " | --UnpairedExtractFormat"
         << " | --ConditionOnTargetLHS ]"
         << " | --BoundaryRules[" << options.boundaryRules << "]"
         << " | --FlexibilityScore"
         << " | --PhraseOrientation\n";

    exit(1);
  }
  char* &fileNameT = argv[1];
  char* &fileNameS = argv[2];
  char* &fileNameA = argv[3];
  string fileNameGlueGrammar;
  string fileNameUnknownWordLabel;
  string fileNameExtract = string(argv[4]);

  int optionInd = 5;

  for(int i=optionInd; i<argc; i++) {
    // maximum span length
    if (strcmp(argv[i],"--MaxSpan") == 0) {
      options.maxSpan = atoi(argv[++i]);
      if (options.maxSpan < 1) {
        cerr << "extract error: --maxSpan should be at least 1" << endl;
        exit(1);
      }
    } else if (strcmp(argv[i],"--MinHoleTarget") == 0) {
      options.minHoleTarget = atoi(argv[++i]);
      if (options.minHoleTarget < 1) {
        cerr << "extract error: --minHoleTarget should be at least 1" << endl;
        exit(1);
      }
    } else if (strcmp(argv[i],"--MinHoleSource") == 0) {
      options.minHoleSource = atoi(argv[++i]);
      if (options.minHoleSource < 1) {
        cerr << "extract error: --minHoleSource should be at least 1" << endl;
        exit(1);
      }
    }
    // maximum number of words in hierarchical phrase
    else if (strcmp(argv[i],"--MaxSymbolsTarget") == 0) {
      options.maxSymbolsTarget = atoi(argv[++i]);
      if (options.maxSymbolsTarget < 1) {
        cerr << "extract error: --MaxSymbolsTarget should be at least 1" << endl;
        exit(1);
      }
    }
    // maximum number of words in hierarchical phrase
    else if (strcmp(argv[i],"--MaxSymbolsSource") == 0) {
      options.maxSymbolsSource = atoi(argv[++i]);
      if (options.maxSymbolsSource < 1) {
        cerr << "extract error: --MaxSymbolsSource should be at least 1" << endl;
        exit(1);
      }
    }
    // minimum number of words in hierarchical phrase
    else if (strcmp(argv[i],"--MinWords") == 0) {
      options.minWords = atoi(argv[++i]);
      if (options.minWords < 0) {
        cerr << "extract error: --MinWords should be at least 0" << endl;
        exit(1);
      }
    }
    // maximum number of non-terminals
    else if (strcmp(argv[i],"--MaxNonTerm") == 0) {
      options.maxNonTerm = atoi(argv[++i]);
      if (options.maxNonTerm < 1) {
        cerr << "extract error: --MaxNonTerm should be at least 1" << endl;
        exit(1);
      }
    }
    // maximum scope (see Hopkins and Langmead (2010))
    else if (strcmp(argv[i],"--MaxScope") == 0) {
      options.maxScope = atoi(argv[++i]);
      if (options.maxScope < 0) {
        cerr << "extract error: --MaxScope should be at least 0" << endl;
        exit(1);
      }
    } else if (strcmp(argv[i], "--GZOutput") == 0) {
      options.gzOutput = true;
    }
    // allow consecutive non-terminals (X Y | X Y)
    else if (strcmp(argv[i],"--TargetSyntax") == 0) {
      options.targetSyntax = true;
    } else if (strcmp(argv[i],"--TargetSyntacticPreferences") == 0) {
      options.targetSyntacticPreferences = true;
    } else if (strcmp(argv[i],"--SourceSyntax") == 0) {
      options.sourceSyntax = true;
    } else if (strcmp(argv[i],"--AllowOnlyUnalignedWords") == 0) {
      options.requireAlignedWord = false;
    } else if (strcmp(argv[i],"--DisallowNonTermConsecTarget") == 0) {
      options.nonTermConsecTarget = false;
    } else if (strcmp(argv[i],"--NonTermConsecSource") == 0) {
      options.nonTermConsecSource = true;
    } else if (strcmp(argv[i],"--NoNonTermFirstWord") == 0) {
      options.nonTermFirstWord = false;
    } else if (strcmp(argv[i],"--OnlyOutputSpanInfo") == 0) {
      options.onlyOutputSpanInfo = true;
    } else if (strcmp(argv[i],"--OnlyDirect") == 0) {
      options.onlyDirectFlag = true;
    } else if (strcmp(argv[i],"--GlueGrammar") == 0) {
      options.glueGrammarFlag = true;
      if (++i >= argc) {
        cerr << "ERROR: Option --GlueGrammar requires a file name" << endl;
        exit(0);
      }
      fileNameGlueGrammar = string(argv[i]);
      cerr << "creating glue grammar in '" << fileNameGlueGrammar << "'" << endl;
    } else if (strcmp(argv[i],"--UnknownWordLabel") == 0) {
      options.unknownWordLabelFlag = true;
      if (++i >= argc) {
        cerr << "ERROR: Option --UnknownWordLabel requires a file name" << endl;
        exit(0);
      }
      fileNameUnknownWordLabel = string(argv[i]);
      cerr << "creating unknown word labels in '" << fileNameUnknownWordLabel << "'" << endl;
    }
    // TODO: this should be a useful option
    //else if (strcmp(argv[i],"--ZipFiles") == 0) {
    //  zipFiles = true;
    //}
    // if an source phrase is paired with two target phrases, then count(t|s) = 0.5
    else if (strcmp(argv[i],"--NoFractionalCounting") == 0) {
      options.fractionalCounting = false;
    } else if (strcmp(argv[i],"--PCFG") == 0) {
      options.pcfgScore = true;
    } else if (strcmp(argv[i],"--UnpairedExtractFormat") == 0) {
      options.unpairedExtractFormat = true;
    } else if (strcmp(argv[i],"--ConditionOnTargetLHS") == 0) {
      options.conditionOnTargetLhs = true;
    } else if (strcmp(argv[i],"--FlexibilityScore") == 0) {
      options.flexScoreFlag = true;
    } else if (strcmp(argv[i],"--PhraseOrientation") == 0) {
      options.phraseOrientation = true;
    } else if (strcmp(argv[i],"-threads") == 0 ||
               strcmp(argv[i],"--threads") == 0 ||
               strcmp(argv[i],"--Threads") == 0) {
#ifdef WITH_THREADS
      thread_count = atoi(argv[++i]);
#else
      cerr << "thread support not compiled in." << '\n';
      exit(1);
#endif
    } else if (strcmp(argv[i], "--SentenceOffset") == 0) {
      if (i+1 >= argc || argv[i+1][0] < '0' || argv[i+1][0] > '9') {
        cerr << "extract: syntax error, used switch --SentenceOffset without a number" << endl;
        exit(1);
      }
      sentenceOffset = atoi(argv[++i]);
    } else if (strcmp(argv[i],"--BoundaryRules") == 0) {
      options.boundaryRules = true;
    } else {
      cerr << "extract: syntax error, unknown option '" << string(argv[i]) << "'\n";
      exit(1);
    }
  }

  cerr << "extracting hierarchical rules" << endl;

  // open input files
  Moses::InputFileStream tFile(fileNameT);
  Moses::InputFileStream sFile(fileNameS);
  Moses::InputFileStream aFile(fileNameA);

  istream *tFileP = &tFile;
  istream *sFileP = &sFile;
  istream *aFileP = &aFile;

  // open output files
  string fileNameExtractInv = fileNameExtract + ".inv" + (options.gzOutput?".gz":"");
  Moses::OutputFileStream extractFile;
  Moses::OutputFileStream extractFileInv;
  Moses::OutputFileStream extractFileContext;
  Moses::OutputFileStream extractFileContextInv;
  extractFile.Open((fileNameExtract  + (options.gzOutput?".gz":"")).c_str());
  if (!options.onlyDirectFlag)
    extractFileInv.Open(fileNameExtractInv.c_str());

  if (options.flexScoreFlag) {
    string fileNameExtractContext = fileNameExtract + ".context" + (options.gzOutput?".gz":"");
    extractFileContext.Open(fileNameExtractContext.c_str());
    if (!options.onlyDirectFlag) {
      string fileNameExtractContextInv = fileNameExtract + ".context.inv" + (options.gzOutput?".gz":"");
      extractFileContextInv.Open(fileNameExtractContextInv.c_str());
    }
  }

  // stats on labels for glue grammar and unknown word label probabilities
  set< string > targetLabelCollection, sourceLabelCollection;
  map< string, int > targetTopLabelCollection, sourceTopLabelCollection;

  // loop through all sentence pairs
  size_t i=sentenceOffset;
  string targetString, sourceString, alignmentString;

  while(getline(*tFileP, targetString)) {
    i++;

    getline(*sFileP, sourceString);
    getline(*aFileP, alignmentString);

    if (i%1000 == 0) cerr << i << " " << flush;

    SentenceAlignmentWithSyntax sentence
    (targetLabelCollection, sourceLabelCollection,
     targetTopLabelCollection, sourceTopLabelCollection,
     options.targetSyntax, options.sourceSyntax);
    //az: output src, tgt, and alingment line
    if (options.onlyOutputSpanInfo) {
      cout << "LOG: SRC: " << sourceString << endl;
      cout << "LOG: TGT: " << targetString << endl;
      cout << "LOG: ALT: " << alignmentString << endl;
      cout << "LOG: PHRASES_BEGIN:" << endl;
    }

    if (sentence.create(targetString.c_str(), sourceString.c_str(), alignmentString.c_str(),"", i, options.boundaryRules)) {
      if (options.unknownWordLabelFlag) {
        collectWordLabelCounts(sentence);
      }
      ExtractTask *task = new ExtractTask(sentence, options, extractFile, extractFileInv, extractFileContext, extractFileContextInv);
      task->Run();
      delete task;
    }
    if (options.onlyOutputSpanInfo) cout << "LOG: PHRASES_END:" << endl; //az: mark end of phrases
  }

  tFile.Close();
  sFile.Close();
  aFile.Close();
  // only close if we actually opened it
  if (!options.onlyOutputSpanInfo) {
    extractFile.Close();
    if (!options.onlyDirectFlag) extractFileInv.Close();
  }

  if (options.flexScoreFlag) {
    extractFileContext.Close();
    if (!options.onlyDirectFlag) extractFileContextInv.Close();
  }

  if (options.glueGrammarFlag)
    writeGlueGrammar(fileNameGlueGrammar, options, targetLabelCollection, targetTopLabelCollection);

  if (options.unknownWordLabelFlag)
    writeUnknownWordLabel(fileNameUnknownWordLabel);

  if (options.phraseOrientation) {
    std::string fileNamePhraseOrientationPriors = fileNameExtract + string(".phraseOrientationPriors");
    writePhraseOrientationPriors(fileNamePhraseOrientationPriors);
  }
}

void ExtractTask::Run()
{
  extractRules();
  consolidateRules();
  writeRulesToFile();
  m_extractedRules.clear();
}

void ExtractTask::extractRules()
{
  int countT = m_sentence.target.size();
  int countS = m_sentence.source.size();

  // initialize phrase orientation scoring object (for lexicalized reordering model)
  if (m_options.phraseOrientation) {
    m_sentence.invertAlignment(); // fill m_sentence.alignedToS
    m_phraseOrientation = PhraseOrientation(countS, countT, m_sentence.alignedToT, m_sentence.alignedToS, m_sentence.alignedCountS);
  }

  // phrase repository for creating hiero phrases
  RuleExist ruleExist(countT);

  // check alignments for target phrase startT...endT
  for(int lengthT=1;
      lengthT <= m_options.maxSpan && lengthT <= countT;
      lengthT++) {
    for(int startT=0; startT < countT-(lengthT-1); startT++) {

      // that's nice to have
      int endT = startT + lengthT - 1;

      // if there is target side syntax, there has to be a node
      if (m_options.targetSyntax && !m_options.targetSyntacticPreferences && !m_sentence.targetTree.HasNode(startT,endT))
//      if (m_options.targetSyntax && !m_sentence.targetTree.HasNode(startT,endT))
        continue;

      // find find aligned source words
      // first: find minimum and maximum source word
      int minS = std::numeric_limits<int>::max();
      int maxS = -1;
      vector< int > usedS = m_sentence.alignedCountS;
      for(int ti=startT; ti<=endT; ti++) {
        for(unsigned int i=0; i<m_sentence.alignedToT[ti].size(); i++) {
          int si = m_sentence.alignedToT[ti][i];
          if (si<minS) {
            minS = si;
          }
          if (si>maxS) {
            maxS = si;
          }
          usedS[ si ]--;
        }
      }

      // unaligned phrases are not allowed
      if( maxS == -1 )
        continue;

      // source phrase has to be within limits
      if( maxS-minS >= m_options.maxSpan )
        continue;

      // check if source words are aligned to out of bound target words
      bool out_of_bounds = false;
      for(int si=minS; si<=maxS && !out_of_bounds; si++)
        if (usedS[si]>0) {
          out_of_bounds = true;
        }

      // if out of bound, you gotta go
      if (out_of_bounds)
        continue;

      // done with all the checks, lets go over all consistent phrase pairs
      // start point of source phrase may retreat over unaligned
      for(int startS=minS;
          (startS>=0 &&
           startS>maxS - m_options.maxSpan && // within length limit
           (startS==minS || m_sentence.alignedCountS[startS]==0)); // unaligned
          startS--) {
        // end point of source phrase may advance over unaligned
        for(int endS=maxS;
            (endS<countS && endS<startS + m_options.maxSpan && // within length limit
             (endS==maxS || m_sentence.alignedCountS[endS]==0)); // unaligned
            endS++) {
          // if there is source side syntax, there has to be a node
          if (m_options.sourceSyntax && !m_sentence.sourceTree.HasNode(startS,endS))
            continue;

          // TODO: loop over all source and target syntax labels

          // if within length limits, add as fully-lexical phrase pair
          if (endT-startT < m_options.maxSymbolsTarget && endS-startS < m_options.maxSymbolsSource) {
            addRule(startT,endT,startS,endS, countS, ruleExist);
          }

          // take note that this is a valid phrase alignment
          ruleExist.Add(startT, endT, startS, endS);

          // extract hierarchical rules

          // are rules not allowed to start non-terminals?
          int initStartT = m_options.nonTermFirstWord ? startT : startT + 1;

          HoleCollection holeColl(startS, endS); // empty hole collection
          addHieroRule(startT, endT, startS, endS,
                       ruleExist, holeColl, 0, initStartT,
                       endT-startT+1, endS-startS+1);
        }
      }
    }
  }
}

void ExtractTask::preprocessSourceHieroPhrase( int startT, int endT, int startS, int endS
    , WordIndex &indexS, HoleCollection &holeColl, const LabelIndex &labelIndex)
{
  vector<Hole*>::iterator iterHoleList = holeColl.GetSortedSourceHoles().begin();
  assert(iterHoleList != holeColl.GetSortedSourceHoles().end());

  int outPos = 0;
  int holeCount = 0;
  int holeTotal = holeColl.GetHoles().size();
  for(int currPos = startS; currPos <= endS; currPos++) {
    bool isHole = false;
    if (iterHoleList != holeColl.GetSortedSourceHoles().end()) {
      const Hole &hole = **iterHoleList;
      isHole = hole.GetStart(0) == currPos;
    }

    if (isHole) {
      Hole &hole = **iterHoleList;

      int labelI = labelIndex[ 2+holeCount+holeTotal ];
      string label = m_options.sourceSyntax ?
                     m_sentence.sourceTree.GetNodes(currPos,hole.GetEnd(0))[ labelI ]->label : "X";
      hole.SetLabel(label, 0);

      currPos = hole.GetEnd(0);
      hole.SetPos(outPos, 0);
      ++iterHoleList;
      ++holeCount;
    } else {
      indexS[currPos] = outPos;
    }

    outPos++;
  }

  assert(iterHoleList == holeColl.GetSortedSourceHoles().end());
}

string ExtractTask::saveTargetHieroPhrase( int startT, int endT, int startS, int endS
    , WordIndex &indexT, HoleCollection &holeColl, const LabelIndex &labelIndex, double &logPCFGScore
    , int countS)
{
  HoleList::iterator iterHoleList = holeColl.GetHoles().begin();
  assert(iterHoleList != holeColl.GetHoles().end());

  string out = "";
  int outPos = 0;
  int holeCount = 0;
  for(int currPos = startT; currPos <= endT; currPos++) {
    bool isHole = false;
    if (iterHoleList != holeColl.GetHoles().end()) {
      const Hole &hole = *iterHoleList;
      isHole = hole.GetStart(1) == currPos;
    }

    if (isHole) {
      Hole &hole = *iterHoleList;

      const string &sourceLabel = hole.GetLabel(0);
      assert(sourceLabel != "");

      int labelI = labelIndex[ 2+holeCount ];
      string targetLabel;
      if (m_options.targetSyntax && !m_options.targetSyntacticPreferences) {
        targetLabel = m_sentence.targetTree.GetNodes(currPos,hole.GetEnd(1))[labelI]->label;
      } else if (m_options.boundaryRules && (startS == 0 || endS == countS - 1)) {
        targetLabel = "S";
      } else {
        targetLabel = "X";
      }

      hole.SetLabel(targetLabel, 1);

      if (m_options.unpairedExtractFormat) {
        out += "[" + targetLabel + "] ";
      } else {
        out += "[" + sourceLabel + "][" + targetLabel + "] ";
      }

      if (m_options.pcfgScore) {
        logPCFGScore -= getPcfgScore(*m_sentence.targetTree.GetNodes(currPos,hole.GetEnd(1))[labelI]);
      }

      currPos = hole.GetEnd(1);
      hole.SetPos(outPos, 1);
      ++iterHoleList;
      holeCount++;
    } else {
      indexT[currPos] = outPos;
      out += m_sentence.target[currPos] + " ";
    }

    outPos++;
  }

  assert(iterHoleList == holeColl.GetHoles().end());
  return out.erase(out.size()-1);
}

string ExtractTask::saveSourceHieroPhrase( int startT, int endT, int startS, int endS
    , HoleCollection &holeColl, const LabelIndex &labelIndex)
{
  vector<Hole*>::iterator iterHoleList = holeColl.GetSortedSourceHoles().begin();
  assert(iterHoleList != holeColl.GetSortedSourceHoles().end());

  string out = "";
  int outPos = 0;
  int holeCount = 0;
  for(int currPos = startS; currPos <= endS; currPos++) {
    bool isHole = false;
    if (iterHoleList != holeColl.GetSortedSourceHoles().end()) {
      const Hole &hole = **iterHoleList;
      isHole = hole.GetStart(0) == currPos;
    }

    if (isHole) {
      Hole &hole = **iterHoleList;

      const string &targetLabel = hole.GetLabel(1);
      assert(targetLabel != "");

      const string &sourceLabel =  hole.GetLabel(0);
      if (m_options.unpairedExtractFormat) {
        out += "[" + sourceLabel + "] ";
      } else {
        out += "[" + sourceLabel + "][" + (m_options.targetSyntacticPreferences ? "X" : targetLabel) + "] ";
      }

      currPos = hole.GetEnd(0);
      hole.SetPos(outPos, 0);
      ++iterHoleList;
      ++holeCount;
    } else {
      out += m_sentence.source[currPos] + " ";
    }

    outPos++;
  }

  assert(iterHoleList == holeColl.GetSortedSourceHoles().end());
  return out.erase(out.size()-1);
}

void ExtractTask::saveHieroAlignment( int startT, int endT, int startS, int endS
                                      , const WordIndex &indexS, const WordIndex &indexT, HoleCollection &holeColl, ExtractedRule &rule)
{
  // print alignment of words
  for(int ti=startT; ti<=endT; ti++) {
    WordIndex::const_iterator p = indexT.find(ti);
    if (p != indexT.end()) { // does word still exist?
      for(unsigned int i=0; i<m_sentence.alignedToT[ti].size(); i++) {
        int si = m_sentence.alignedToT[ti][i];
        std::string sourceSymbolIndex = IntToString(indexS.find(si)->second);
        std::string targetSymbolIndex = IntToString(p->second);
        rule.alignment      += sourceSymbolIndex + "-" + targetSymbolIndex + " ";
        if (! m_options.onlyDirectFlag)
          rule.alignmentInv += targetSymbolIndex + "-" + sourceSymbolIndex + " ";
      }
    }
  }

  // print alignment of non terminals
  HoleList::const_iterator iterHole;
  for (iterHole = holeColl.GetHoles().begin(); iterHole != holeColl.GetHoles().end(); ++iterHole) {
    const Hole &hole = *iterHole;

    std::string sourceSymbolIndex = IntToString(hole.GetPos(0));
    std::string targetSymbolIndex = IntToString(hole.GetPos(1));
    rule.alignment      += sourceSymbolIndex + "-" + targetSymbolIndex + " ";
    if (!m_options.onlyDirectFlag)
      rule.alignmentInv += targetSymbolIndex + "-" + sourceSymbolIndex + " ";
  }

  rule.alignment.erase(rule.alignment.size()-1);
  if (!m_options.onlyDirectFlag) {
    rule.alignmentInv.erase(rule.alignmentInv.size()-1);
  }
}

void ExtractTask::saveTargetSyntacticPreference( const HoleCollection &holeColl, const LabelIndex &labelIndex, ExtractedRule &rule)
{
  rule.targetSyntacticPreference = "";
  int holeCount = 0;
  for (HoleList::const_iterator iterHoleList = holeColl.GetHoles().begin();
       iterHoleList != holeColl.GetHoles().end();
       ++iterHoleList) {

    const Hole &hole = *iterHoleList;

    int labelI = labelIndex[ 2+holeCount ];
    int startT = hole.GetStart(1);
    int endT = hole.GetEnd(1);
    if (m_sentence.targetTree.HasNode(startT,endT)) {
      rule.targetSyntacticPreference += m_sentence.targetTree.GetNodes(startT,endT)[labelI]->label;
      rule.targetSyntacticPreference += " ";
    } else {
      rule.targetSyntacticPreference += "XRHS ";
    }
    ++holeCount;
  }

  rule.targetSyntacticPreference.erase(rule.targetSyntacticPreference.size()-1);
}


void ExtractTask::saveHieroPhrase( int startT, int endT, int startS, int endS
                                   , HoleCollection &holeColl, LabelIndex &labelIndex, int countS)
{
  WordIndex indexS, indexT; // to keep track of word positions in rule

  ExtractedRule rule( startT, endT, startS, endS );

  // phrase labels
  string targetLabel;
//  if (m_options.targetSyntax && m_sentence.targetTree.HasNode(startT,endT)) {
  if (m_options.targetSyntax && !m_options.targetSyntacticPreferences) {
    targetLabel = m_sentence.targetTree.GetNodes(startT,endT)[labelIndex[0] ]->label;
  } else if (m_options.boundaryRules && (startS == 0 || endS == countS - 1)) {
    targetLabel = "S";
  } else {
    targetLabel = "X";
  }

  string sourceLabel = m_options.sourceSyntax ?
                       m_sentence.sourceTree.GetNodes(startS,endS)[ labelIndex[1] ]->label : "X";

  // create non-terms on the source side
  preprocessSourceHieroPhrase(startT, endT, startS, endS, indexS, holeColl, labelIndex);

  // target
  if (m_options.pcfgScore) {
    double logPCFGScore = getPcfgScore(*m_sentence.targetTree.GetNodes(startT,endT)[labelIndex[0]]);
    rule.target = saveTargetHieroPhrase(startT, endT, startS, endS, indexT, holeColl, labelIndex, logPCFGScore, countS)
                  + " [" + targetLabel + "]";
    rule.pcfgScore = std::exp(logPCFGScore);
  } else {
    double logPCFGScore = 0.0f;
    rule.target = saveTargetHieroPhrase(startT, endT, startS, endS, indexT, holeColl, labelIndex, logPCFGScore, countS)
                  + " [" + targetLabel + "]";
  }

  // source
  rule.source = saveSourceHieroPhrase(startT, endT, startS, endS, holeColl, labelIndex);
  if (m_options.conditionOnTargetLhs) {
    rule.source += " [" + targetLabel + "]";
  } else {
    rule.source += " [" + sourceLabel + "]";
  }

  // alignment
  saveHieroAlignment(startT, endT, startS, endS, indexS, indexT, holeColl, rule);

  // context (words to left and right)
  if (m_options.flexScoreFlag) {
    rule.sourceContextLeft = startS == 0 ? "<s>" : m_sentence.source[startS-1];
    rule.sourceContextRight = endS+1 == m_sentence.source.size() ? "<s>" : m_sentence.source[endS+1];
    rule.targetContextLeft = startT == 0 ? "<s>" : m_sentence.target[startT-1];
    rule.targetContextRight = endT+1 == m_sentence.target.size() ? "<s>" : m_sentence.target[endT+1];
    rule.sourceHoleString = "";
    rule.targetHoleString = "";

    HoleList::const_iterator iterHole;
    for (iterHole = holeColl.GetHoles().begin(); iterHole != holeColl.GetHoles().end(); ++iterHole) {
      const Hole &hole = *iterHole;
      rule.sourceHoleString += hole.GetLabel(0) + ": ";

      // rule starts with nonterminal; end of NT is considered left context
      if (hole.GetStart(0) == startS) {
        rule.sourceContextLeft = m_sentence.source[hole.GetEnd(0)];
      }
      // rule ends with nonterminal; start of NT is considered right context
      else if (hole.GetEnd(0) == endS) {
        rule.sourceContextRight = m_sentence.source[hole.GetStart(0)];
      }

      if (hole.GetStart(1) == startT) {
        rule.targetContextLeft = m_sentence.target[hole.GetEnd(1)];
      } else if (hole.GetEnd(1) == endT) {
        rule.targetContextRight = m_sentence.target[hole.GetStart(1)];
      }

      for (int i = hole.GetStart(0); i <= hole.GetEnd(0); ++i) {
        rule.sourceHoleString += m_sentence.source[i] + " ";
      }
      rule.targetHoleString += hole.GetLabel(1) + ": ";
      for (int i = hole.GetStart(1); i <= hole.GetEnd(1); ++i) {
        rule.targetHoleString += m_sentence.target[i] + " ";
      }
    }
  }

  // phrase orientation (lexicalized reordering model)
  if (m_options.phraseOrientation) {
    rule.l2rOrientation = m_phraseOrientation.GetOrientationInfo(startS,endS,PhraseOrientation::REO_DIR_L2R);
    rule.r2lOrientation = m_phraseOrientation.GetOrientationInfo(startS,endS,PhraseOrientation::REO_DIR_R2L);
    // std::cerr << "span " << startS << " " << endS << std::endl;
    // std::cerr << "phraseOrientationL2R " << m_phraseOrientation.GetOrientationInfo(startS,endS,PhraseOrientation::REO_DIR_L2R) << std::endl;
    // std::cerr << "phraseOrientationR2L " << m_phraseOrientation.GetOrientationInfo(startS,endS,PhraseOrientation::REO_DIR_R2L) << std::endl;
  }

  // target syntactic preferences
  if (m_options.targetSyntacticPreferences) {
    saveTargetSyntacticPreference(holeColl, labelIndex, rule);
    if (m_sentence.targetTree.HasNode(startT,endT)) {
      rule.targetSyntacticPreference += " ";
      rule.targetSyntacticPreference += m_sentence.targetTree.GetNodes(startT,endT)[labelIndex[0] ]->label;
    } else {
      rule.targetSyntacticPreference += " XLHS";
    }
  }

  addRuleToCollection( rule );
}

void ExtractTask::saveAllHieroPhrases( int startT, int endT, int startS, int endS, HoleCollection &holeColl, int countS)
{
  LabelIndex labelIndex,labelCount;

  // number of target head labels
  int numLabels = m_options.targetSyntax ? m_sentence.targetTree.GetNodes(startT,endT).size() : 1;
  if (m_options.targetSyntacticPreferences && !numLabels) {
    numLabels++;
  }
  labelCount.push_back(numLabels);
  labelIndex.push_back(0);

  // number of source head labels
  numLabels =  m_options.sourceSyntax ? m_sentence.sourceTree.GetNodes(startS,endS).size() : 1;
  labelCount.push_back(numLabels);
  labelIndex.push_back(0);

  // number of target hole labels
  for( HoleList::const_iterator hole = holeColl.GetHoles().begin();
       hole != holeColl.GetHoles().end(); hole++ ) {
    int numLabels = m_options.targetSyntax ? m_sentence.targetTree.GetNodes(hole->GetStart(1),hole->GetEnd(1)).size() : 1 ;
    if (m_options.targetSyntacticPreferences && !numLabels) {
      numLabels++;
    }
    labelCount.push_back(numLabels);
    labelIndex.push_back(0);
  }

  // number of source hole labels
  holeColl.SortSourceHoles();
  for( vector<Hole*>::iterator i = holeColl.GetSortedSourceHoles().begin();
       i != holeColl.GetSortedSourceHoles().end(); i++ ) {
    const Hole &hole = **i;
    int numLabels =  m_options.sourceSyntax ? m_sentence.sourceTree.GetNodes(hole.GetStart(0),hole.GetEnd(0)).size() : 1 ;
    labelCount.push_back(numLabels);
    labelIndex.push_back(0);
  }

  // loop through the holes
  bool done = false;
  while(!done) {
    saveHieroPhrase( startT, endT, startS, endS, holeColl, labelIndex, countS );
    for(unsigned int i=0; i<labelIndex.size(); i++) {
      labelIndex[i]++;
      if(labelIndex[i] == labelCount[i]) {
        labelIndex[i] = 0;
        if (i == labelIndex.size()-1)
          done = true;
      } else {
        break;
      }
    }
  }
}

// this function is called recursively
// it pokes a new hole into the phrase pair, and then calls itself for more holes
void ExtractTask::addHieroRule( int startT, int endT, int startS, int endS
                                , RuleExist &ruleExist, HoleCollection &holeColl
                                , int numHoles, int initStartT, int wordCountT, int wordCountS)
{
  // done, if already the maximum number of non-terminals in phrase pair
  if (numHoles >= m_options.maxNonTerm)
    return;

  // find a hole...
  for (int startHoleT = initStartT; startHoleT <= endT; ++startHoleT) {
    for (int endHoleT = startHoleT+(m_options.minHoleTarget-1); endHoleT <= endT; ++endHoleT) {
      // if last non-terminal, enforce word count limit
      if (numHoles == m_options.maxNonTerm-1 && wordCountT - (endHoleT-startT+1) + (numHoles+1) > m_options.maxSymbolsTarget)
        continue;

      // determine the number of remaining target words
      const int newWordCountT = wordCountT - (endHoleT-startHoleT+1);

      // always enforce min word count limit
      if (newWordCountT < m_options.minWords)
        continue;

      // except the whole span
      if (startHoleT == startT && endHoleT == endT)
        continue;

      // does a phrase cover this target span?
      // if it does, then there should be a list of mapped source phrases
      // (multiple possible due to unaligned words)
      const HoleList &sourceHoles = ruleExist.GetSourceHoles(startHoleT, endHoleT);

      // loop over sub phrase pairs
      HoleList::const_iterator iterSourceHoles;
      for (iterSourceHoles = sourceHoles.begin(); iterSourceHoles != sourceHoles.end(); ++iterSourceHoles) {
        const Hole &sourceHole = *iterSourceHoles;

        const int sourceHoleSize = sourceHole.GetEnd(0)-sourceHole.GetStart(0)+1;

        // enforce minimum hole size
        if (sourceHoleSize < m_options.minHoleSource)
          continue;

        // determine the number of remaining source words
        const int newWordCountS = wordCountS - sourceHoleSize;

        // if last non-terminal, enforce word count limit
        if (numHoles == m_options.maxNonTerm-1 && newWordCountS + (numHoles+1) > m_options.maxSymbolsSource)
          continue;

        // enforce min word count limit
        if (newWordCountS < m_options.minWords)
          continue;

        // hole must be subphrase of the source phrase
        // (may be violated if subphrase contains additional unaligned source word)
        if (startS > sourceHole.GetStart(0) || endS <  sourceHole.GetEnd(0))
          continue;

        // make sure target side does not overlap with another hole
        if (holeColl.OverlapSource(sourceHole))
          continue;

        // if consecutive non-terminals are not allowed, also check for source
        if (!m_options.nonTermConsecSource && holeColl.ConsecSource(sourceHole) )
          continue;

        // check that rule scope would not exceed limit if sourceHole
        // were added
        if (holeColl.Scope(sourceHole) > m_options.maxScope)
          continue;

        // require that at least one aligned word is left (unless there are no words at all)
        if (m_options.requireAlignedWord && (newWordCountS > 0 || newWordCountT > 0)) {
          HoleList::const_iterator iterHoleList = holeColl.GetHoles().begin();
          bool foundAlignedWord = false;
          // loop through all word positions
          for(int pos = startT; pos <= endT && !foundAlignedWord; pos++) {
            // new hole? moving on...
            if (pos == startHoleT) {
              pos = endHoleT;
            }
            // covered by hole? moving on...
            else if (iterHoleList != holeColl.GetHoles().end() && iterHoleList->GetStart(1) == pos) {
              pos = iterHoleList->GetEnd(1);
              ++iterHoleList;
            }
            // covered by word? check if it is aligned
            else {
              if (m_sentence.alignedToT[pos].size() > 0)
                foundAlignedWord = true;
            }
          }
          if (!foundAlignedWord)
            continue;
        }

        // update list of holes in this phrase pair
        holeColl.Add(startHoleT, endHoleT, sourceHole.GetStart(0), sourceHole.GetEnd(0));
        // now some checks that disallow this phrase pair, but not further recursion
        bool allowablePhrase = true;

        // maximum words count violation?
        if (newWordCountS + (numHoles+1) > m_options.maxSymbolsSource)
          allowablePhrase = false;

        if (newWordCountT + (numHoles+1) > m_options.maxSymbolsTarget)
          allowablePhrase = false;

        // passed all checks...
        if (allowablePhrase)
          saveAllHieroPhrases(startT, endT, startS, endS, holeColl, wordCountS);

        // recursively search for next hole
        int nextInitStartT = m_options.nonTermConsecTarget ? endHoleT + 1 : endHoleT + 2;
        addHieroRule(startT, endT, startS, endS
                     , ruleExist, holeColl, numHoles + 1, nextInitStartT
                     , newWordCountT, newWordCountS);

        holeColl.RemoveLast();
      }
    }
  }
}

void ExtractTask::addRule( int startT, int endT, int startS, int endS, int countS, RuleExist &ruleExist)
{
  // contains only <s> or </s>. Don't output
  if (m_options.boundaryRules
      && (   (startS == 0         && endS == 0)
             || (startS == countS-1  && endS == countS-1))) {
    return;
  }

  if (m_options.onlyOutputSpanInfo) {
    cout << startS << " " << endS << " " << startT << " " << endT << endl;
    return;
  }

  ExtractedRule rule(startT, endT, startS, endS);

  // phrase labels
  string targetLabel,sourceLabel;
  if (m_options.targetSyntax && m_options.conditionOnTargetLhs) {
    if (m_sentence.targetTree.HasNode(startT,endT) && !m_options.targetSyntacticPreferences) {
      sourceLabel = targetLabel = m_sentence.targetTree.GetNodes(startT,endT)[0]->label;
    } else if (m_options.boundaryRules && (startS == 0 || endS == countS - 1)) {
      sourceLabel = "S";
    } else {
      sourceLabel = "X";
    }
  } else {
    sourceLabel = m_options.sourceSyntax ?
                  m_sentence.sourceTree.GetNodes(startS,endS)[0]->label : "X";

    if (m_options.targetSyntax && !m_options.targetSyntacticPreferences) {
//      if (m_options.targetSyntax && !m_options.targetSyntacticPreferences && !m_sentence.targetTree.HasNode(startT,endT))
      targetLabel = m_sentence.targetTree.GetNodes(startT,endT)[0]->label;
    } else if (m_options.boundaryRules && (startS == 0 || endS == countS - 1)) {
      targetLabel = "S";
    } else {
      targetLabel = "X";
    }
  }

  // source
  rule.source = "";
  for(int si=startS; si<=endS; si++)
    rule.source += m_sentence.source[si] + " ";
  rule.source += "[" + sourceLabel + "]";

  // target
  rule.target = "";
  for(int ti=startT; ti<=endT; ti++)
    rule.target += m_sentence.target[ti] + " ";
  rule.target += "[" + targetLabel + "]";

  if (m_options.pcfgScore) {
    double logPCFGScore = getPcfgScore(*m_sentence.targetTree.GetNodes(startT,endT)[0]);
    rule.pcfgScore = std::exp(logPCFGScore);
  }

  // alignment
  for(int ti=startT; ti<=endT; ti++) {
    for(unsigned int i=0; i<m_sentence.alignedToT[ti].size(); i++) {
      int si = m_sentence.alignedToT[ti][i];
      std::string sourceSymbolIndex = IntToString(si-startS);
      std::string targetSymbolIndex = IntToString(ti-startT);
      rule.alignment += sourceSymbolIndex + "-" + targetSymbolIndex + " ";
      if (!m_options.onlyDirectFlag)
        rule.alignmentInv += targetSymbolIndex + "-" + sourceSymbolIndex + " ";
    }
  }

  rule.alignment.erase(rule.alignment.size()-1);
  if (!m_options.onlyDirectFlag)
    rule.alignmentInv.erase(rule.alignmentInv.size()-1);

  // context (words to left and right)
  if (m_options.flexScoreFlag) {
    rule.sourceContextLeft = startS == 0 ? "<s>" : m_sentence.source[startS-1];
    rule.sourceContextRight = endS+1 == m_sentence.source.size() ? "<s>" : m_sentence.source[endS+1];
    rule.targetContextLeft = startT == 0 ? "<s>" : m_sentence.target[startT-1];
    rule.targetContextRight = endT+1 == m_sentence.target.size() ? "<s>" : m_sentence.target[endT+1];
  }

  // phrase orientation (lexicalized reordering model)
  if (m_options.phraseOrientation) {
    rule.l2rOrientation = m_phraseOrientation.GetOrientationInfo(startS,endS,PhraseOrientation::REO_DIR_L2R);
    rule.r2lOrientation = m_phraseOrientation.GetOrientationInfo(startS,endS,PhraseOrientation::REO_DIR_R2L);
    // std::cerr << "span " << startS << " " << endS << std::endl;
    // std::cerr << "phraseOrientationL2R " << m_phraseOrientation.GetOrientationInfo(startS,endS,PhraseOrientation::REO_DIR_L2R) << std::endl;
    // std::cerr << "phraseOrientationR2L " << m_phraseOrientation.GetOrientationInfo(startS,endS,PhraseOrientation::REO_DIR_R2L) << std::endl;
  }

  // target syntactic preferences
  if (m_options.targetSyntacticPreferences) {
    if (m_sentence.targetTree.HasNode(startT,endT)) {
      rule.targetSyntacticPreference += m_sentence.targetTree.GetNodes(startT,endT)[0]->label;
    } else {
      rule.targetSyntacticPreference += "XLHS";
    }
  }

  addRuleToCollection( rule );
}

void ExtractTask::addRuleToCollection( ExtractedRule &newRule )
{

  // no double-counting of identical rules from overlapping spans
  if (!m_options.duplicateRules) {
    vector<ExtractedRule>::const_iterator rule;
    for(rule = m_extractedRules.begin(); rule != m_extractedRules.end(); rule++ ) {
      if (rule->source.compare( newRule.source ) == 0 &&
          rule->target.compare( newRule.target ) == 0 &&
          !(rule->endT < newRule.startT || rule->startT > newRule.endT)) { // overlapping
        return;
      }
    }
  }
  m_extractedRules.push_back( newRule );
}

void ExtractTask::consolidateRules()
{
  typedef vector<ExtractedRule>::iterator R;
  map<int, map<int, map<int, map<int,int> > > > spanCount;

  // compute number of rules per span
  if (m_options.fractionalCounting) {
    for(R rule = m_extractedRules.begin(); rule != m_extractedRules.end(); rule++ ) {
      spanCount[ rule->startT ][ rule->endT ][ rule->startS ][ rule->endS ]++;
    }
  }

  // compute fractional counts
  for(R rule = m_extractedRules.begin(); rule != m_extractedRules.end(); rule++ ) {
    rule->count =    1.0/(float) (m_options.fractionalCounting ? spanCount[ rule->startT ][ rule->endT ][ rule->startS ][ rule->endS ] : 1.0 );
  }

  // consolidate counts
  map<std::string, map< std::string, map< std::string, float> > > consolidatedCount;
  for(R rule = m_extractedRules.begin(); rule != m_extractedRules.end(); rule++ ) {
    consolidatedCount[ rule->source ][ rule->target][ rule->alignment ] += rule->count;
  }

  for(R rule = m_extractedRules.begin(); rule != m_extractedRules.end(); rule++ ) {
    float count = consolidatedCount[ rule->source ][ rule->target][ rule->alignment ];
    rule->count = count;
    consolidatedCount[ rule->source ][ rule->target][ rule->alignment ] = 0;
  }
}

void ExtractTask::writeRulesToFile()
{
  vector<ExtractedRule>::const_iterator rule;
  ostringstream out;
  ostringstream outInv;
  ostringstream outContext;
  ostringstream outContextInv;
  for(rule = m_extractedRules.begin(); rule != m_extractedRules.end(); rule++ ) {
    if (rule->count == 0)
      continue;

    out << rule->source << " ||| "
        << rule->target << " ||| "
        << rule->alignment << " ||| "
        << rule->count << " ||| ";
    if (m_options.pcfgScore) {
      out << " ||| " << rule->pcfgScore;
    }
    if (m_options.phraseOrientation) {
      out << " {{Orientation ";
      m_phraseOrientation.WriteOrientation(out,rule->l2rOrientation);
      out << " ";
      m_phraseOrientation.WriteOrientation(out,rule->r2lOrientation);
      m_phraseOrientation.IncrementPriorCount(PhraseOrientation::REO_DIR_L2R,rule->l2rOrientation,1);
      m_phraseOrientation.IncrementPriorCount(PhraseOrientation::REO_DIR_R2L,rule->r2lOrientation,1);
      out << "}}";
    }
    if (m_options.targetSyntacticPreferences) {
      out << " {{TargetPreferences ";
      out << rule->targetSyntacticPreference;
      out << "}}";
    }
    out << "\n";

    if (!m_options.onlyDirectFlag) {
      outInv << rule->target << " ||| "
             << rule->source << " ||| "
             << rule->alignmentInv << " ||| "
             << rule->count << "\n";
    }

    if (m_options.flexScoreFlag) {
      for(int iContext=0; iContext<2; iContext++) {
        outContext << rule->source << " ||| "
                   << rule->target << " ||| "
                   << rule->alignment << " ||| ";
        iContext ? outContext << "< " << rule->sourceContextLeft << "\n" : outContext << "> " << rule->sourceContextRight << "\n";

        if (!m_options.onlyDirectFlag) {
          outContextInv << rule->target << " ||| "
                        << rule->source << " ||| "
                        << rule->alignmentInv << " ||| ";
          iContext ? outContextInv << "< " << rule->targetContextLeft << "\n" : outContextInv << "> " << rule->targetContextRight << "\n";
        }
      }

      if (rule->sourceHoleString != "") {
        outContext << rule->source << " ||| "
                   << rule->target << " ||| "
                   << rule->alignment << " ||| v "
                   << rule->sourceHoleString << "\n";
      }

      if (!m_options.onlyDirectFlag and rule->targetHoleString != "") {
        outContextInv << rule->target << " ||| "
                      << rule->source << " ||| "
                      << rule->alignmentInv << " ||| v "
                      << rule->targetHoleString << "\n";
      }
    }
  }
  m_extractFile << out.str();
  m_extractFileInv << outInv.str();
  m_extractFileContext << outContext.str();
  m_extractFileContextInv << outContextInv.str();
}

void writeGlueGrammar( const string & fileName, RuleExtractionOptions &options, set< string > &targetLabelCollection, map< string, int > &targetTopLabelCollection )
{
  ofstream grammarFile;
  grammarFile.open(fileName.c_str());

  std::string glueRulesPhraseProperty = "";
  if (options.phraseOrientation) {
    glueRulesPhraseProperty.append(" {{Orientation 1 1 0.5 0.5 1 1 0.5 0.5}}");
  }
  const size_t targetSyntacticPreferencesLabelGlueTop = 0;
  const size_t targetSyntacticPreferencesLabelGlueX = 1;

  if (!options.targetSyntax || options.targetSyntacticPreferences) {
    grammarFile << "<s> [X] ||| <s> [S] ||| 1 ||| 0-0 ||| 0 ||| |||" << glueRulesPhraseProperty;
    if (options.targetSyntacticPreferences) {
      grammarFile << " {{TargetPreferences 1 1 " << targetSyntacticPreferencesLabelGlueTop << " 1}}";
    }
    grammarFile << std::endl;
    grammarFile << "[X][S] </s> [X] ||| [X][S] </s> [S] ||| 1 ||| 0-0 1-1 ||| 0 ||| |||" << glueRulesPhraseProperty;
    if (options.targetSyntacticPreferences) {
      grammarFile << " {{TargetPreferences 2 1 " << targetSyntacticPreferencesLabelGlueTop << " 1 1 " << targetSyntacticPreferencesLabelGlueTop << " 1}}";
    }
    grammarFile << std::endl;
    grammarFile << "[X][S] [X][X] [X] ||| [X][S] [X][X] [S] ||| 2.718 ||| 0-0 1-1 ||| 0 ||| |||" << glueRulesPhraseProperty;
    if (options.targetSyntacticPreferences) {
      grammarFile << " {{TargetPreferences 3 1 " << targetSyntacticPreferencesLabelGlueTop << " " << targetSyntacticPreferencesLabelGlueX << " 1 1 " << targetSyntacticPreferencesLabelGlueTop << " 1}}";
    }
    grammarFile << std::endl;
  } else {
    // choose a top label that is not already a label
    string topLabel = "QQQQQQ";
    for( unsigned int i=1; i<=topLabel.length(); i++) {
      if(targetLabelCollection.find( topLabel.substr(0,i) ) == targetLabelCollection.end() ) {
        topLabel = topLabel.substr(0,i);
        break;
      }
    }
    // basic rules
    grammarFile << "<s> [X] ||| <s> [" << topLabel << "] ||| 1  ||| 0-0" << std::endl
                << "[X][" << topLabel << "] </s> [X] ||| [X][" << topLabel << "] </s> [" << topLabel << "] ||| 1 ||| 0-0 1-1" << std::endl;

    // top rules
    for( map<string,int>::const_iterator i =  targetTopLabelCollection.begin();
         i !=  targetTopLabelCollection.end(); i++ ) {
      grammarFile << "<s> [X][" << i->first << "] </s> [X] ||| <s> [X][" << i->first << "] </s> [" << topLabel << "] ||| 1 ||| 0-0 1-1 2-2" << std::endl;
    }

    // glue rules
    for( set<string>::const_iterator i =  targetLabelCollection.begin();
         i !=  targetLabelCollection.end(); i++ ) {
      grammarFile << "[X][" << topLabel << "] [X][" << *i << "] [X] ||| [X][" << topLabel << "] [X][" << *i << "] [" << topLabel << "] ||| 2.718 ||| 0-0 1-1" << std::endl;
    }
    grammarFile << "[X][" << topLabel << "] [X][X] [X] ||| [X][" << topLabel << "] [X][X] [" << topLabel << "] ||| 2.718 |||  0-0 1-1 " << std::endl; // glue rule for unknown word...
  }
  grammarFile.close();
}

// collect counts for labels for each word
// ( labels of singleton words are used to estimate
//   distribution of labels for unknown words )

map<string,int> wordCount;
map<string,string> wordLabel;
void collectWordLabelCounts( SentenceAlignmentWithSyntax &sentence )
{
  int countT = sentence.target.size();
  for(int ti=0; ti < countT; ti++) {
    string &word = sentence.target[ ti ];
    const vector< SyntaxNode* >& labels = sentence.targetTree.GetNodes(ti,ti);
    if (labels.size() > 0) {
      wordCount[ word ]++;
      wordLabel[ word ] = labels[0]->label;
    }
  }
}

void writeUnknownWordLabel(const string & fileName)
{
  ofstream outFile;
  outFile.open(fileName.c_str());
  typedef map<string,int>::const_iterator I;

  map<string,int> count;
  int total = 0;
  for(I word = wordCount.begin(); word != wordCount.end(); word++) {
    // only consider singletons
    if (word->second == 1) {
      count[ wordLabel[ word->first ] ]++;
      total++;
    }
  }

  for(I pos = count.begin(); pos != count.end(); pos++) {
    double ratio = ((double) pos->second / (double) total);
    if (ratio > 0.03)
      outFile << pos->first << " " << ratio << endl;
  }

  outFile.close();
}

void writePhraseOrientationPriors(const string &fileName)
{
  ofstream outFile;
  outFile.open(fileName.c_str());
  PhraseOrientation::WritePriorCounts(outFile);
  outFile.close();
}

double getPcfgScore(const SyntaxNode &node)
{
  double score = 0.0f;
  SyntaxNode::AttributeMap::const_iterator p = node.attributes.find("pcfg");
  if (p != node.attributes.end()) {
    score = std::atof(p->second.c_str());
  }
  return score;
}
