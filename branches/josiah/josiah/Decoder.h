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
#include "SearchRandom.h"
#include "StaticData.h"
#include "TrellisPathList.h"
#include "TranslationOptionCollectionText.h"

//
// Wrapper functions and objects for the decoder.
//

namespace Josiah {
  
typedef std::vector<const Moses::Factor*> Translation;

/**
 * Initialise moses (including StaticData) using the given ini file and debuglevel, passing through any 
 * other command line arguments. This must be called, even if the moses decoder is not being used, as the
 * sampler requires the moses tables.
 **/
void initMoses(const std::string& inifile, const std::string& weightfile, int debuglevel, bool l1normalize = false, int argc=0, char** argv=NULL );

//Convenience methods for accessing the moses global data structures
void GetFeatureNames(std::vector<std::string>* featureNames);
void GetFeatureWeights(std::vector<float>* weights);
void SetFeatureWeights(const std::vector<float>& weights, bool compute_scale_gradient = false);
void OutputWeights(std::ostream& out);
void OutputWeights(const std::vector<float>& weights, std::ostream& out);
//inline float GetCurrQuenchingTemp() { return quenching_temp;}
//void SetQuenchingTemp(const std::vector<float>& weights);

//comparison method for sorting container of hyps
bool hypCompare(const Moses::Hypothesis* a, const Moses::Hypothesis* b);


/**
  * Wrapper around any decoder. Notice the moses specific return values!
  **/
class Decoder {
  public:
    Decoder(): m_isMonotone(false), m_useNoLM(false), m_useZeroWeights(false) {}
    virtual void decode(const std::string& source, Moses::Hypothesis*& bestHypo, Moses::TranslationOptionCollection*& toc, std::vector<Moses::Word>& sent, size_t nBestSize = 0) = 0;
    virtual void SetMonotone(bool isMonotone) {m_isMonotone = isMonotone;}
    virtual void SetNoLM(bool useNoLM) {m_useNoLM = useNoLM;}
    virtual void SetZeroWeights(bool useZeroWeights) {m_useZeroWeights = useZeroWeights;}
    virtual ~Decoder();
    
  
  protected:
    bool m_isMonotone;
    bool m_useNoLM;
    bool m_useZeroWeights;

};
/**
  * Wraps moses decoder.
 **/
class MosesDecoder : public virtual Decoder {
  public:
    MosesDecoder()  {}
    virtual void decode(const std::string& source, Moses::Hypothesis*& bestHypo, Moses::TranslationOptionCollection*& toc,  
                        std::vector<Moses::Word>& sent, size_t nBestSize = 0);
    const std::vector<std::pair<Translation, float> > & GetNbestTranslations() {return m_translations;}
    virtual Moses::Search* createSearch(Moses::Sentence& sentence, Moses::TranslationOptionCollection& toc);
    void PrintNBest(std::ostream& out) const;
    double CalcZ() ;
    double GetTranslationScore(const std::vector <const Moses::Factor *>& target) const;
  protected:
    std::auto_ptr<Moses::Search> m_searcher;
    std::auto_ptr<Moses::TranslationOptionCollection> m_toc;
    std::vector<std::pair<Translation,float> > m_translations;
    void CalcNBest(size_t count, Moses::TrellisPathList &ret,bool onlyDistinct = true) const; 
    void TrellisToTranslations(const Moses::TrellisPathList &ret, std::vector<std::pair<Translation,float> > &);
    void GetWinnerConnectedGraph( std::map< int, bool >* pConnected, std::vector< const Moses::Hypothesis* >* pConnectedList) const ;
  private:
    std::map < int, bool > connected;
    std::vector< const Moses::Hypothesis *> connectedList;    
};

/**
  * Wraps moses decoder to calculate KL
 **/
class KLDecoder : public  MosesDecoder {
  public:
    KLDecoder() : MosesDecoder() {}
    virtual void decode(const std::string& source, Moses::Hypothesis*& bestHypo, Moses::TranslationOptionCollection*& toc,  
                        std::vector<Moses::Word>& sent, size_t nBestSize = 0);
    double GetZ(const std::string& source); //only makes sense to ask for Z for KL decoder since other decoders do pruning
};




/**
  * Creates random hypotheses.
 **/
class RandomDecoder : public virtual MosesDecoder {
  public:
    RandomDecoder()  {}
    virtual Moses::Search* createSearch(Moses::Sentence& sentence, Moses::TranslationOptionCollection& toc);
    
  private:
};
} //namespace

