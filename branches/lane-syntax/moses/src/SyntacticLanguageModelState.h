//

#ifndef moses_SyntacticLanguageModelState_h
#define moses_SyntacticLanguageModelState_h

#include "nl-iomacros.h"
#include "nl-cpt.h"
#include "nl-hmm.h"

#include "SyntacticLanguageModelFiles.h"
#include "FFState.h"
#include <string>

namespace Moses
{

template <class MY, class MX, class YS=typename MY::RandVarType, class B=NullBackDat<typename MY::RandVarType> >
  class SyntacticLanguageModelState : public FFState {
 public:

  // Initialize an empty LM state
  SyntacticLanguageModelState( SyntacticLanguageModelFiles<MY,MX>* modelData, int beamSize );

  // Get the next LM state from an existing LM state and the next word
  SyntacticLanguageModelState( const SyntacticLanguageModelState* prev, std::string word );


 ~SyntacticLanguageModelState() {
   //cerr << "Deleting SyntacticLanguageModelState" << std::endl;
   //delete randomVariableStore;
 }

 virtual int Compare(const FFState& other) const;

  // Get the LM score from this LM state
  double getScore() const;

 private:

 void setScore(double score);

 SafeArray1D<Id<int>,pair<YS,LogProb> >* randomVariableStore;

 double score;
 int beamSize;
 SyntacticLanguageModelFiles<MY,MX>* modelData;

};


////////////////////////////////////////////////////////////////////////////////

  

// Initialize an empty LM state from grammar files
//
//    nArgs is the number of model files
//    argv is the list of model file names
//
template <class MY, class MX, class YS, class B>
  SyntacticLanguageModelState<MY,MX,YS,B>::SyntacticLanguageModelState( SyntacticLanguageModelFiles<MY,MX>* modelData, int beamSize ) {

  this->randomVariableStore = new SafeArray1D<Id<int>,pair<YS,LogProb> >();
  this->modelData = modelData;
  this->beamSize = beamSize;

  // Initialize an empty random variable value
  YS xBEG;
  StringInput(String(BEG_STATE).c_array())>>xBEG>>"\0";
  cerr<<xBEG<<"\n";

  // Initialize the random variable store
  this->randomVariableStore->init(1,pair<YS,LogProb>(xBEG,0));

  // Get score of final frame in HHMM
  LogProb l(1.0);
  //score = l.toDouble();
  setScore(l.toDouble());
  //  MY::F_ROOT_OBS = true;
  this->modelData->getHiddenModel()->setRootObs(true);
  
  
}


template <class MY, class MX, class YS, class B>
  int SyntacticLanguageModelState<MY,MX,YS,B>::Compare(const FFState& other) const {
  /*
  const SyntacticLanguageModelState<MY,MX,YS,B>& o = 
    static_cast<const SyntacticLanguageModelState<MY,MX,YS,B>&>(other);

  if (o.score > score) return 1;
  else if (o.score < score) return -1;
  else return 0;
  */
  return 0;
 }


template <class MY, class MX, class YS, class B>
  SyntacticLanguageModelState<MY,MX,YS,B>::SyntacticLanguageModelState( const SyntacticLanguageModelState* prev, std::string word ) {

  // Initialize member variables
  this->randomVariableStore = new SafeArray1D<Id<int>,pair<YS,LogProb> >();
  this->modelData = prev->modelData;
  this->beamSize = prev->beamSize;

  // Get HHMM model files
  MY& mH = *(modelData->getHiddenModel());
  MX& mO = *(modelData->getObservedModel());
  
  // Initialize HHMM
  HMM<MY,MX,YS,B> hmm(mH,mO);  
  int MAX_WORDS  = 2;
  hmm.init(MAX_WORDS,this->beamSize,prev->randomVariableStore);
  
  // Initialize observed variable
  //  typename MX::RandVarType ov;
  //  ov.set(word.c_str(),mO);
  //  MY::WORD = ov.getW();
  X ov(word.c_str());
  mH.setWord(ov);
  // MY::WORD = ov;//ov.getW();

  // Update HHMM based on observed variable
  hmm.updateRanked(ov);
  mH.setRootObs(true);
  //MY::F_ROOT_OBS = false;

  // Get the current score
  setScore(hmm.getCurrSum());

  // Get new hidden random variable store from HHMM
  hmm.gatherElementsInBeam(randomVariableStore);
  
}


template <class MY, class MX, class YS, class B>
double SyntacticLanguageModelState<MY,MX,YS,B>::getScore() const {
  
  return score;
}


template <class MY, class MX, class YS, class B>
  void SyntacticLanguageModelState<MY,MX,YS,B>::setScore(double score) {

  // We want values to range from -100 to 0
  //
  // If the minimum positive value for a double is min=4.94065645841246544e-324
  //    then to scale, we want a logarithmic base such that log_b(min)=-100
  //
  // -100 = log(min) / log(b)
  //
  // log(b) = log(min) / -100
  //
  // b = exp( log(min) / -100 )
  //
  // b = 7.44440071921381

  this->score = log(score) / 7.44440071921381;

}


}

#endif
