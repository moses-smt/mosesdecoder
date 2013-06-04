//

#ifndef moses_SyntacticLanguageModelState_h
#define moses_SyntacticLanguageModelState_h

#include "nl-iomacros.h"
#include "nl-cpt.h"
#include "nl-hmm.h"

#include "SyntacticLanguageModelFiles.h"
#include "moses/FF/FFState.h"
#include <string>

namespace Moses
{

template <class MY, class MX, class YS=typename MY::RandVarType, class B=NullBackDat<typename MY::RandVarType> >
class SyntacticLanguageModelState : public FFState
{
public:

  // Initialize an empty LM state
  SyntacticLanguageModelState( SyntacticLanguageModelFiles<MY,MX>* modelData, int beamSize );

  // Get the next LM state from an existing LM state and the next word
  SyntacticLanguageModelState( const SyntacticLanguageModelState* prev, std::string word );


  ~SyntacticLanguageModelState() {
    VERBOSE(3,"Destructing SyntacticLanguageModelState" << std::endl);
    delete randomVariableStore;
  }

  virtual int Compare(const FFState& other) const;

  // Get the LM score from this LM state
  double getScore() const;

  double getProb() const;

private:

  void setScore(double score);
  void printRV();

  SafeArray1D<Id<int>,pair<YS,LogProb> >* randomVariableStore;
  double prob;
  double score;
  int beamSize;
  SyntacticLanguageModelFiles<MY,MX>* modelData;
  bool sentenceStart;
};


////////////////////////////////////////////////////////////////////////////////


template <class MY, class MX, class YS, class B>
void SyntacticLanguageModelState<MY,MX,YS,B>::printRV()
{

  cerr << "*********** BEGIN printRV() ******************" << endl;
  int size=randomVariableStore->getSize();
  cerr << "randomVariableStore->getSize() == " << size << endl;

  for (int depth=0; depth<size; depth+=1) {


    const pair<YS,LogProb> *data = &(randomVariableStore->get(depth));
    std::cerr << "randomVariableStore[" << depth << "]\t" << data->first << "\tprob = " << data->second.toProb() << "\tlogProb = " << double(data->second.toInt())/100 << std::endl;

  }
  cerr << "*********** END printRV() ******************" << endl;

}

// Initialize an empty LM state from grammar files
//
//    nArgs is the number of model files
//    argv is the list of model file names
//
template <class MY, class MX, class YS, class B>
SyntacticLanguageModelState<MY,MX,YS,B>::SyntacticLanguageModelState( SyntacticLanguageModelFiles<MY,MX>* modelData, int beamSize )
{

  this->randomVariableStore = new SafeArray1D<Id<int>,pair<YS,LogProb> >();
  this->modelData = modelData;
  this->beamSize = beamSize;

  // Initialize an empty random variable value
  YS xBEG;
  StringInput(String(BEG_STATE).c_array())>>xBEG>>"\0";
  cerr<<xBEG<<"\n";

  //  cout << "Examining RV store just before RV init" << endl;
  //printRV();

  // Initialize the random variable store
  this->randomVariableStore->init(1,pair<YS,LogProb>(xBEG,0));

  this->sentenceStart = true;

  IFVERBOSE(3) {
    VERBOSE(3,"Examining RV store just after RV init" << endl);
    printRV();
  }

  // Get score of final frame in HHMM
  LogProb l(1.0);
  //score = l.toDouble();
  setScore(l.toDouble());
  //  MY::F_ROOT_OBS = true;
// this->modelData->getHiddenModel()->setRootObs(true);


}


template <class MY, class MX, class YS, class B>
int SyntacticLanguageModelState<MY,MX,YS,B>::Compare(const FFState& other) const
{
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
SyntacticLanguageModelState<MY,MX,YS,B>::SyntacticLanguageModelState( const SyntacticLanguageModelState* prev, std::string word )
{

  // Initialize member variables
  this->randomVariableStore = new SafeArray1D<Id<int>,pair<YS,LogProb> >();
  this->modelData = prev->modelData;
  this->beamSize = prev->beamSize;
  this->randomVariableStore->init(this->beamSize);
  this->sentenceStart=false;

  YS ysEND;
  StringInput(String(END_STATE).c_array())>>ysEND>>"\0";

  // Get HHMM model files
  MY& mH = *(modelData->getHiddenModel());
  MX& mO = *(modelData->getObservedModel());

  // Initialize HHMM
  HMM<MY,MX,YS,B> hmm(mH,mO);
  int MAX_WORDS  = 2;
  hmm.init(MAX_WORDS,this->beamSize,prev->randomVariableStore);
  typename MX::RandVarType x(word.c_str());
  //  cout << "Examining HHMM just after hmm.init" << endl;
  //  hmm.debugPrint();


  /*  cerr << "*********** BEGIN writeCurr() ******************" << endl;
  hmm.writeCurr(cout,0);
  hmm.writeCurr(cout,1);
  cerr << "*********** END writeCurr() ******************" << endl;
  */
  /*
    {

    int wnum=1;
    list<TrellNode<YS,B> > lys = hmm.getMLSnodes(ysEND);                                           // get mls list
        for ( typename list<TrellNode<YS,B> >::iterator i=lys.begin(); i!=lys.end(); i++, wnum++ ) {   // for each frame
          cout << "HYPOTH " << wnum
               << " " << i->getBackData()
               << " " << x
               << " " << i->getId()
               << " (" << i->getLogProb() << ")"
               << endl;                                                                             //   print RV val
        }
    }
    */


  /*
  cerr << "Writing hmm.writeCurr" << endl;
  hmm.writeCurr(cerr,0);
  hmm.writeCurr(cerr,1);
  cerr << "...done writing hmm.writeCurr" << endl;
  */
  hmm.getCurrSum();



  // Initialize observed variable
  //  typename MX::RandVarType ov;
  //  ov.set(word.c_str(),mO);
  //  MY::WORD = ov.getW();
  //bool endOfSentence = prev->sentenceStart;//true;

  //  std::cerr << "About to give HHMM a word of input:\t" << word << std::endl;

  hmm.updateRanked(x, prev->sentenceStart);

  //  cout << "Examining HHMM just after hmm.updateRanked(" << x << "," << prev->sentenceStart << ")" << endl;
  //  hmm.debugPrint();
  /*
    cerr << "*********** BEGIN writeCurr() ******************" << endl;
    hmm.writeCurr(cout,0);
    hmm.writeCurr(cout,1);
    cerr << "*********** END writeCurr() ******************" << endl;
    */
  /*
  {

    int wnum=1;
    list<TrellNode<YS,B> > lys = hmm.getMLSnodes(ysEND);                                           // get mls list
        for ( typename list<TrellNode<YS,B> >::iterator i=lys.begin(); i!=lys.end(); i++, wnum++ ) {   // for each frame
          cout << "HYPOTH " << wnum
               << " " << i->getBackData()
               << " " << x
               << " " << i->getId()
               << " (" << i->getLogProb() << ")"
               << endl;                                                                             //   print RV val
        }
    }
    */
//  X ov(word.c_str());
  //mH.setWord(ov);
  // MY::WORD = ov;//ov.getW();

  // Update HHMM based on observed variable
  //hmm.updateRanked(ov);
  //mH.setRootObs(true);
  //MY::F_ROOT_OBS = false;

  // Get the current score
  double currSum = hmm.getCurrSum();
  //VERBOSE(3,"Setting score using currSum for " << scientific << x << " = " << currSum << endl);
  setScore(currSum);
  //  cout << "Examining RV store just before RV init via gatherElementsInBeam" << endl;
  //  printRV();

  // Get new hidden random variable store from HHMM
  hmm.gatherElementsInBeam(randomVariableStore);
  //  cout << "Examining RV store just after RV init via gatherElementsInBeam" << endl;
  //  printRV();
  /*
  cerr << "Writing hmm.writeCurr..." << endl;
  hmm.writeCurr(cerr,0);
  hmm.writeCurr(cerr,1);
  cerr << "...done writing hmm.writeCurr" << endl;
  */
}


template <class MY, class MX, class YS, class B>
double SyntacticLanguageModelState<MY,MX,YS,B>::getProb() const
{

  return prob;
}

template <class MY, class MX, class YS, class B>
double SyntacticLanguageModelState<MY,MX,YS,B>::getScore() const
{

  return score;
}


template <class MY, class MX, class YS, class B>
void SyntacticLanguageModelState<MY,MX,YS,B>::setScore(double score)
{




  this->prob = score;

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

  // Check for score==0 to avoid causing -infinity with log(score)
  if (score==0) {
    this->score = -100;
  } else {
    double x = log(score) / 7.44440071921381;
    if ( x >= -100) {
      this->score = x;
    } else {
      this->score = -100;
    }
  }

  VERBOSE(3,"\tSyntacticLanguageModelState has score=" << this->score << endl);

}


}

#endif
