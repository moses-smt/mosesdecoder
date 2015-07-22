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
#include <boost/locale.hpp>

#include "SafeGetline.h"
#include "SentenceAlignmentMBOT.h"
#include "SyntaxTree.h"
#include "XmlTree.h"
#include "ExtractedRuleMBOT.h"
#include "RuleExistMBOT.h"
#include "HoleMBOT.h"
#include "HoleCollectionMBOT.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "MBOTExtractionOptions.h"
#include "tables-core.h"

#define LINE_MAX_LENGTH 500000

using namespace std;

typedef vector< int > LabelIndex;
typedef map< int, int > WordIndex;
map<string, int> mbotGlues;

class ExtractTask {
private:
  SentenceAlignmentMBOT &m_sentence;
  const MBOTExtractionOptions &m_options;
  OutputFileStream& m_extractFile;
  OutputFileStream& m_extractFileInv;

  vector< ExtractedRuleMBOT > m_extractedRules;

  // main functions
  void extractRules();
  void addRuleToCollection(ExtractedRuleMBOT &rule);
  void consolidateRules();
  void writeRulesToFile(); 

  // subs
  void addInitialRule( int, int, vector<pair<int,int> >, RuleExistMBOT &ruleExist);
  void addExtractableRule( int startS, int endS, vector<pair<int,int> > spansFrags
		     , RuleExistMBOT &ruleExist, const HoleCollectionMBOT &holeColl, int numHoles, int initStartS, int wordCountS, int wordCountT);
  void saveMbotRule( int startS, int endS, vector<pair<int,int> > spansFrags, HoleCollectionMBOT &holeColl, LabelIndex &labelIndex);
  void preprocessSourceFragment( int startS, int endS, vector<pair<int,int> > spansFrags, WordIndex &indexS, HoleCollectionMBOT &holeColl, const LabelIndex &labelIndex);
  void preprocessTargetFragments( int startS, int endS, vector<pair<int,int> > spansFrags, WordIndex &indexS, HoleCollectionMBOT &holeColl, const LabelIndex &labelIndex);
  string generateTargetFragments( int startS, int endS, vector<pair<int,int> > spansFrags, WordIndex &indexT, HoleCollectionMBOT &holeColl, const LabelIndex &labelIndex, double &logPCFGScore);
  string generateSourceFragment( int startS, int endS, vector<pair<int,int> > spansFrags, HoleCollectionMBOT &holeColl, const LabelIndex &labelIndex);
  void saveMbotAlignment( int startS, int endS, vector<pair<int,int> > spansFrags, const WordIndex &indexS, const WordIndex &indexT, HoleCollectionMBOT &holeColl, ExtractedRuleMBOT &rule);
  void saveAllMbotRules( int startS, int endS, vector<pair<int,int> > spansFrags, HoleCollectionMBOT &holeColl);
  void generateMbotGlueRule( string source, vector<string> targets, int size );

  inline string IntToString( int i )
  {
    stringstream out;
    out << i;
    return out.str();
  }

public:
  ExtractTask(SentenceAlignmentMBOT &sentence, const MBOTExtractionOptions &options, OutputFileStream &extractFile, OutputFileStream &extractFileInv):
    m_sentence(sentence),
    m_options(options),
    m_extractFile(extractFile),
  	m_extractFileInv(extractFileInv) {}
  void Run();
};

// stats for glue grammar and unknown word label probabilities
void collectUnknownWordLabelCounts(SentenceAlignmentMBOT &sentence);
void writeGlueGrammar(const string &, MBOTExtractionOptions &options, set< string > &sourceLabelCollection,
		set< string > &targetLabelCollection, map< string,int > &sourceTopLabelCollection, map< string,int > &targetTopLabelCollection);
void writeUnknownWordLabel(const string &);
void writeMbotGrammar(const string &);

