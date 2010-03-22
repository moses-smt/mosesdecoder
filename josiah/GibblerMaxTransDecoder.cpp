#include "GibblerMaxTransDecoder.h"
#include "Derivation.h"
#include "GainFunction.h"
#include "SentenceBleu.h"
#include "StaticData.h"
#include "Gibbler.h"
#include "Decoder.h"


#include <sstream>
#include <map>
#include <ext/algorithm>

using namespace __gnu_cxx;
using namespace std;

namespace Josiah
{

  template<class M>
  void MaxCollector<M>::reset() 
  {
    m_samples.clear();
    m_sampleList.clear();
    SampleCollector::reset();
  }
  
  template<class M>
  void MaxCollector<M>::getDistribution(map<const M*,double>& p) const
  {
    const vector<double>& importanceWeights =  getImportanceWeights();
    
    for (typename map<M,vector<size_t> >::const_iterator i = m_samples.begin(); i != m_samples.end(); ++i) {
      const M* sample = &(i->first);
      for (vector<size_t>::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
        p[sample] += importanceWeights[*j];
      }
    }
    IFVERBOSE(2) {
      float total = 0;
      VERBOSE(2, "Distribution: ");
      //sort it
      multimap<double, const M*> sortedp;
      for (typename map<const M*,double>::const_iterator pi = p.begin(); pi != p.end(); ++pi) {
        sortedp.insert(make_pair(pi->second,pi->first));
        total += pi->second;
      }
      
      for (typename multimap<double, const M*>::reverse_iterator spi = sortedp.rbegin(); spi != sortedp.rend(); ++spi) {
        VERBOSE(2, spi->second << "{ " << *(spi->second) << " }: " <<  spi->first << " " << endl;);
      }
      VERBOSE(2, endl << "Total = " << total << endl);
    }
  }
  
  template<class M>
  void MaxCollector<M>::printDistribution(ostream& out) const
  {
    map<const M*, double> p;
    getDistribution(p);
    //sort it
    multimap<double, const M*> sortedp;
    for (typename map<const M*,double>::const_iterator pi = p.begin(); pi != p.end(); ++pi) {
      sortedp.insert(make_pair(pi->second,pi->first));
    }
    
    for (typename multimap<double, const M*>::reverse_iterator spi = sortedp.rbegin(); spi != sortedp.rend(); ++spi) {
      out << *(spi->second) << "|||" << spi->first << endl;
    }
  }
  
  template<class M>
  float MaxCollector<M>::getEntropy() const
  {
    map<const M*, double> p;
    getDistribution(p);
    float entropy = 0;
    //cerr << "Entropy: ";
    for (typename map<const M*,double>::const_iterator pi = p.begin(); pi != p.end(); ++pi) {
      //cerr << pi->second << " ";
      entropy -= pi->second*log(pi->second);
    }
    //cerr << endl;
    //cerr << "Entropy : " << entropy << endl;
    return entropy;
  }

  template<class M>
  void MaxCollector<M>::collectSample( const M &m)
  {
    m_samples[m].push_back(N());
    typename map<M,vector<size_t> >::const_iterator i = m_samples.find(m);
    m_sampleList.push_back(&(i->first));
    
    if (m_outputMaxChange) {
      pair<const M*,float> max = getMax();
      if (max.first != m_max) {
        m_max = max.first;
        cerr << "NewMax" << m_name << "(" << N() << ") ";
        cerr << *m_max;
        cerr << endl;
      }
    }
  }
  
  template<class M>
      const M* MaxCollector<M>::getSample(size_t index) const 
  {
    return m_sampleList.at(index);
  }
  
  template<class M>
      pair<const M*,float> MaxCollector<M>::getMax() const 
  {
    const M* argmax = NULL;
    float max = 0;
    map<const M*,double> p;
    getDistribution(p);
    for (typename map<const M*,double>::const_iterator pi = p.begin(); pi != p.end(); ++pi) {
      if (pi->second > max) {
        max = pi->second;
        argmax = pi->first;
      }
    }
    
    return pair<const M*,float>(argmax,max);
  }
  
