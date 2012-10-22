// $Id: PhraseDictionaryMemory.cpp 4365 2011-10-14 16:40:30Z heafield $
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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

#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <memory>
#include <sys/stat.h>
#include <stdlib.h>
#include "util/file_piece.hh"
#include "util/tokenize_piece.hh"

#include "PhraseDictionaryMemory.h"
#include "FactorCollection.h"
#include "Word.h"
#include "Util.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "WordsRange.h"
#include "UserMessage.h"
#include "SparsePhraseDictionaryFeature.h"

using namespace std;

namespace Moses
{

namespace {
void ParserDeath(const std::string &file, size_t line_num) {
  stringstream strme;
  strme << "Syntax error at " << file << ":" << line_num;
  UserMessage::Add(strme.str());
  abort();
}
template <class It> StringPiece GrabOrDie(It &it, const std::string &file, size_t line_num) {
  if (!it) ParserDeath(file, line_num);
  return *it++;
}
} // namespace

bool PhraseDictionaryMemory::Load(const std::vector<FactorType> &input
                                  , const std::vector<FactorType> &output
                                  , const string &filePath
                                  , const vector<float> &weight
                                  , size_t tableLimit
                                  , const LMList &languageModels
                                  , float weightWP)
{
  const_cast<LMList&>(languageModels).InitializeBeforeSentenceProcessing();

  const StaticData &staticData = StaticData::Instance();

  m_tableLimit = tableLimit;

  util::FilePiece inFile(filePath.c_str(), staticData.GetVerboseLevel() >= 1 ? &std::cerr : NULL);

  size_t line_num = 0;
  size_t numElement = NOT_FOUND; // 3=old format, 5=async format which include word alignment info
  const std::string& factorDelimiter = staticData.GetFactorDelimiter();

  Phrase sourcePhrase(0);
  std::vector<float> scv;
  scv.reserve(m_numScoreComponent);

  TargetPhraseCollection *preSourceNode = NULL;
  std::string preSourceString;

  while(true) {
    ++line_num;
    StringPiece line;
    try {
      line = inFile.ReadLine();
    } catch (util::EndOfFileException &e) {
      break;
    }

    util::TokenIter<util::MultiCharacter> pipes(line, util::MultiCharacter("|||"));
    StringPiece sourcePhraseString(GrabOrDie(pipes, filePath, line_num));
    StringPiece targetPhraseString(GrabOrDie(pipes, filePath, line_num));
    StringPiece scoreString(GrabOrDie(pipes, filePath, line_num));

    bool isLHSEmpty = !util::TokenIter<util::AnyCharacter, true>(sourcePhraseString, util::AnyCharacter(" \t"));
    if (isLHSEmpty && !staticData.IsWordDeletionEnabled()) {
      TRACE_ERR( filePath << ":" << line_num << ": pt entry contains empty source, skipping\n");
      continue;
    }
 
    //target
    std::auto_ptr<TargetPhrase> targetPhrase(new TargetPhrase());
    targetPhrase->CreateFromString(output, targetPhraseString, factorDelimiter);

    scv.clear();
    for (util::TokenIter<util::AnyCharacter, true> token(scoreString, util::AnyCharacter(" \t")); token; ++token) {
      char *err_ind;
      // Token is always delimited by some form of space.  Also, apparently strtod is portable but strtof isn't.  
      scv.push_back(FloorScore(TransformScore(static_cast<float>(strtod(token->data(), &err_ind)))));
      if (err_ind == token->data()) {
        stringstream strme;
        strme << "Bad number " << token << " on line " << line_num;
        UserMessage::Add(strme.str());
        abort();
      }
    }
    if (scv.size() != m_numScoreComponent) {
      stringstream strme;
      strme << "Size of scoreVector != number (" <<scv.size() << "!=" <<m_numScoreComponent<<") of score components on line " << line_num;
      UserMessage::Add(strme.str());
      abort();
    }


    
    size_t consumed = 3;
    if (pipes) {
      targetPhrase->SetAlignmentInfo(*pipes++);
      ++consumed;
    }

    ScoreComponentCollection sparse;
    if (pipes) pipes++; //counts
    if (pipes) {
      //sparse features
      SparsePhraseDictionaryFeature* spdf = 
        GetFeature()->GetSparsePhraseDictionaryFeature();
      if (spdf) {
        sparse.Assign(spdf,(pipes++)->as_string());
      }
    }


    // scv good to go sir!
    targetPhrase->SetScore(m_feature, scv, sparse, weight, weightWP, languageModels);

    // Check number of entries delimited by ||| agrees across all lines.  
    for (; pipes; ++pipes, ++consumed) {}
    if (numElement != consumed) {
      if (numElement == NOT_FOUND) {
        numElement = consumed;
      } else {
        stringstream strme;
        strme << "Syntax error at " << filePath << ":" << line_num;
        UserMessage::Add(strme.str());
        abort();
      }
    }

    //TODO: Would be better to reuse source phrases, but ownership has to be 
    //consistent across phrase table implementations
    sourcePhrase.Clear();
    sourcePhrase.CreateFromString(input, sourcePhraseString, factorDelimiter);
    //Now that the source phrase is ready, we give the target phrase a copy
    targetPhrase->SetSourcePhrase(sourcePhrase);
    if (preSourceString == sourcePhraseString && preSourceNode) {
      preSourceNode->Add(targetPhrase.release());
    } else {
      preSourceNode = CreateTargetPhraseCollection(sourcePhrase);
      preSourceNode->Add(targetPhrase.release());
      preSourceString.assign(sourcePhraseString.data(), sourcePhraseString.size());
    }
  }

  // sort each target phrase collection
  m_collection.Sort(m_tableLimit);

  /* // TODO ASK OLIVER WHY THIS IS NEEDED
  const_cast<LMList&>(languageModels).CleanUpAfterSentenceProcessing();
  */
  
  return true;
}

TargetPhraseCollection *PhraseDictionaryMemory::CreateTargetPhraseCollection(const Phrase &source)
{
  const size_t size = source.GetSize();

  PhraseDictionaryNode *currNode = &m_collection;
  for (size_t pos = 0 ; pos < size ; ++pos) {
    const Word& word = source.GetWord(pos);
    currNode = currNode->GetOrCreateChild(word);
    if (currNode == NULL)
      return NULL;
  }

  return currNode->CreateTargetPhraseCollection();
}

const TargetPhraseCollection *PhraseDictionaryMemory::GetTargetPhraseCollection(const Phrase &source) const
{
  // exactly like CreateTargetPhraseCollection, but don't create
  const size_t size = source.GetSize();

  const PhraseDictionaryNode *currNode = &m_collection;
  for (size_t pos = 0 ; pos < size ; ++pos) {
    const Word& word = source.GetWord(pos);
    currNode = currNode->GetChild(word);
    if (currNode == NULL)
      return NULL;
  }

  return currNode->GetTargetPhraseCollection();
}

PhraseDictionaryMemory::~PhraseDictionaryMemory()
{
}

TO_STRING_BODY(PhraseDictionaryMemory);

// friend
ostream& operator<<(ostream& out, const PhraseDictionaryMemory& phraseDict)
{
  const PhraseDictionaryNode &coll = phraseDict.m_collection;
  PhraseDictionaryNode::const_iterator iter;
  for (iter = coll.begin() ; iter != coll.end() ; ++iter) {
    const Word &word = (*iter).first;
    out << word;
  }
  return out;
}


}