int main ( int argc, char *argv[] )
{
  int sentenceOffset = 0;
  MBOTExtractionOptions options;

  if ( argc < 5 ) {
	cerr << "usage: extract-mbot-rules corpus.target corpus.source corpus.align table [ "
       << " --GlueGrammar FILE"
       << " --MbotGrammar FILE"
       << " --UnknownWordLabel FILE"
       << " | --MaxSpan[" << options.maxSpan << "]"
       << " | --MinHoleTarget[" << options.minHoleTarget << "]"
       << " | --MinHoleSource[" << options.minHoleSource << "]"
       << " | --MaxSymbolsTarget[" << options.maxSymbolsTarget << "]"
       << " | --MaxSymbolsSource[" << options.maxSymbolsSource << "]"
       << " | --MinWords[" << options.minWords << "]"
       << " | --MaxNonTerm[" << options.maxNonTerm << "]"
       << " | --SourceSyntax[" << options.sourceSyntax << "]"
       << " | --NoNonTermFirstWord" << options.nonTermFirstWord << "]"
       << " | --AllowOnlyUnalignedWords | --NonTermConsecSource |  --NoNonTermFirstWord"
       << " | --GZOutput[" << options.gzOutput << "]";
  exit(1);
  }

  char* &fileNameT = argv[1];
  char* &fileNameS = argv[2];
  char* &fileNameA = argv[3];
  string fileNameExtract = string(argv[4]);
  string fileNameGlueGrammar;
  string fileNameMbotGrammar;
  string fileNameUnknownWordLabel;

  int optionInd = 5;

  for (int i=optionInd; i<argc; i++) {
    if (strcmp(argv[i],"--MaxSpan") == 0) {
      options.maxSpan = atoi(argv[++i]);
      if (options.maxSpan < 1) {
        cerr << "extract-mbot-rules error: --maxSpan should be at least 1" << endl;
        exit(1);
      }
    } else if (strcmp(argv[i],"--MinHoleTarget") == 0) {
        options.minHoleTarget = atoi(argv[++i]);
        if (options.minHoleTarget < 1) {
          cerr << "extract-mbot-rules error: --minHoleTarget should be at least 1" << endl;
          exit(1);
        }
    } else if (strcmp(argv[i],"--MinHoleSource") == 0) {
        options.minHoleSource = atoi(argv[++i]);
        if (options.minHoleSource < 1) {
          cerr << "extrac-mbot-rulest error: --minHoleSource should be at least 1" << endl;
          exit(1);
        }
    } // maximum number of symbols in (discontiguous) mbot target phrase
    else if (strcmp(argv[i],"--MaxSymbolsTarget") == 0) {
      options.maxSymbolsTarget = atoi(argv[++i]);
      if (options.maxSymbolsTarget < 1) {
        cerr << "extract-mbot-rules error: --MaxSymbolsTarget should be at least 1" << endl;
        exit(1);
      }
    } // maximum number of symbols in contiguous mbot source phrase
    else if (strcmp(argv[i],"--MaxSymbolsSource") == 0) {
      options.maxSymbolsSource = atoi(argv[++i]);
      if (options.maxSymbolsSource < 1) {
        cerr << "extract-mbot-rules error: --MaxSymbolsSource should be at least 1" << endl;
        exit(1);
      }
    } // minimum number of words in contiguous mbot source phrase
    else if (strcmp(argv[i],"--MinWords") == 0) {
      options.minWords = atoi(argv[++i]);
      if (options.minWords < 0) {
        cerr << "extract-mbot-rules error: --MinWords should be at least 0" << endl;
        exit(1);
      }
    } // maximum number of non-terminals in contiguous mbot source phrase
    else if (strcmp(argv[i],"--MaxNonTerm") == 0) {
      options.maxNonTerm = atoi(argv[++i]);
      if (options.maxNonTerm < 1) {
        cerr << "extract-mbot-rules error: --MaxNonTerm should be at least 1" << endl;
        exit(1);
      }
    } else if (strcmp(argv[i],"--AllowOnlyUnalignedWords") == 0) {
    	options.requireAlignedWord = false;
    } else if (strcmp(argv[i],"--NonTermConsecSource") == 0) {
    	options.nonTermConsecSource = true;
    } else if (strcmp(argv[i],"--NoNonTermFirstWord") == 0) {
         options.nonTermFirstWord = false;
    } else if (strcmp(argv[i],"--GlueGrammar") == 0) {
        options.glueGrammarFlag = true;
        if (++i >= argc) {
          cerr << "ERROR: Option --GlueGrammar requires a file name" << endl;
          exit(0);
        }
        fileNameGlueGrammar = string(argv[i]);
        cerr << "creating glue grammar in '" << fileNameGlueGrammar << "'" << endl;
    } else if (strcmp(argv[i],"--MbotGrammar") == 0) {
        options.mbotGrammarFlag = true;
        if (++i >= argc) {
          cerr << "ERROR: Option --MbotGrammar requires a file name" << endl;
          exit(0);
        }
        fileNameMbotGrammar = string(argv[i]);
        cerr << "creating mbot grammar in '" << fileNameMbotGrammar << "'" << endl;
    } else if (strcmp(argv[i],"--UnknownWordLabel") == 0) {
    	options.unknownWordLabelFlag = true;
    	if (++i >= argc) {
    		cerr << "ERROR: Option --UnknownWordLabel requires a file name" << endl;
    		exit(0);
    	}
    	fileNameUnknownWordLabel = string(argv[i]);
        cerr << "creating unknown word labels in '" << fileNameUnknownWordLabel << "'" << endl;
    } else if (strcmp(argv[i],"--GZOutput") == 0) {
        options.gzOutput = true;
    } else if (strcmp(argv[i],"--SourceSyntax") == 0) {
    	options.sourceSyntax = true;
    } else if (strcmp(argv[i], "--SentenceOffset") == 0) {
        if (i+1 >= argc || argv[i+1][0] < '0' || argv[i+1][0] > '9') {
          cerr << "extract-mbot-rules: syntax error, used switch --SentenceOffset without a number" << endl;
          exit(1);
        }
        sentenceOffset = atoi(argv[++i]);
    }
    else {
      cerr << "extract-mbot-rules: syntax error, unknown option '" << string(argv[i]) << "'\n";
      exit(1);
    }
  }

  cerr << "Extracting MBOT rules" << endl;

  // open input files
  InputFileStream tFile(fileNameT);
  InputFileStream sFile(fileNameS);
  InputFileStream aFile(fileNameA);

  istream *tFileP = &tFile;
  istream *sFileP = &sFile;
  istream *aFileP = &aFile;
  
  // open output files
  string fileNameExtractInv = fileNameExtract + ".inv" + (options.gzOutput?".gz":"");
  OutputFileStream extractFile;
  OutputFileStream extractFileInv;
  extractFile.Open((fileNameExtract  + (options.gzOutput?".gz":"")).c_str());
  extractFileInv.Open(fileNameExtractInv.c_str());

  // stats on labels for glue grammar and unknown word label probabilities
  set< string > targetLabelCollection, sourceLabelCollection;
  map< string, int > targetTopLabelCollection, sourceTopLabelCollection;

  // loop through all sentence pairs
  size_t i = sentenceOffset;
  while (true) {
    i++;
    if (i%1000 == 0) cerr << i << " " << flush;
    char targetString[LINE_MAX_LENGTH];
    char sourceString[LINE_MAX_LENGTH];
    char alignmentString[LINE_MAX_LENGTH];
    SAFE_GETLINE((*tFileP), targetString, LINE_MAX_LENGTH, '\n', __FILE__);
    if (tFileP->eof()) break;
    SAFE_GETLINE((*sFileP), sourceString, LINE_MAX_LENGTH, '\n', __FILE__);
    SAFE_GETLINE((*aFileP), alignmentString, LINE_MAX_LENGTH, '\n', __FILE__);

    SentenceAlignmentMBOT sentence(targetLabelCollection, sourceLabelCollection, targetTopLabelCollection, sourceTopLabelCollection, options);
        
    if (sentence.create(targetString, sourceString, alignmentString, i)) {
      if (options.unknownWordLabelFlag)
    	  collectUnknownWordLabelCounts(sentence);

      ExtractTask *task = new ExtractTask(sentence, options, extractFile, extractFileInv);
      task->Run();
      delete task;
    } else {
      cout << "LOG: CREATION FAILED!!!" << endl;
    }
  }

  tFile.Close();
  sFile.Close();
  aFile.Close();

  extractFile.Close();
  extractFileInv.Close();

  if (options.glueGrammarFlag)
    writeGlueGrammar(fileNameGlueGrammar, options, sourceLabelCollection, targetLabelCollection,
    		sourceTopLabelCollection,targetTopLabelCollection);

  if (options.mbotGrammarFlag)
    writeMbotGrammar(fileNameMbotGrammar);

  if (options.unknownWordLabelFlag)
    writeUnknownWordLabel(fileNameUnknownWordLabel);
}  