  template<class M>
  struct ProbGreaterThan :  public std::binary_function<const pair<const M*,float>&,const pair<const M*,float>&,bool>{
    bool operator()(const pair<const M*,float>& d1, const pair<const M*,float>& d2) const {
      return d1.second > d2.second; 
    }
  };
  
  template<class M>
      void MaxCollector<M>::getNbest(vector<pair<const M*, float> >& nbest, size_t n) const 
  {
    map<const M*,double> p;
    getDistribution(p);
    nbest.assign(p.begin(),p.end());
    ProbGreaterThan<M> comparator;

    stable_sort(nbest.begin(),nbest.end(),comparator);
    if (n > 0) {
      while (nbest.size() > n) {
        nbest.pop_back();
      }
    }
  }

  template class MaxCollector<Josiah::Derivation>;
  template class MaxCollector<Josiah::Translation>;



  string ToString(const Translation& ws)
  {
    ostringstream os;
    for (Translation::const_iterator i = ws.begin(); i != ws.end(); ++i)
      os << (*i)->GetString() << " ";
    return os.str();
  }

  ostream& operator<<(ostream& out, const Translation& ws)
  {
    out << ToString(ws);
    return out;
  }

  void GibblerMaxTransDecoder::collect(Sample& sample)
  {
    const Hypothesis* h = sample.GetSampleHypothesis();
    vector<const Factor*> trans;
    h->GetTranslation(&trans, 0);
    collectSample(trans);

    
  }
  
  pair<const Translation*,float> GibblerMaxTransDecoder::getMbr(size_t mbrSize, size_t topNsize) const {
    
  //Posterior probs computed using the whole evidence set
  //MBR decoding outer loop using configurable size
    vector<pair<const Translation*, float> > topNTranslations;
    getNbest(topNTranslations,topNsize);
    
    GainFunctionVector g;
    vector<pair<const Translation*, float> >::iterator it;
    for (it = topNTranslations.begin(); it != topNTranslations.end(); ++it) {
      VERBOSE(1, "translation: " <<   ToString(*it->first) << " " << (it->second) << endl);
      g.push_back(new SentenceBLEU(4,*it->first)); //Calc the sufficient statistics for the translation
    }
  
  //Main MBR computation done here
    float bleu(0.0), weightedLoss(0.0), weightedLossCumul(0.0), minMBRLoss(100000);
    vector<float> mbrLoss;
    int minMBRLossIdx(-1);
    mbrSize = min(mbrSize,  topNTranslations.size());
    VERBOSE(0, "MBR SIZE " << mbrSize << ", all Translations Size " << topNTranslations.size() << endl);
  
  //Outer loop using only the top #mbrSize samples 
    for(size_t i = 0; i < mbrSize; ++i) {
      weightedLossCumul = 0.0;
      const GainFunction& gf = g[i];
      VERBOSE(2, "Reference " << ToString(*topNTranslations[i].first) << endl);
      for(size_t j = 0; j < topNTranslations.size(); ++j) {//Inner loop using all samples
        if (static_cast<size_t>(i) != j) {
          bleu = gf.ComputeGain(g[j]);
          VERBOSE(2, "Hypothesis " << ToString(*topNTranslations[j].first) << endl);
          weightedLoss = (1- bleu) * topNTranslations[j].second;
          VERBOSE(2, "Bleu " << bleu << ", prob " <<  topNTranslations[j].second << ", weightedLoss : " << weightedLoss << endl);
          weightedLossCumul += weightedLoss;
          if (weightedLossCumul > minMBRLoss)
            break;
        }
      }
      VERBOSE(2, "Bayes risk for cand " << i << " " <<  weightedLossCumul << endl);
      if (weightedLossCumul < minMBRLoss){
        minMBRLoss = weightedLossCumul;
        minMBRLossIdx = i;
      }
    }
    VERBOSE(2, "Minimum Bayes risk cand is " <<  minMBRLossIdx << " with risk " << minMBRLoss << endl);
  
    return topNTranslations[minMBRLossIdx];
  }
  
