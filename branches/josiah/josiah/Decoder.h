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

#include "Hypothesis.h"
#include "Parameter.h"
#include "Sentence.h"
#include "SearchNormal.h"
#include "StaticData.h"
#include "TranslationOptionCollectionText.h"

//
// Wrapper functions and objects for the decoder.
//

namespace Josiah {

/**
 * Initialise moses (including StaticData) using the given ini file and debuglevel, passing through any 
 * other command line arguments. This must be called, even if the moses decoder is not being used, as the
 * sampler requires the moses tables.
 **/
void initMoses(const std::string& inifile, const std::string& weightfile, int debuglevel, int argc=0, char** argv=NULL);

/**
  * Wrapper around any decoder. Notice the moses specific return values!
  **/
class Decoder {
  public:
    Decoder(): m_isMonotone(false), m_useNoLM(false), m_useZeroWeights(false) {}
    virtual void decode(const std::string& source, Moses::Hypothesis*& bestHypo, Moses::TranslationOptionCollection*& toc, std::vector<Moses::Word>& sent) = 0;
    virtual void GetFeatureNames(std::vector<std::string>* featureNames) const = 0;
    virtual void GetFeatureWeights(std::vector<float>* weights) const = 0;
    virtual void SetFeatureWeights(const std::vector<float>& weights) = 0;
    virtual void SetMonotone(bool isMonotone) {m_isMonotone = isMonotone;}
    virtual void SetNoLM(bool useNoLM) {m_useNoLM = useNoLM;}
    virtual void SetZeroWeights(bool useZeroWeights) {m_useZeroWeights = useZeroWeights;}
    virtual ~Decoder();
  
  protected:
    bool m_isMonotone;
    bool m_useNoLM;
    bool m_useZeroWeights;

};

class MosesDecoder : public virtual Decoder {
  public:
    MosesDecoder()  {}
    virtual void decode(const std::string& source, Moses::Hypothesis*& bestHypo, Moses::TranslationOptionCollection*& toc, std::vector<Moses::Word>& sent);
    virtual void GetFeatureNames(std::vector<std::string>* featureNames) const;
    virtual void GetFeatureWeights(std::vector<float>* weights) const;
    virtual void SetFeatureWeights(const std::vector<float>& weights);
 
  private:
    std::auto_ptr<Moses::SearchNormal> m_searcher;
    std::auto_ptr<Moses::TranslationOptionCollection> m_toc;
};


} //namespace