void ExtractTask::Run() {
  extractRules();
  consolidateRules();
  writeRulesToFile();
  m_extractedRules.clear();
}

void ExtractTask::extractRules() {
  int countS = m_sentence.source.size();
  int countT = m_sentence.target.size();

  // phrase repository for creating mbot rules
  RuleExistMBOT ruleExist(countS);

  // check alignments for source phrase startS...endS
  for (int lengthS = 1; lengthS <= m_options.maxSpan && lengthS <= countS; lengthS++) {
    for (int startS=0; startS < countS-(lengthS-1); startS++) {
      int endS = startS + lengthS - 1;

      // if we're having source side syntax, there has to be a node
      if (m_options.sourceSyntax && !m_sentence.sourceTree.HasNode(startS, endS)) {
        continue;
      }
      // find aligned target words
      // store all current alignments in a vector due to multiple target tree fragments
      int minT = 9999;
      int maxT = -1;
      vector<int> align;
      vector<int> usedT = m_sentence.alignedCountT;
      for (int ti=startS; ti<=endS; ti++) {
        for (unsigned int i=0; i<m_sentence.alignedToS[ti].size(); i++) {
          int si = m_sentence.alignedToS[ti][i];
          align.push_back(si);
          usedT[si]--;
        }
      }

      // unaligned spans are not allowed
      if ( align.empty() )
        continue;

      sort(align.begin(), align.end());
      vector<pair<int,int> > fragmentsSpans;
      int tmp_size = 0;
      int curr_start, curr_end, next_point;
      // one target tree fragment?
      // target syntax, there has to be at least one node
      if (m_sentence.targetTree.HasNode(minT,maxT)) {
        tmp_size += maxT-minT+1;
        fragmentsSpans.push_back(make_pair(minT,maxT));
      } else {
        // multiple target tree fragments!
    	// iterate over alignment points to find...
    	// corresponding nodes in target tree
        for (int i=0; i<align.size(); i++) {
          curr_start = align[i];
          for (int j=i; j<align.size(); j++) {
            int start = align[i];
            int end = align[j];
            if (m_sentence.targetTree.HasNode(start,end)) {
              curr_end = end;
              next_point = j;
            }
          }
          fragmentsSpans.push_back(make_pair(curr_start,curr_end));
          tmp_size += curr_end - curr_start + 1;
          i = next_point;
        }
      }

      // target fragments have to be within limits
      int targetLength = tmp_size;
      if( targetLength >= m_options.maxSpan )
        continue;
      
      // check if target words are aligned to out of bound source words
      bool out_of_bounds = false;
      for (int t=0; t<fragmentsSpans.size() && !out_of_bounds; t++) {
        int minT = fragmentsSpans[t].first;
        int maxT = fragmentsSpans[t].second;
        for (int si=minT; si<=maxT && !out_of_bounds; si++) {
          if (usedT[si]>0)
            out_of_bounds = true;
        }
      }
      // if out of bounds, move on
      if (out_of_bounds)
        continue;
      
      // now loop over possible unaligned words...
      // ...for each target tree fragment!
      // ATTENTION: whenever there are n target fragments, the same rule gets n times extracted...
      // ...only extract once in first pass!
      bool firstPass = true;
      // count subTargetLength over target fragments length + unaligned
      int subTargetLength;
      for (int i=0; i<fragmentsSpans.size(); i++) {
        int minT = fragmentsSpans[i].first;
        int maxT = fragmentsSpans[i].second;

        if (i != 0)
        	firstPass = false;

        // reset subTargetLength to targetLength
        subTargetLength = targetLength;

        for (int startT=minT;
             (startT>=0 && targetLength < m_options.maxSpan && // within length limit
              (startT==minT || m_sentence.alignedCountT[startT]==0)); // unaligned
             startT--) {
        	subTargetLength = subTargetLength + (minT - startT);
          
          for (int endT=maxT;
               (endT<countT && targetLength < m_options.maxSpan && // within length limit
                (endT==maxT || m_sentence.alignedCountT[endT]==0)); // unaligned
               endT++) {

            // if not first pass, do not addInitialRule again!
            if (startT == minT && endT == maxT && !firstPass)
              continue;

            subTargetLength = subTargetLength + (endT - maxT);
            
            // Only consider unaligned words, if span is covered by a node
            if (!m_sentence.targetTree.HasNode(startT,endT))
              continue;
            
            // update fragmentsSpans at current position
            fragmentsSpans[i].first = startT;
            fragmentsSpans[i].second = endT;

            // if within length limits, add as initial rule
            if (endS-startS < m_options.maxSymbolsSource && subTargetLength <= m_options.maxSymbolsTarget) {
              addInitialRule(startS, endS, fragmentsSpans, ruleExist);
            }
            
            // take note that this is a valid phrase alignment
            ruleExist.Add(startS, endS, fragmentsSpans);
            
            // are rules not allowed to start with non-terminals?
            int initStartS = m_options.nonTermFirstWord ? startS : startS + 1;
            
            HoleCollectionMBOT holeColl(startS, endS); // empty hole collection
            addExtractableRule(startS, endS, fragmentsSpans, ruleExist, holeColl, 0, initStartS, endS-startS+1, subTargetLength);
          }
        }
      }
    }
  }
}

