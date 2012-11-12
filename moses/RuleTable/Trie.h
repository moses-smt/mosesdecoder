/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh
 
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

#pragma once

#include "moses/PhraseDictionary.h"
#include "moses/TypeDef.h"

#include <string>
#include <vector>

namespace Moses
{

class LMList;
class Phrase;
class TargetPhrase;
class TargetPhraseCollection;
class Word;
class WordPenaltyProducer;

/*** Implementation of a SCFG rule table in a trie.  Looking up a rule of
 * length n symbols requires n look-ups to find the TargetPhraseCollection.
 * @todo why need this and PhraseDictionarySCFG?
 */
class RuleTableTrie : public PhraseDictionary
{
 public:
  RuleTableTrie(size_t numScoreComponents, PhraseDictionaryFeature *feature)
      : PhraseDictionary(numScoreComponents, feature) {}

  virtual ~RuleTableTrie();

  virtual bool Load(const std::vector<FactorType> &input,
                    const std::vector<FactorType> &output,
                    const std::string &filePath,
                    const std::vector<float> &weight,
                    size_t tableLimit,
                    const LMList &languageModels,
                    const WordPenaltyProducer *wpProducer);

  const std::string &GetFilePath() const { return m_filePath; }
  void SetFilePath(const std::string &path) { m_filePath = path; }

  // Required by PhraseDictionary.
  const TargetPhraseCollection *GetTargetPhraseCollection(const Phrase &) const
  {
    assert(false);
    return NULL;
  }

  virtual void InitializeForInput(const InputType& i);

  void CleanUp();

 private:
  friend class RuleTableLoader;

  virtual TargetPhraseCollection &GetOrCreateTargetPhraseCollection(
      const Phrase &source, const TargetPhrase &target,
      const Word &sourceLHS) = 0;

  virtual void SortAndPrune() = 0;

  std::string m_filePath;
};

}  // namespace Moses
