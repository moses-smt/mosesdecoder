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
#pragma once

#include <iostream>
#include <cstring>
#include <sstream>

#include <boost/shared_ptr.hpp>

#include "FeatureVector.h"
#include "Hypothesis.h"
#include "Parameter.h"
#include "Sentence.h"
#include "SearchNormal.h"
#include "StaticData.h"
#include "TrellisPathList.h"
#include "TranslationOptionCollectionText.h"

//
// Wrapper functions and objects for the decoder.
//

namespace Josiah {
  
typedef std::vector<const Moses::Factor*> Translation;
typedef boost::shared_ptr<Moses::Hypothesis> HypothesisHandle;
typedef boost::shared_ptr<Moses::Manager> ManagerHandle;
typedef boost::shared_ptr<Moses::TranslationOptionCollection> TOCHandle;
typedef std::vector<HypothesisHandle> HypothesisVector; 

/**
 * Initialise moses (including StaticData) using the given ini file and
 *  debuglevel, passing through any * other command line arguments. 
 **/
void initMoses(const std::string& inifile, int debuglevel,  const std::vector<std::string>& = std::vector<std::string>());



/** Update all the core moses weights */
void setMosesWeights(const Moses::FVector& weights);

/**
  * Generates random translation hypotheses.
  **/
class TranslationHypothesis {
  public:
    TranslationHypothesis(const std::string& source);

    Moses::TranslationOptionCollection* getToc() const;
    Moses::Hypothesis* getHypothesis() const;
    //source sentence
    const std::vector<Moses::Word>& getWords() const;

  private:
    static bool m_cleanup;
    HypothesisHandle m_hypothesis;
    TOCHandle m_toc;
    HypothesisVector m_allHypos;
    ManagerHandle m_manager;
    std::vector<Moses::Word> m_words;
};

} //namespace