void ExtractTask::preprocessTargetFragments( int startS, int endS, vector<pair<int,int> > spansFrags
                                  , WordIndex &indexS, HoleCollectionMBOT &holeColl, const LabelIndex &labelIndex)
{
  HoleList::iterator iterHoleList = holeColl.GetHoles().begin();
  assert(iterHoleList != holeColl.GetHoles().end());

  int numFrags = spansFrags.size();
  int subHoleCount = 0;
  int holeTotal = holeColl.GetHoles().size();

  for (iterHoleList = holeColl.GetHoles().begin(); iterHoleList != holeColl.GetHoles().end(); iterHoleList++) {
	HoleMBOT &hole = *iterHoleList;
	vector<int> hole_startT = hole.GetStartT();
	vector<int> hole_endT = hole.GetEndT();
	for (vector<int>::size_type i=0; i< hole_startT.size(); ++i) {
	  int labelI = labelIndex[ 1+holeTotal+numFrags+subHoleCount ];
	  string targetLabel = m_sentence.targetTree.GetNodes(hole_startT[i],hole_endT[i])[ labelI ]->GetLabel();
	  hole.SetLabelT(targetLabel, i+1);
	  ++subHoleCount;
	}
  }
}

void ExtractTask::preprocessSourceFragment( int startS, int endS, vector<pair<int,int> > spansFrags
                                  , WordIndex &indexS, HoleCollectionMBOT &holeColl, const LabelIndex &labelIndex)
{
  HoleList::iterator iterHoleList = holeColl.GetHoles().begin();
  assert(iterHoleList != holeColl.GetHoles().end());

  int outPos = 0;
  int holeCount = 0;
  int holeTotal = holeColl.GetHoles().size();
  for (int currPos = startS; currPos <= endS; currPos++) {
    bool isHole = false;
    if (iterHoleList != holeColl.GetHoles().end()) {
      const HoleMBOT &hole = *iterHoleList;
      isHole = hole.GetStartS() == currPos;
    }

    if (isHole) {
      HoleMBOT &hole = *iterHoleList;

      int labelI = labelIndex[ 1+holeCount ];
      string label = m_options.sourceSyntax ? m_sentence.sourceTree.GetNodes(currPos,hole.GetEndS())[ labelI ]->GetLabel() : "X";
      hole.SetLabelS(label);

      currPos = hole.GetEndS();
      hole.SetPosS(outPos);
      ++iterHoleList;
      ++holeCount;
    } else {
      indexS[currPos] = outPos;
    }

    outPos++;
  }
  
  assert(iterHoleList == holeColl.GetHoles().end());
}

string ExtractTask::generateTargetFragments(int startS, int endS, vector<pair<int,int> > spansFrags, WordIndex &indexT
					   , HoleCollectionMBOT &holeColl, const LabelIndex &labelIndex, double &logPCFGScore)
{
  vector<HoleMBOT>::iterator iterHoleListSorted = holeColl.GetSortedTargetHoles().begin();
  assert(iterHoleListSorted != holeColl.GetSortedTargetHoles().end());

  string out = "";
  int holeCount = 0;
  int holeTotal = holeColl.GetHoles().size();

  int numFrags = spansFrags.size();
  for (vector<int>::size_type i=0; i<spansFrags.size(); ++i) {
    int startT = spansFrags[i].first;
    int endT = spansFrags[i].second;
    int labelI = labelIndex[ 1+holeTotal+i ];
    string phraseTargetLabel = m_sentence.targetTree.GetNodes(startT,endT)[ labelI ]->GetLabel();

    int outPos = 0;
    for (int currPos = startT; currPos <= endT; currPos++) {
      bool isHole = false;
      if (iterHoleListSorted != holeColl.GetSortedTargetHoles().end()) {
        const HoleMBOT &hole = *iterHoleListSorted;
        isHole = hole.GetStart(1) == currPos;
      }

      if (isHole) {
    	HoleMBOT &hole = *iterHoleListSorted;

    	const string &sourceLabel = hole.GetLabel(0);
    	assert(sourceLabel != "");

    	const string &targetLabel = hole.GetLabel(1);
    	out += "[" + sourceLabel + "]" + "[" + targetLabel + "] ";
    	currPos = hole.GetEnd(1);
    	hole.SetPos(outPos, 1);
    	++iterHoleListSorted;
    	++holeCount;
      } else {
    	indexT[currPos] = outPos;
    	out += m_sentence.target[currPos] + " ";
      }
      
      outPos++;
    }
    out += "[" + phraseTargetLabel + "] ";
    if (numFrags > 1) {
      out += "|| ";
      --numFrags;
    }
  }

  assert(iterHoleListSorted == holeColl.GetSortedTargetHoles().end());
  return out.erase(out.size()-1);
}

