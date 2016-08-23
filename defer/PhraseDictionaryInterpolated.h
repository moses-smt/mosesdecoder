/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2013- University of Edinburgh

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


#ifndef moses_PhraseDictionaryInterpolated_h
#define moses_PhraseDictionaryInterpolated_h

#include <boost/shared_ptr.hpp>

#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"

namespace Moses
{

/**
  * An interpolation of 1 or more PhraseDictionaryTree translation tables.
  **/
class PhraseDictionaryInterpolated : public PhraseDictionary
{
public:

  PhraseDictionaryInterpolated
  (size_t numScoreComponent,size_t numInputScores,const PhraseDictionaryFeature* feature);

  virtual ~PhraseDictionaryInterpolated() {
    delete m_targetPhrases;
  }

  // initialize ...
  bool Load(const std::vector<FactorType> &input
            , const std::vector<FactorType> &output
            , const std::vector<std::string>& config
            , const std::vector<float> &weight
            , size_t tableLimit
            , const LMList &languageModels
            , float weightWP);

  virtual TargetPhraseCollection::shared_ptr GetTargetPhraseCollection(const Phrase& src) const;
  virtual void InitializeForInput(ttasksptr const& ttask);
  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const InputType &,
    const ChartCellCollectionBase &) {
    throw std::logic_error("PhraseDictionaryInterpolated.CreateRuleLookupManager() Not implemented");
  }

private:

  typedef boost::shared_ptr<PhraseDictionaryTreeAdaptor> DictionaryHandle;
  std::vector<DictionaryHandle> m_dictionaries;
  std::vector<std::vector<float> > m_weights; //feature x table
  mutable TargetPhraseCollection::shared_ptr  m_targetPhrases;
  std::vector<float> m_weightT;
  size_t m_tableLimit;
  const LMList* m_languageModels;
  float m_weightWP;

};


}

#endif
