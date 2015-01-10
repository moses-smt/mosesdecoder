// $Id$
/***********************************************************************
 Moses - factored phrase-based, hierarchical and syntactic language decoder
 Copyright (C) 2009 Hieu Hoang

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
#include <iostream>
#include <string>
#include <vector>
#include <iterator>
#include <cassert>
#include "moses/InputFileStream.h"
#include "moses/Util.h"
#include "OnDiskWrapper.h"
#include "SourcePhrase.h"
#include "TargetPhrase.h"
#include "TargetPhraseCollection.h"
#include "Word.h"
#include "Vocab.h"
#include "Main.h"

using namespace std;
using namespace OnDiskPt;

int main (int argc, char * const argv[])
{
  // insert code here...
  Moses::ResetUserTime();
  Moses::PrintUserTime("Starting");

  if (argc != 8) {
    std::cerr << "Usage: " << argv[0] << " numSourceFactors numTargetFactors numScores tableLimit sortScoreIndex inputPath outputPath" << std::endl;
    return 1;
  }

  int numSourceFactors	= Moses::Scan<int>(argv[1])
                          , numTargetFactors	= Moses::Scan<int>(argv[2])
                              , numScores				= Moses::Scan<int>(argv[3])
                                  , tableLimit				= Moses::Scan<int>(argv[4]);
  TargetPhraseCollection::s_sortScoreInd			= Moses::Scan<int>(argv[5]);
  assert(TargetPhraseCollection::s_sortScoreInd < numScores);

  const string filePath 	= argv[6]
                            ,destPath	= argv[7];

  Moses::InputFileStream inStream(filePath);

  OnDiskWrapper onDiskWrapper;
  onDiskWrapper.BeginSave(destPath, numSourceFactors, numTargetFactors, numScores);

  PhraseNode &rootNode = onDiskWrapper.GetRootSourceNode();
  size_t lineNum = 0;
  string line;

  while(getline(inStream, line)) {
    lineNum++;
    if (lineNum%1000 == 0) cerr << "." << flush;
    if (lineNum%10000 == 0) cerr << ":" << flush;
    if (lineNum%100000 == 0) cerr << lineNum << flush;
    //cerr << lineNum << " " << line << endl;

    std::vector<float> misc(1);
    SourcePhrase sourcePhrase;
    TargetPhrase *targetPhrase = new TargetPhrase(numScores);
    OnDiskPt::PhrasePtr spShort = Tokenize(sourcePhrase, *targetPhrase, line, onDiskWrapper, numScores, misc);
    assert(misc.size() == onDiskWrapper.GetNumCounts());

    rootNode.AddTargetPhrase(sourcePhrase, targetPhrase, onDiskWrapper, tableLimit, misc, spShort);
  }

  rootNode.Save(onDiskWrapper, 0, tableLimit);
  onDiskWrapper.EndSave();

  Moses::PrintUserTime("Finished");

  //pause();
  return 0;

} // main()

bool Flush(const OnDiskPt::SourcePhrase *prevSourcePhrase, const OnDiskPt::SourcePhrase *currSourcePhrase)
{
  if (prevSourcePhrase == NULL)
    return false;

  assert(currSourcePhrase);
  bool ret = (*currSourcePhrase > *prevSourcePhrase);
  //cerr << *prevSourcePhrase << endl << *currSourcePhrase << " " << ret << endl << endl;

  return ret;
}

OnDiskPt::PhrasePtr Tokenize(SourcePhrase &sourcePhrase, TargetPhrase &targetPhrase, const std::string &lineStr, OnDiskWrapper &onDiskWrapper, int numScores, vector<float> &misc)
{
  char line[lineStr.size() + 1];
  strcpy(line, lineStr.c_str());

  stringstream sparseFeatures, property;

  size_t scoreInd = 0;

  // MAIN LOOP
  size_t stage = 0;
  /*	0 = source phrase
   1 = target phrase
   2 = scores
   3 = align
   4 = count
   7 = properties
   */
  char *tok = strtok (line," ");
  OnDiskPt::PhrasePtr out(new Phrase());
  while (tok != NULL) {
    if (0 == strcmp(tok, "|||")) {
      ++stage;
    } else {
      switch (stage) {
      case 0: {
        WordPtr w = Tokenize(sourcePhrase, tok, true, true, onDiskWrapper, 1);
        if (w != NULL)
          out->AddWord(w);

        break;
      }
      case 1: {
        Tokenize(targetPhrase, tok, false, true, onDiskWrapper, 0);
        break;
      }
      case 2: {
        float score = Moses::Scan<float>(tok);
        targetPhrase.SetScore(score, scoreInd);
        ++scoreInd;
        break;
      }
      case 3: {
        //targetPhrase.Create1AlignFromString(tok);
        targetPhrase.CreateAlignFromString(tok);
        break;
      }
      case 4: {
      	// store only the 3rd one (rule count)
      	float val = Moses::Scan<float>(tok);
      	misc[0] = val;
          break;
      }
      case 5: {
      	// sparse features
      	sparseFeatures << tok << " ";
        break;
      }
      case 6: {
	    property << tok << " ";
	    break;
      }
      default:
        cerr << "ERROR in line " << line << endl;
        assert(false);
        break;
      }
    }

    tok = strtok (NULL, " ");
  } // while (tok != NULL)

  assert(scoreInd == numScores);
  targetPhrase.SetSparseFeatures(Moses::Trim(sparseFeatures.str()));
  targetPhrase.SetProperty(Moses::Trim(property.str()));
  targetPhrase.SortAlign();
  return out;
} // Tokenize()