string ExtractTask::generateSourceFragment( int startS, int endS, vector<pair<int,int> > spansFrags
                               , HoleCollectionMBOT &holeColl, const LabelIndex &labelIndex)
{
  HoleList::iterator iterHoleList = holeColl.GetHoles().begin();
  assert(iterHoleList != holeColl.GetHoles().end());

  string out = "";
  int outPos = 0;
  int holeCount = 0;
  for(int currPos = startS; currPos <= endS; currPos++) {
    bool isHole = false;
    if (iterHoleList != holeColl.GetHoles().end()) {
      const HoleMBOT &hole = *iterHoleList;
      isHole = hole.GetStartS() == currPos;
    }

    if (isHole) {
      HoleMBOT &hole = *iterHoleList;
      vector<string> targetLabels = hole.GetLabelT();
      assert(!targetLabels.empty());
      const string &sourceLabel =  hole.GetLabelS();
      out += "[" + sourceLabel + "]";
      for (vector<int>::size_type i=0; i<targetLabels.size(); ++i)
    	out += "[" + targetLabels[i] + "]";
      out += " ";
      currPos = hole.GetEndS();
      hole.SetPosS(outPos);
      ++iterHoleList;
      ++holeCount;
    } else {
      out += m_sentence.source[currPos] + " ";
    }

    outPos++;
  }

  assert(iterHoleList == holeColl.GetHoles().end());
  return out.erase(out.size()-1);
}

void ExtractTask::saveMbotAlignment( int startS, int endS, vector<pair<int,int> > spansFrags
                         , const WordIndex &indexS, const WordIndex &indexT, HoleCollectionMBOT &holeColl, ExtractedRuleMBOT &rule)
{
  vector<HoleMBOT>::const_iterator iterHoleListSorted = holeColl.GetSortedTargetHoles().begin();
  int numFrags = spansFrags.size();
  for (vector<int>::size_type i=0; i<spansFrags.size(); ++i) {
    int startT = spansFrags[i].first;
    int endT = spansFrags[i].second;

    // print alignment of words for current span
    for (int ti=startT; ti<=endT; ti++) {
      bool foundWord = false;
      bool foundHole = false;
      WordIndex::const_iterator p = indexT.find(ti);
      if (p != indexT.end()) { // does word still exist?
    	foundWord = true;
    	for (unsigned int i=0; i<m_sentence.alignedToT[ti].size(); i++) {
    	  int si = m_sentence.alignedToT[ti][i];
    	  std::string sourceSymbolIndex = IntToString(indexS.find(si)->second);
    	  std::string targetSymbolIndex = IntToString(p->second);
    	  rule.alignment += sourceSymbolIndex + "-" + targetSymbolIndex + " ";
    	  rule.alignmentInv += targetSymbolIndex + "-" + sourceSymbolIndex + " ";
    	}
      }
      
      // print alignment of non terminals for current span
      if (!foundWord) {
    	const HoleMBOT &hole = *iterHoleListSorted;
    	foundHole = ti == hole.GetStart(1);
    	foundHole = true;
    	int posS = hole.GetPos(0);
    	int posT = hole.GetPos(1);
    	std::string sourceSymbolIndex = IntToString(hole.GetPos(0));
    	std::string targetSymbolIndex = IntToString(hole.GetPos(1));
    	rule.alignment += sourceSymbolIndex + "-" + targetSymbolIndex + " ";
    	rule.alignmentInv += targetSymbolIndex + "-" + sourceSymbolIndex + " ";
    	ti = hole.GetEnd(1);
    	++iterHoleListSorted;
      }
    }
    if (numFrags > 1) {
      rule.alignment += "|| ";
      rule.alignmentInv += "|| ";
      --numFrags;
    }
  }
  assert(iterHoleListSorted == holeColl.GetSortedTargetHoles().end());
  rule.alignment.erase(rule.alignment.size()-1);
  rule.alignmentInv.erase(rule.alignmentInv.size()-1);
}

 void ExtractTask::saveMbotRule( int startS, int endS, vector<pair<int,int> > spansFrags
                       , HoleCollectionMBOT &holeColl, LabelIndex &labelIndex)
{
  WordIndex indexS, indexT; // to keep track of word positions in rule

  ExtractedRuleMBOT rule( startS, endS, spansFrags);

  // source tree fragment label
  string sourceLabel = m_options.sourceSyntax ? m_sentence.sourceTree.GetNodes(startS,endS)[ labelIndex[0] ]->GetLabel() : "X";

  // create non-terms for source tree fragment
  preprocessSourceFragment(startS, endS, spansFrags, indexS, holeColl, labelIndex);

  // create non-terms for target tree fragments
  preprocessTargetFragments(startS, endS, spansFrags, indexS, holeColl, labelIndex);

  // construct HoleCollectionMBOT with sorted target tree fragments
  holeColl.SortTargetHoles();

  // target tree fragments
  double logPCFGScore = 0.0f;
  rule.target = generateTargetFragments(startS, endS, spansFrags, indexT, holeColl, labelIndex, logPCFGScore);

  // source tree fragment
  rule.source = generateSourceFragment(startS, endS, spansFrags, holeColl, labelIndex);
  rule.source += " [" + sourceLabel + "]";

  // mbot non-term alignment
  saveMbotAlignment(startS, endS, spansFrags, indexS, indexT, holeColl, rule);

  addRuleToCollection( rule );
}

