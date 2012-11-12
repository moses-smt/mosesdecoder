// $Id$
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#include <map>
#include <vector>
#include <string>
#include "moses/PhraseDictionary.h"
#include "OnDiskPt/OnDiskWrapper.h"
#include "OnDiskPt/Word.h"
#include "OnDiskPt/PhraseNode.h"

namespace Moses
{
class TargetPhraseCollection;
class DottedRuleStackOnDisk;
class WordPenaltyProducer;

/** Implementation of on-disk phrase table for hierarchical/syntax model.
 */   
class PhraseDictionaryOnDisk : public PhraseDictionary
{
  typedef PhraseDictionary MyBase;
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryOnDisk&);

protected:
  OnDiskPt::OnDiskWrapper m_dbWrapper;
  const LMList* m_languageModels;
  const WordPenaltyProducer* m_wpProducer;
  std::vector<FactorType> m_inputFactorsVec, m_outputFactorsVec;
  std::string m_filePath;

  void LoadTargetLookup();

public:
  PhraseDictionaryOnDisk(size_t numScoreComponent, PhraseDictionaryFeature* feature)
    : MyBase(numScoreComponent, feature), m_languageModels(NULL)
  {}
  virtual ~PhraseDictionaryOnDisk();

  PhraseTableImplementation GetPhraseTableImplementation() const {
    return OnDisk;
  }

  bool Load(const std::vector<FactorType> &input
            , const std::vector<FactorType> &output
            , const std::string &filePath
	    , const std::vector<float> &weight
            , size_t tableLimit,
            const LMList& languageModels,
            const WordPenaltyProducer* wpProducer);

  std::string GetScoreProducerDescription(unsigned) const {
    return "BerkeleyPt";
  }

  // PhraseDictionary impl
  //! find list of translations that can translates src. Only for phrase input
  virtual const TargetPhraseCollection *GetTargetPhraseCollection(const Phrase& src) const;

  void InitializeForInput(const InputType& input);
  void CleanUp();

  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const InputType &,
    const ChartCellCollectionBase &);
};

}  // namespace Moses