  size_t GibblerMaxTransDecoder::getMbr(const vector<pair<Translation,float> >& translations, size_t topNsize) const {
    
    //Posterior probs computed using the whole evidence set
    //MBR decoding outer loop using configurable size
    vector<pair<const Translation*, float> > topNTranslations;
    getNbest(topNTranslations,topNsize);
    
    GainFunctionVector gEvidenceSet;
    vector<pair<const Translation*, float> >::iterator it;
    for (it = topNTranslations.begin(); it != topNTranslations.end(); ++it) {
      VERBOSE(1, "Evidence translation: " <<   ToString(*it->first) << " " << (it->second) << endl);
      gEvidenceSet.push_back(new SentenceBLEU(4,*it->first)); //Calc the sufficient statistics for the translation
    }
    
    GainFunctionVector gHypothesisSet;
    vector<pair<Translation, float> >::const_iterator itt;
    for (itt = translations.begin(); itt != translations.end(); ++itt) {
      VERBOSE(1, "Hypothesis translation: " <<   ToString(itt->first) << " " << (itt->second) << endl);
      gHypothesisSet.push_back(new SentenceBLEU(4,itt->first)); //Calc the sufficient statistics for the translation
    }
    
    //Main MBR computation done here
    float bleu(0.0), weightedLoss(0.0), weightedLossCumul(0.0), minMBRLoss(100000);
    vector<float> mbrLoss;
    int minMBRLossIdx(-1);
    size_t mbrSize = translations.size();
    VERBOSE(0, "MBR SIZE " << mbrSize << ", all Translations Size " << topNTranslations.size() << endl);
    
    //Outer loop using only the top #mbrSize samples 
    for(size_t i = 0; i < mbrSize; ++i) {
      weightedLossCumul = 0.0;
      const GainFunction& gf = gHypothesisSet[i];
      VERBOSE(1, "Reference " << ToString(translations[i].first) << " : [" << translations[i].second << "]" <<  endl);
      for(size_t j = 0; j < topNTranslations.size(); ++j) {//Inner loop using all samples
        //if (static_cast<size_t>(i) != j) {
          bleu = gf.ComputeGain(gEvidenceSet[j]);
          VERBOSE(1, "Hypothesis " << ToString(*topNTranslations[j].first) << endl);
          weightedLoss = (1- bleu) * topNTranslations[j].second;
          VERBOSE(1, "Bleu " << bleu << ", prob " <<  topNTranslations[j].second << ", weightedLoss : " << weightedLoss << endl);
          weightedLossCumul += weightedLoss;
          if (weightedLossCumul > minMBRLoss)
            break;
        //}
      }
      VERBOSE(1, "Bayes risk for cand " << i << " " <<  weightedLossCumul << endl);
      if (weightedLossCumul < minMBRLoss){
        VERBOSE(1, "New best MBR sol: " << ToString(translations[i].first) << " " <<  weightedLossCumul << endl);
        minMBRLoss = weightedLossCumul;
        minMBRLossIdx = i;
      }
    }
    VERBOSE(2, "Minimum Bayes risk cand is " <<  minMBRLossIdx << " with risk " << minMBRLoss << endl);
    
    return minMBRLossIdx;
  }
  /**** Algorithm: Run exact decoder for given input. Then compute exact p(e|f) for elements in evidence set.  
   Then perform MBR a) by using a sample from the evidence set b) by sorting evidence set and discarding low prob translations
  ****/
  void GibblerMaxTransDecoder::getExactMbr(const std::string& line, std::vector< std::pair<const Translation*,float> > & best, float shrinkEvidenceRatio, size_t mbrSize, size_t topNsize)  {
    //Posterior probs computed using the whole evidence set
    //MBR decoding outer loop using configurable size
    vector<pair<const Translation*, float> > topNTranslations;
    getNbest(topNTranslations,topNsize);
    
    //Get unpruned lattice for input
    auto_ptr<KLDecoder> klDecoder;
    klDecoder.reset(new KLDecoder); 
    double trueZ = klDecoder->GetZ(line); //Lattice Z
    
    //Now calculate true p(e|f) for all translations samples
    vector<pair<const Translation*, float> >::iterator it; 
    VERBOSE(1,"Hypothesis set: " << endl);
    for (it = topNTranslations.begin(); it != topNTranslations.end(); ++it) {
      float trueTransScore = exp(klDecoder->GetTranslationScore(*it->first) - trueZ);;
      VERBOSE(1, "translation: " <<   ToString(*it->first) << " " << (it->second) << " " << trueTransScore << endl);
      it->second = trueTransScore;
    }
    
    topNsize = topNTranslations.size(); //size of full evidence set
    size_t evidenceSize = topNsize * shrinkEvidenceRatio; //New size of evidence set
    evidenceSize = max(static_cast<size_t>(1), evidenceSize);
    mbrSize = min(mbrSize,  evidenceSize);

    //Construct hypothesis set
    GainFunctionVector gHypothesisSet;
    for (size_t i = 0; i < mbrSize; ++i) {
      gHypothesisSet.push_back(new SentenceBLEU(4, *(topNTranslations[i].first))); //Calc the sufficient statistics for the translation
    }
    
    //Construct randomly sampled evidence set
    vector<pair<const Translation*, float> > randomEvidenceSet(evidenceSize);
    random_sample(topNTranslations.begin(), topNTranslations.end(), randomEvidenceSet.begin(), randomEvidenceSet.end());
    for (size_t i = 0; i < evidenceSize; ++i) {
      VERBOSE(1, "random evidence translation: " <<   ToString(*(randomEvidenceSet[i].first)) << " " << randomEvidenceSet[i].second << endl);
    }

    GainFunctionVector gRandomEvidenceSet;
    for (size_t i = 0; i < evidenceSize; ++i) {
      gRandomEvidenceSet.push_back(new SentenceBLEU(4, *(randomEvidenceSet[i].first))); //Calc the sufficient statistics for the translation
    }
    
    //Main random evidence set MBR computation done here
    float bleu(0.0), weightedLoss(0.0), weightedLossCumul(0.0), minMBRLoss(100000);
    int minMBRLossIdx(-1);
    VERBOSE(0, "MBR SIZE " << mbrSize << ", evidence Size " << evidenceSize << endl);
  
    //Outer loop using only the top #mbrSize samples 
    for(size_t i = 0; i < mbrSize; ++i) {
      weightedLossCumul = 0.0;
      const GainFunction& gf = gHypothesisSet[i];
      VERBOSE(1, "Reference " << ToString(*topNTranslations[i].first) << endl);
      for(size_t j = 0; j < evidenceSize; ++j) {//Inner loop using samples in evidence set
        bleu = gf.ComputeGain(gRandomEvidenceSet[j]);
        VERBOSE(2, "Hypothesis " << ToString(*randomEvidenceSet[j].first) << endl);
        weightedLoss = (1- bleu) * randomEvidenceSet[j].second;
        VERBOSE(2, "Bleu " << bleu << ", prob " <<  randomEvidenceSet[j].second << ", weightedLoss : " << weightedLoss << endl);
        weightedLossCumul += weightedLoss;
        if (weightedLossCumul > minMBRLoss)
          break;
      }
      VERBOSE(2, "Bayes risk for cand " << i << " " <<  weightedLossCumul << endl);
      if (weightedLossCumul < minMBRLoss){
        minMBRLoss = weightedLossCumul;
        minMBRLossIdx = i;
      }
    }
    VERBOSE(1, "Minimum Bayes risk cand for sampled evidence set is " <<  minMBRLossIdx << " with risk " << minMBRLoss << endl);
    assert(minMBRLossIdx > -1);
    best.push_back(topNTranslations[minMBRLossIdx]);

    //Now do MBR for evidence set ranked by true p(e|f)
    ProbGreaterThan<Translation> comparator;
    vector<pair<const Translation*, float> > topNTranslationsByPEF = topNTranslations;
    nth_element(topNTranslationsByPEF.begin(), topNTranslationsByPEF.begin() + evidenceSize, topNTranslationsByPEF.end(), comparator);
    
    for (it = topNTranslationsByPEF.begin(); it < topNTranslationsByPEF.begin() + evidenceSize ; ++it) {
      VERBOSE(1, "retained top translation: " <<   ToString(*it->first) << " " << (it->second) << endl);
    }
    for (it = topNTranslationsByPEF.begin() + evidenceSize; it != topNTranslationsByPEF.end() ; ++it) {
      VERBOSE(1, "discarded translations: " <<   ToString(*it->first) << " " << (it->second) << endl);
    }

    
    //Create Evidence Set
    GainFunctionVector gTopNEvidenceSet;
    for (size_t i = 0; i < evidenceSize; ++i) {
      gTopNEvidenceSet.push_back(new SentenceBLEU(4, *(topNTranslationsByPEF[i].first))); //Calc the sufficient statistics for the translation
    }
    
    //Main MBR computation done here
    //Reset variables
    bleu = 0.0;
    weightedLoss = 0.0;
    weightedLossCumul = 0.0;
    minMBRLoss = 100000;
    minMBRLossIdx = -1;
    
    //Outer loop using only the top #mbrSize samples and inner loop using topNByPEF samples
    for(size_t i = 0; i < mbrSize; ++i) {
      weightedLossCumul = 0.0;
      const GainFunction& gf = gHypothesisSet[i];
      VERBOSE(1, "Reference " << ToString(*topNTranslations[i].first) << endl);
      for(size_t j = 0; j < evidenceSize; ++j) {//Inner loop using samples in evidence set
        bleu = gf.ComputeGain(gTopNEvidenceSet[j]);
        VERBOSE(2, "Hypothesis " << ToString(*topNTranslationsByPEF[j].first) << endl);
        weightedLoss = (1- bleu) * topNTranslationsByPEF[j].second;
        VERBOSE(2, "Bleu " << bleu << ", prob " <<  topNTranslationsByPEF[j].second << ", weightedLoss : " << weightedLoss << endl);
        weightedLossCumul += weightedLoss;
        if (weightedLossCumul > minMBRLoss)
          break;
      }
      VERBOSE(2, "Bayes risk for cand " << i << " " <<  weightedLossCumul << endl);
      if (weightedLossCumul < minMBRLoss){
        minMBRLoss = weightedLossCumul;
        minMBRLossIdx = i;
      }
    }
    VERBOSE(1, "Minimum Bayes risk cand for sampled evidence set is " <<  minMBRLossIdx << " with risk " << minMBRLoss << endl);
    assert(minMBRLossIdx > -1);
    best.push_back(topNTranslations[minMBRLossIdx]);
    
    
  }
  