void ExtractTask::saveAllMbotRules( int startS, int endS, vector<pair<int,int> > spansFrags, HoleCollectionMBOT &holeColl)
{
  LabelIndex labelIndex,labelCount;

  // number of labels for source tree fragment
  int numLabels = m_options.sourceSyntax ? m_sentence.sourceTree.GetNodes(startS,endS).size() : 1;
  labelCount.push_back(numLabels);
  labelIndex.push_back(0);

  // number of source hole labels
   for (HoleList::const_iterator hole = holeColl.GetHoles().begin(); hole != holeColl.GetHoles().end(); hole++) {
     numLabels =  m_options.sourceSyntax ? m_sentence.sourceTree.GetNodes(hole->GetStartS(),hole->GetEndS()).size() : 1;
     labelCount.push_back(numLabels);
     labelIndex.push_back(0);
   }

  // number of labels for target tree fragments
   for (vector<int>::size_type i=0; i<spansFrags.size(); ++i) {
    numLabels = m_sentence.targetTree.GetNodes(spansFrags[i].first,spansFrags[i].second).size();
  	labelCount.push_back(numLabels);
  	labelIndex.push_back(0);
  }

   // number of target hole labels
  //int size_target = 0;
  for( HoleList::const_iterator hole = holeColl.GetHoles().begin(); hole != holeColl.GetHoles().end(); hole++ ) {
    vector<int> startT= hole->GetStartT();
    vector<int> endT = hole->GetEndT();
    for (vector<int>::size_type i=0; i<startT.size(); ++i) {
      //size_target++;
      int numLabels = m_sentence.targetTree.GetNodes(startT[i],endT[i]).size();
      labelCount.push_back(numLabels);
      labelIndex.push_back(0);
    }
  }
  
  // loop through the holes
  bool done = false;
  while (!done) {
    saveMbotRule( startS, endS, spansFrags, holeColl, labelIndex );
    for (unsigned int i=0; i<labelIndex.size(); i++) {
      labelIndex[i]++;
      if (labelIndex[i] == labelCount[i]) {
        labelIndex[i] = 0;
        if (i == labelIndex.size()-1) {
          done = true;
        }
      } else {
        break;
      }
    }
  }
}

// this function is called recursively
// it pokes a new hole into the rule, and then calls itself for more holes
void ExtractTask::addExtractableRule( int startS, int endS, vector<pair<int,int> > fragmentsSpans
				, RuleExistMBOT &ruleExist, const HoleCollectionMBOT &holeColl
				, int numHoles, int initStartS, int wordCountS, int wordCountT)
{
  // done, if already the maximum number of non-terminals in rule pair
  if (numHoles >= m_options.maxNonTerm)
    return;

  // find a hole...
  for (int startHoleS = initStartS; startHoleS <= endS; ++startHoleS) {
    for (int endHoleS = startHoleS+(m_options.minHoleSource-1); endHoleS <= endS; ++endHoleS) {

      // except the whole span
      if (startHoleS == startS && endHoleS == endS) 
        continue;
 
      // if last non-terminal, enforce word count limit
      if (numHoles == m_options.maxNonTerm-1 && wordCountS - (endHoleS-startS+1) + (numHoles+1) > m_options.maxSymbolsSource)
        continue;

      // determine the number of remaining source words
      const int newWordCountS = wordCountS - (endHoleS-startHoleS+1);

      // always enforce min word count limit
      if (newWordCountS < m_options.minWords)
    	  continue;

      // does a rule cover this source span?
      // if it does, then there should be a list of mapped target tree fragments
      // (multiple possible due to unaligned words)
      const HoleList &sourceHoles = ruleExist.GetSourceHoles(startHoleS, endHoleS);

      // loop over sub rules
      HoleList::const_iterator iterSourceHoles;
      for (iterSourceHoles = sourceHoles.begin(); iterSourceHoles != sourceHoles.end(); ++iterSourceHoles) {
        const HoleMBOT &sourceHole = *iterSourceHoles;
        bool minSize = true;
        if ( m_options.minHoleTarget != 1 ) {
          vector<int> fragmentLength = sourceHole.GetFragmentsLength();
          for (vector<int>::size_type i=0; i<fragmentLength.size(); ++i) {
            // (1) enforce minimum hole size for each target tree fragment
            if ( fragmentLength[i] < m_options.minHoleTarget) {
              minSize = false;
              break;
            }
          }
        }
        // (2) enforce minimum hole size for all target tree fragments
        if (minSize == false)
          continue;

        // determine the number of remaining target words
        const int targetHoleSize = sourceHole.GetCompLength();
        const int newWordCountT = wordCountT - targetHoleSize;

        // if last non-terminal, enforce word count limit
        if (numHoles == m_options.maxNonTerm-1 && newWordCountT + (numHoles+1) > m_options.maxSymbolsTarget)
          continue;

        // enforce min word count limit
        if (newWordCountT < m_options.minWords)
          continue;

        // hole must be sub target tree fragments of the target tree fragments
        // (may be violated if additional unaligned target word)
        bool subphrase;
        subphrase = sourceHole.subTargetFragments(fragmentsSpans);
        if ( !subphrase ) {
        	continue;
        }

        // make sure target tree fragments does not overlap with another hole
        if (holeColl.OverlapTargetFragments(sourceHole)) {
        	continue;
        }

        // if consecutive non-terminals are not allowed, check for it
        if (!m_options.nonTermConsecSource && holeColl.ConsecSource(sourceHole))
        	continue;

        // require that at least one aligned word is left (unless there are no words at all)
        if (m_options.requireAlignedWord && (newWordCountS > 0 || newWordCountT > 0)) {
          HoleList::const_iterator iterHoleList = holeColl.GetHoles().begin();
          bool foundAlignedWord = false;
          // loop through all word positions
          for (int pos = startS; pos <= endS && !foundAlignedWord; pos++) {
           // new hole? moving on...
           if (pos == startHoleS) {
             pos = endHoleS;
           }
           // covered by hole? moving on...
           else if (iterHoleList != holeColl.GetHoles().end() && iterHoleList->GetStartS() == pos) {
             pos = iterHoleList->GetEndS();
             ++iterHoleList;
           }
           // covered by word? check if it is aligned
           else {
             if (m_sentence.alignedToS[pos].size() > 0)
               foundAlignedWord = true;
           }
         }
         if (!foundAlignedWord)
           continue;
       }
       
       // generate vector with paired start and end of target tree fragments
       vector<pair<int,int> > holefragmentsSpans = sourceHole.GetPairedSpans();

       // update list of holes in this rule
       HoleCollectionMBOT copyHoleColl(holeColl);
       copyHoleColl.Add(startHoleS, endHoleS, holefragmentsSpans);

       // now some checks that disallow this rule, but not further recursion
       bool allowablePhrase = true;

       // maximum words count violation?
       if (newWordCountS + (numHoles+1) > m_options.maxSymbolsSource)
         allowablePhrase = false;

       if (newWordCountT + (numHoles+1) > m_options.maxSymbolsTarget)
         allowablePhrase = false;

       // passed all checks...
       if (allowablePhrase)
         saveAllMbotRules(startS, endS, fragmentsSpans, copyHoleColl);

      // recursively search for next hole
       int nextInitStartS = m_options.nonTermConsecSource ? endHoleS + 1 : endHoleS + 2;
       addExtractableRule(startS, endS, fragmentsSpans, ruleExist, copyHoleColl
		     , numHoles + 1, nextInitStartS, newWordCountS, newWordCountT);
      }
    }
  }
}

