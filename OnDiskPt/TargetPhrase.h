#pragma once
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

#include <fstream>
#include <string>
#include <vector>
#include "Word.h"
#include "Phrase.h"
#include "SourcePhrase.h"

namespace Moses
{
class PhraseDictionary;
class TargetPhrase;
class Phrase;
}

namespace OnDiskPt
{

typedef std::pair<UINT64, UINT64>  AlignPair;
typedef std::vector<AlignPair> AlignType;

class Vocab;

/** A target phrase, with the score breakdowns, alignment info and assorted other information it need.
 *  Readable and writeable to disk
 */
class TargetPhrase: public Phrase
{
  friend std::ostream& operator<<(std::ostream&, const TargetPhrase&);
protected:
  AlignType m_align;
  PhrasePtr m_sourcePhrase;

  std::vector<float> m_scores;
  UINT64 m_filePos;

  size_t WriteAlignToMemory(char *mem) const;
  size_t WriteScoresToMemory(char *mem) const;

  UINT64 ReadAlignFromFile(std::fstream &fileTPColl);
  UINT64 ReadScoresFromFile(std::fstream &fileTPColl);

public:
  TargetPhrase() {
  }
  TargetPhrase(size_t numScores);
  TargetPhrase(const TargetPhrase &copy);
  virtual ~TargetPhrase();

  void SetSourcePhrase(PhrasePtr p) {
    m_sourcePhrase = p;
  }
  const PhrasePtr GetSourcePhrase() const {
    return m_sourcePhrase;
  }
  const std::vector<float> &GetScores() const {
    return m_scores;
  }

  void SetLHS(WordPtr lhs);

  void Create1AlignFromString(const std::string &align1Str);
  void CreateAlignFromString(const std::string &align1Str);
  void SetScore(float score, size_t ind);

  const AlignType &GetAlign() const {
    return m_align;
  }
  void SortAlign();

  char *WriteToMemory(OnDiskWrapper &onDiskWrapper, size_t &memUsed) const;
  char *WriteOtherInfoToMemory(OnDiskWrapper &onDiskWrapper, size_t &memUsed) const;
  void Save(OnDiskWrapper &onDiskWrapper);

  UINT64 GetFilePos() const {
    return m_filePos;
  }
  float GetScore(size_t ind) const {
    return m_scores[ind];
  }

  Moses::TargetPhrase *ConvertToMoses(const std::vector<Moses::FactorType> &inputFactors
                                      , const std::vector<Moses::FactorType> &outputFactors
                                      , const Vocab &vocab
                                      , const Moses::PhraseDictionary &phraseDict
                                      , const std::vector<float> &weightT
                                      , bool isSyntax) const;
  UINT64 ReadOtherInfoFromFile(UINT64 filePos, std::fstream &fileTPColl);
  UINT64 ReadFromFile(std::fstream &fileTP);

  virtual void DebugPrint(std::ostream &out, const Vocab &vocab) const;

};

}