  /**** Algorithm: Run exact decoder for given input. Then compute exact p(e|f) for elements in evidence set.  
   Then perform MBR a) by using a sample from the evidence set b) by sorting evidence set and discarding low prob translations.
   Recompute p(e|f) estimating it from sample set.
  ****/
  void GibblerMaxTransDecoder::getExactMbrSamplerDistro(const std::string& line, std::vector< std::pair<const Translation*,float> > & best, float shrinkEvidenceRatio, size_t mbrSize, size_t topNsize)  {
    //Posterior probs computed using the whole evidence set
    //MBR decoding outer loop using configurable size
    vector<pair<const Translation*, float> > topNTranslations;
    getNbest(topNTranslations,topNsize);
    
    topNsize = topNTranslations.size(); //size of full evidence set
    size_t evidenceSize = topNsize * shrinkEvidenceRatio; //New size of evidence set
    evidenceSize = max(static_cast<size_t>(1), evidenceSize);
    mbrSize = min(mbrSize,  evidenceSize);

    //Construct hypothesis set
    GainFunctionVector gHypothesisSet;
    for (size_t i = 0; i < mbrSize; ++i) {
      gHypothesisSet.push_back(new SentenceBLEU(4, *(topNTranslations[i].first))); //Calc the sufficient statistics for the translation
    }
    
    //Construct randomly sampled evidence set
    vector<pair<const Translation*, float> > randomEvidenceSet(evidenceSize);
    random_sample(topNTranslations.begin(), topNTranslations.end(), randomEvidenceSet.begin(), randomEvidenceSet.end());
    for (size_t i = 0; i < evidenceSize; ++i) {
      VERBOSE(1, "random evidence translation: " <<   ToString(*(randomEvidenceSet[i].first)) << " " << randomEvidenceSet[i].second << endl);
    }

    GainFunctionVector gRandomEvidenceSet;
    for (size_t i = 0; i < evidenceSize; ++i) {
      gRandomEvidenceSet.push_back(new SentenceBLEU(4, *(randomEvidenceSet[i].first))); //Calc the sufficient statistics for the translation
    }
    
    //Main random evidence set MBR computation done here
    float bleu(0.0), weightedLoss(0.0), weightedLossCumul(0.0), minMBRLoss(100000);
    int minMBRLossIdx(-1);
    VERBOSE(0, "MBR SIZE " << mbrSize << ", evidence Size " << evidenceSize << endl);
  
    //Outer loop using only the top #mbrSize samples 
    for(size_t i = 0; i < mbrSize; ++i) {
      weightedLossCumul = 0.0;
      const GainFunction& gf = gHypothesisSet[i];
      VERBOSE(1, "Reference " << ToString(*topNTranslations[i].first) << endl);
      for(size_t j = 0; j < evidenceSize; ++j) {//Inner loop using samples in evidence set
        bleu = gf.ComputeGain(gRandomEvidenceSet[j]);
        VERBOSE(2, "Hypothesis " << ToString(*randomEvidenceSet[j].first) << endl);
        weightedLoss = (1- bleu) * randomEvidenceSet[j].second;
        VERBOSE(2, "Bleu " << bleu << ", prob " <<  randomEvidenceSet[j].second << ", weightedLoss : " << weightedLoss << endl);
        weightedLossCumul += weightedLoss;
        if (weightedLossCumul > minMBRLoss)
          break;
      }
      VERBOSE(2, "Bayes risk for cand " << i << " " <<  weightedLossCumul << endl);
      if (weightedLossCumul < minMBRLoss){
        minMBRLoss = weightedLossCumul;
        minMBRLossIdx = i;
      }
    }
    VERBOSE(1, "Minimum Bayes risk cand for sampled evidence set is " <<  minMBRLossIdx << " with risk " << minMBRLoss << endl);
    assert(minMBRLossIdx > -1);
    best.push_back(topNTranslations[minMBRLossIdx]);


    //Now do MBR discarding low probability translations
    //First Get unpruned lattice for input
    auto_ptr<KLDecoder> klDecoder;
    klDecoder.reset(new KLDecoder); 
    double trueZ = klDecoder->GetZ(line); //Lattice Z
    
    //back up sampler estimated distro in a convenient data structure
    vector<pair<const Translation*, float> >::iterator it; 
    map<const Translation*, float> topNTranslationsSamplerDistro;
    for (it = topNTranslations.begin(); it != topNTranslations.end(); ++it) {
      topNTranslationsSamplerDistro[it->first] = it->second;
    }

    //Now calculate true p(e|f) for all translations samples
    VERBOSE(1,"Hypothesis set: " << endl);
    for (it = topNTranslations.begin(); it != topNTranslations.end(); ++it) {
      float trueTransScore = exp(klDecoder->GetTranslationScore(*it->first) - trueZ);;
      VERBOSE(1, "translation: " <<   ToString(*it->first) << " " << (it->second) << " " << trueTransScore << endl);
      it->second = trueTransScore;
    }
    
    //Now do MBR for evidence set ranked by true p(e|f)
    ProbGreaterThan<Translation> comparator;
    vector<pair<const Translation*, float> > topNTranslationsByPEF = topNTranslations;
    nth_element(topNTranslationsByPEF.begin(), topNTranslationsByPEF.begin() + evidenceSize, topNTranslationsByPEF.end(), comparator);
    
    for (it = topNTranslationsByPEF.begin(); it < topNTranslationsByPEF.begin() + evidenceSize ; ++it) {
      VERBOSE(1, "retained top translation: " <<   ToString(*it->first) << " " << (it->second) << endl);
    }
    for (it = topNTranslationsByPEF.begin() + evidenceSize; it != topNTranslationsByPEF.end() ; ++it) {
      VERBOSE(1, "discarded translations: " <<   ToString(*it->first) << " " << (it->second) << endl);
    }

    //Set evidence set's distribution - this requires 2 passes, first calculate new denominator, then renormalize 
    double totalScore = 0.0;
    for (size_t i = 0; i < topNTranslationsByPEF.size(); ++i) {
       double samplerScore =  topNTranslationsSamplerDistro[topNTranslationsByPEF[i].first];
       totalScore += samplerScore;
       topNTranslationsByPEF[i].second = samplerScore;
    }
    
    for (size_t i = 0; i < topNTranslationsByPEF.size(); ++i) {
       topNTranslationsByPEF[i].second /= totalScore;
    }
    
    //Create Evidence Set
    GainFunctionVector gTopNEvidenceSet;
    for (size_t i = 0; i < evidenceSize; ++i) {
      gTopNEvidenceSet.push_back(new SentenceBLEU(4, *(topNTranslationsByPEF[i].first))); //Calc the sufficient statistics for the translation
    }
    
    //Main MBR computation done here
    //Reset variables
    bleu = 0.0;
    weightedLoss = 0.0;
    weightedLossCumul = 0.0;
    minMBRLoss = 100000;
    minMBRLossIdx = -1;
    
    //Outer loop using only the top #mbrSize samples and inner loop using topNByPEF samples
    for(size_t i = 0; i < mbrSize; ++i) {
      weightedLossCumul = 0.0;
      const GainFunction& gf = gHypothesisSet[i];
      VERBOSE(1, "Reference " << ToString(*topNTranslations[i].first) << endl);
      for(size_t j = 0; j < evidenceSize; ++j) {//Inner loop using samples in evidence set
        bleu = gf.ComputeGain(gTopNEvidenceSet[j]);
        VERBOSE(2, "Hypothesis " << ToString(*topNTranslationsByPEF[j].first) << endl);
        weightedLoss = (1- bleu) * topNTranslationsByPEF[j].second;
        VERBOSE(2, "Bleu " << bleu << ", prob " <<  topNTranslationsByPEF[j].second << ", weightedLoss : " << weightedLoss << endl);
        weightedLossCumul += weightedLoss;
        if (weightedLossCumul > minMBRLoss)
          break;
      }
      VERBOSE(2, "Bayes risk for cand " << i << " " <<  weightedLossCumul << endl);
      if (weightedLossCumul < minMBRLoss){
        minMBRLoss = weightedLossCumul;
        minMBRLossIdx = i;
      }
    }
    VERBOSE(1, "Minimum Bayes risk cand for sampled evidence set is " <<  minMBRLossIdx << " with risk " << minMBRLoss << endl);
    assert(minMBRLossIdx > -1);
    best.push_back(topNTranslations[minMBRLossIdx]);
    
    
  }

  
}