void ExtractTask::addInitialRule( int startS, int endS, vector<pair<int,int> > fragmentsSpans, RuleExistMBOT &ruleExist)
{
  ExtractedRuleMBOT rule(startS, endS, fragmentsSpans);

  // rule labels
  string targetLabel,sourceLabel;
  sourceLabel = m_options.sourceSyntax ? m_sentence.sourceTree.GetNodes(startS,endS)[0]->GetLabel() : "X";

  // initial source
  rule.source = "";
  for (int si=startS; si<=endS; si++)
    rule.source += m_sentence.source[si] + " ";
  rule.source += "[" + sourceLabel + "]";
  
  // initial target
  int numFrags = fragmentsSpans.size();
  int numAlign = numFrags;
  rule.target = "";
  vector<string> labelsTarget;
  for (vector<int>::size_type i = 0; i< fragmentsSpans.size(); ++i) {
    int startT = fragmentsSpans[i].first;
    int endT = fragmentsSpans[i].second;
    targetLabel = m_sentence.targetTree.GetNodes(startT,endT)[0]->GetLabel();
    labelsTarget.push_back(targetLabel);

    for (int ti=startT; ti<=endT; ti++)
      rule.target += m_sentence.target[ti] + " ";
    rule.target += "[" + targetLabel + "]";
    if (numFrags > 1) {
      rule.target += " || ";
      --numFrags;
    }
  }
  // generate mobt-glue-rule
  if ( m_options.mbotGrammarFlag && numAlign > 1 ) {
    generateMbotGlueRule( sourceLabel, labelsTarget, numAlign );
  }

  //mbot alignment
  for (vector<int>::size_type t=0; t<fragmentsSpans.size(); ++t) {
    int startT = fragmentsSpans[t].first;
    int endT = fragmentsSpans[t].second;
    for (int ti=startT; ti<=endT; ++ti) {
      for (unsigned int j=0; j<m_sentence.alignedToT[ti].size(); ++j) {
    	int si = m_sentence.alignedToT[ti][j];
    	std::string sourceSymbolIndex = IntToString(si-startS);
    	std::string targetSymbolIndex = IntToString(ti-startT);
    	rule.alignment += sourceSymbolIndex + "-" + targetSymbolIndex + " ";
    	rule.alignmentInv += targetSymbolIndex + "-" + sourceSymbolIndex + " ";
      }
    }
    if (numAlign > 1) {
      rule.alignment += "|| ";
      rule.alignmentInv += "|| ";
      --numAlign;
    }
  }
 
  rule.alignment.erase(rule.alignment.size()-1);
  rule.alignmentInv.erase(rule.alignmentInv.size()-1);

  addRuleToCollection( rule );
}

void ExtractTask::addRuleToCollection( ExtractedRuleMBOT &newRule )
{
  // no double-counting of identical rules from overlapping spans
  vector<ExtractedRuleMBOT>::const_iterator rule;
  for (rule = m_extractedRules.begin(); rule != m_extractedRules.end(); rule++ ) {
    if (rule->source.compare( newRule.source ) == 0 
        && rule->target.compare( newRule.target ) == 0 
        && !(rule->endS < newRule.startS || rule->startS > newRule.endS)) { // overlapping
      return;
    }
  }
  m_extractedRules.push_back( newRule );
}

void ExtractTask::consolidateRules()
 {
  typedef vector<ExtractedRuleMBOT>::iterator R;

  // consolidate counts
  map<std::string, map< std::string, map< std::string, float> > > consolidatedCount;
  for(R rule = m_extractedRules.begin(); rule != m_extractedRules.end(); rule++ ) {
	  consolidatedCount[ rule->source ][ rule->target][ rule->alignment ] += 1.0;
  }

  for(R rule = m_extractedRules.begin(); rule != m_extractedRules.end(); rule++ ) {
	  float count = consolidatedCount[ rule->source ][ rule->target][ rule->alignment ];
	  rule->count = count;
    consolidatedCount[ rule->source ][ rule->target][ rule->alignment ] = 0;
  }
 }


void ExtractTask::writeRulesToFile() {
  vector<ExtractedRuleMBOT>::const_iterator rule;
  ostringstream out;
  ostringstream outInv;

  for(rule = m_extractedRules.begin(); rule != m_extractedRules.end(); rule++ ) {
    if (rule->count == 0)
      continue;
    
    out << "[MBOT] ||| "
        << rule->source << " ||| "
        << rule->target << " ||| "
        << rule->alignment << " ||| "
        << rule->count << " ||| \n";

    outInv << "[MBOT] ||| "
           << rule->target << " ||| "
           << rule->source << " ||| "
           << rule->alignmentInv << " ||| "
           << rule->count << "\n";
  }

  m_extractFile << out.str();
  m_extractFileInv << outInv.str();
}