OnDiskPt::WordPtr Tokenize(OnDiskPt::Phrase &phrase
                           , const std::string &token, bool addSourceNonTerm, bool addTargetNonTerm
                           , OnDiskPt::OnDiskWrapper &onDiskWrapper, int retSourceTarget)
{
  // retSourceTarget: 0 = don't return anything. 1 = source, 2 = target

  bool nonTerm = false;
  size_t tokSize = token.size();
  int comStr =token.compare(0, 1, "[");

  if (comStr == 0) {
    comStr = token.compare(tokSize - 1, 1, "]");
    nonTerm = comStr == 0;
  }

  OnDiskPt::WordPtr out;
  if (nonTerm) {
    // non-term
    size_t splitPos		= token.find_first_of("[", 2);
    string wordStr	= token.substr(0, splitPos);

    if (splitPos == string::npos) {
      // lhs - only 1 word
      WordPtr word(new Word());
      word->CreateFromString(wordStr, onDiskWrapper.GetVocab());
      phrase.AddWord(word);
    } else {
      // source & target non-terms
      if (addSourceNonTerm) {
        WordPtr word(new Word());
        word->CreateFromString(wordStr, onDiskWrapper.GetVocab());
        phrase.AddWord(word);

        if (retSourceTarget == 1) {
          out = word;
        }
      }

      wordStr = token.substr(splitPos, tokSize - splitPos);
      if (addTargetNonTerm) {
        WordPtr word(new Word());
        word->CreateFromString(wordStr, onDiskWrapper.GetVocab());
        phrase.AddWord(word);

        if (retSourceTarget == 2) {
          out = word;
        }
      }

    }
  } else {
    // term
    WordPtr word(new Word());
    word->CreateFromString(token, onDiskWrapper.GetVocab());
    phrase.AddWord(word);
    out = word;
  }

  return out;
}

void InsertTargetNonTerminals(std::vector<std::string> &sourceToks, const std::vector<std::string> &targetToks, const ::AlignType &alignments)
{
  for (int ind = alignments.size() - 1; ind >= 0; --ind) {
    const ::AlignPair &alignPair = alignments[ind];
    size_t sourcePos = alignPair.first
                       ,targetPos = alignPair.second;

    const string &target = targetToks[targetPos];
    sourceToks.insert(sourceToks.begin() + sourcePos + 1, target);

  }
}

class AlignOrderer
{
public:
  bool operator()(const ::AlignPair &a, const ::AlignPair &b) const {
    return a.first < b.first;
  }
};

void SortAlign(::AlignType &alignments)
{
  std::sort(alignments.begin(), alignments.end(), AlignOrderer());
}