void ExtractTask::generateMbotGlueRule( string source, vector<string> targets, int size )
{
	string prefix, gluerule, lhs, rhs, align;

	if (m_options.sourceSyntax) {
		prefix = "[MBOT] ||| [ROOT][Q] [" + source + "]";
	}
	else {
		prefix = "[MBOT] ||| [X][Q] [" + source + "]";
	}

	for (size_t i=0; i<targets.size(); i++) {
		lhs += "[" + targets[i] + "]";
		rhs += " [" + source + "][" + targets[i] + "]";
	}

	int j=1;
	while ( j <= size ) {
		align += " 1-" + IntToString(j);
		j++;
	}

	if (m_options.sourceSyntax) {
		gluerule = prefix + lhs + " [ROOT] ||| [ROOT][Q]" + rhs + " [Q] ||| 2.718 ||| 0-0" + align;
	}
	else {
		gluerule = prefix + lhs + " [X] ||| [X][Q]" + rhs + " [Q] ||| 2.718 ||| 0-0" + align;
	}

	mbotGlues[gluerule]++;
}

void writeMbotGrammar( const string &fileName )
{
  ofstream mbotFile;
  mbotFile.open(fileName.c_str());

  map<string,int>::iterator it = mbotGlues.begin();
  for (it=mbotGlues.begin(); it!=mbotGlues.end(); ++it)
     mbotFile << it->first << endl;

  mbotFile.close();
}

void writeGlueGrammar( const string &fileName, MBOTExtractionOptions &options, set<string> &sourceLabelCollection,
		set<string> &targetLabelCollection, map<string,int> &sourceTopLabelCollection, map<string, int> &targetTopLabelCollection )
{
  ofstream grammarFile;
  grammarFile.open(fileName.c_str());

  // chose a top label that is not already a label
  string topLabel = "QQQQQQ";
  for( unsigned int i=1; i<=topLabel.length(); i++) {
    if(targetLabelCollection.find( topLabel.substr(0,i) ) == targetLabelCollection.end() ) {
      topLabel = topLabel.substr(0,i);
      break;
    }
  }

  if (options.sourceSyntax) {
    // basic rules
    grammarFile << "[MBOT] ||| <s> [ROOT] ||| <s> [" << topLabel << "] ||| 1 ||| " << endl
                << "[MBOT] ||| [ROOT][" << topLabel << "] </s> [ROOT] ||| [ROOT][" << topLabel 
                << "] </s> [" << topLabel << "] ||| 1 ||| 0-0 " << endl;

    // top rules
    for (map<string,int>::const_iterator i = sourceTopLabelCollection.begin();
         i != sourceTopLabelCollection.end(); i++) {
      for (map<string,int>::const_iterator j = targetTopLabelCollection.begin();
           j != targetTopLabelCollection.end(); j++) {
        grammarFile << "[MBOT] ||| <s> [" << i->first << "][" << j->first << "] </s> [ROOT] ||| <s> ["
                    << i->first << "][" << j->first << "] </s> [" << topLabel << "] ||| 1 ||| 1-1 " << endl;
      }
    }
    // glue rules
    for (set<string>::const_iterator i = sourceLabelCollection.begin();
         i != sourceLabelCollection.end(); i++) {
      for (set<string>::const_iterator j = targetLabelCollection.begin();
           j != targetLabelCollection.end(); j++) {
        grammarFile << "[MBOT] ||| [ROOT][" << topLabel << "] [" << *i << "][" << *j << "] [ROOT] ||| [ROOT]["
                    << topLabel << "] [" << *i << "][" << *j << "] [" << topLabel << "] ||| 2.718 ||| 0-0 1-1 " << endl;
      }
    }
  }
  else {
    // basic rules
    grammarFile << "[MBOT] ||| <s> [X] ||| <s> [" << topLabel << "] ||| 1  ||| " << endl
                << "[MBOT] ||| [X][" << topLabel << "] </s> [X] ||| [X][" << topLabel 
                << "] </s> [" << topLabel << "] ||| 1 ||| 0-0 " << endl;
    
    // top rules
    for ( map<string,int>::const_iterator i =  targetTopLabelCollection.begin();
          i !=  targetTopLabelCollection.end(); i++ ) {
      grammarFile << "[MBOT] ||| <s> [X][" << i->first << "] </s> [X] ||| <s> [X][" << i->first << "] </s> [" << topLabel << "] ||| 1 ||| 1-1" << endl;
    }
    
    // glue rules
    for ( set<string>::const_iterator i =  targetLabelCollection.begin();
          i !=  targetLabelCollection.end(); i++ ) {
      grammarFile << "[MBOT] ||| [X][" << topLabel << "] [X][" << *i << "] [X] ||| [X][" << topLabel << "] [X][" << *i << "] [" << topLabel << "] ||| 2.718 ||| 0-0 1-1" << endl;
	}
  }
  
  grammarFile.close();
}

map<string,int> wordCount;
map<string,string> wordLabel;
void collectUnknownWordLabelCounts( SentenceAlignmentMBOT &sentence)
{
	int countT = sentence.target.size();
	for (int ti=0; ti < countT; ti++) {
		string &word = sentence.target[ti];
		const vector< SyntaxNode* >& labels = sentence.targetTree.GetNodes(ti,ti);
		if (labels.size() > 0 ) {
			wordCount[word]++;
			wordLabel[word] = labels[0]->GetLabel();
		}
	}
}

void writeUnknownWordLabel(const string &fileName)
{
	ofstream outFile;
	outFile.open(fileName.c_str());
	typedef map<string,int>::const_iterator I;

	map<string,int> count;
	int total=0;
	for (I word=wordCount.begin(); word != wordCount.end(); word++) {
		// only consider singletons
		if (word->second == 1) {
			count[ wordLabel[word->first] ]++;
			total++;
		}
	}

	for (I pos=count.begin(); pos!=count.end(); pos++) {
		double ratio = ((double) pos->second / (double) total);
		if (ratio > 0.03)
			outFile << pos->first << " " << ratio << endl;
	}

	outFile.close();
}
