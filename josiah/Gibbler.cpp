#include "Gibbler.h"
#include "Derivation.h"
#include "Hypothesis.h"
#include "TranslationOptionCollection.h"
#include "GibblerMaxTransDecoder.h"
#include "StaticData.h"
#include "AnnealingSchedule.h"
#include "GibbsOperator.h"

using namespace std;

namespace Josiah {



Sample::Sample(Hypothesis* target_head, const std::vector<Word>& source, const FeatureVector& features, bool doRaoBlackwell) : 
        m_sourceWords(source),   m_doRaoBlackwell(doRaoBlackwell), m_updates(0) { 
  std::map<int, Hypothesis*> source_order;
  this->target_head = target_head;
  Hypothesis* next = NULL;

  for (Hypothesis* h = target_head; h; h = const_cast<Hypothesis*>(h->GetPrevHypo())) {
    size_t startPos = h->GetCurrSourceWordsRange().GetStartPos();
    SetSourceIndexedHyps(h); 
    if (h->GetPrevHypo()){
      source_order[startPos] = h;  
    }
    else {
      source_order[-1] = h;  
    }
    this->target_tail = h;
    h->SetNextHypo(next);
    next = h;
  }
  
  std::map<int, Hypothesis*>::const_iterator source_it = source_order.begin();
  Hypothesis* prev = NULL;
  this->source_tail = source_it->second;
  
  
  for (; source_it != source_order.end(); source_it++) {
    Hypothesis *h = source_it->second;  
    h->SetSourcePrevHypo(prev);
    if (prev != NULL) 
      prev->SetSourceNextHypo(h);
    this->source_head = h;
    prev = h;
  }
  
  this->source_head->SetSourceNextHypo(NULL);
  this->target_head->SetNextHypo(NULL);
  
  this->source_tail->SetSourcePrevHypo(NULL);
  this->target_tail->SetPrevHypo(NULL);
  
  for (FeatureVector::const_iterator i=features.begin(); i!=features.end(); ++i){
    // tell the feature that we have a new sample
    m_featureFunctions.push_back((*i)->getFunction(*this));
  }
  
  UpdateTargetWords();
      
  for (FeatureFunctionVector::const_iterator i=m_featureFunctions.begin(); i!=m_featureFunctions.end(); ++i){
    (*i)->assignScore(feature_values);
  }
      
}
 
Sample::~Sample() {
  RemoveAllInColl(cachedSampledHyps);
}


Hypothesis* Sample::CreateHypothesis(Hypothesis& prevTarget, const TranslationOption& option) {
  UpdateCoverageVector(prevTarget, option);
  
  Hypothesis* hypo = Hypothesis::Create(prevTarget, option, NULL);
  prevTarget.SetNextHypo(hypo);
  cachedSampledHyps.insert(hypo);
  SetSourceIndexedHyps(hypo);
  //SetTgtIndexedHyps(hypo);
  return hypo;
}


void Sample::UpdateTargetWords()  {
  m_targetWords.clear();
  const Hypothesis* currHypo = GetTargetTail(); //target tail
  
  
  IFVERBOSE(2) {
    VERBOSE(2,"Sentence: ");
  }
  
  //we're now at the dummy hypo at the start of the sentence
  while ((currHypo = (currHypo->GetNextHypo()))) {
    const TargetPhrase& targetPhrase = currHypo->GetCurrTargetPhrase();
    for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
      m_targetWords.push_back(targetPhrase.GetWord(i));
      IFVERBOSE(2) {
        VERBOSE(2,targetPhrase.GetWord(i) << " ");  
      }  
    }
    IFVERBOSE(2) {
      if (currHypo->GetCurrTargetPhrase().GetSize() > 0) {
        VERBOSE(2, "|" << currHypo->GetCurrSourceWordsRange().GetStartPos()
                << "-" << currHypo->GetCurrSourceWordsRange().GetEndPos() << "| ");    
      }
    }
  }
  IFVERBOSE(2) {
    VERBOSE(2,endl);
  }
  
  IFVERBOSE(2) {
    VERBOSE(2,"FVs: " << feature_values << endl);
  }
  //Inform the extra features that the target words have changed
  for (FeatureFunctionVector::const_iterator i=m_featureFunctions.begin(); i!=m_featureFunctions.end(); ++i){
    (*i)->updateTarget();
  }
}

  
Hypothesis* Sample::GetHypAtSourceIndex(size_t i)  {
  std::map<size_t, Hypothesis*>::iterator it = sourceIndexedHyps.find(i);
  if (it == sourceIndexedHyps.end())
    return NULL;
  return it->second;
}
 
  
void Sample::SetSourceIndexedHyps(Hypothesis* h) {
  
  size_t startPos = h->GetCurrSourceWordsRange().GetStartPos();
  size_t endPos = h->GetCurrSourceWordsRange().GetEndPos();
  if (startPos + 1 == 0 ) {
    sourceIndexedHyps[startPos] = h; 
    return;
  }
  for (size_t i = startPos; i <= endPos; i++) {
    sourceIndexedHyps[i] = h; 
  } 
}
  
  
void Sample::SetTgtNextHypo(Hypothesis* newHyp, Hypothesis* currNextHypo) {
  if (newHyp) {
    newHyp->SetNextHypo(currNextHypo);  
  }
    
  if (currNextHypo) {
    currNextHypo->SetPrevHypo(newHyp);  
  }
}
  
void Sample::SetSrcPrevHypo(Hypothesis* newHyp, Hypothesis* srcPrevHypo) {
  if (newHyp) {
    newHyp->SetSourcePrevHypo(srcPrevHypo); 
  }
    
  if (srcPrevHypo) {
    srcPrevHypo->SetSourceNextHypo(newHyp);  
  }
}  
  
void Sample::FlipNodes(const TranslationOption& leftTgtOption, const TranslationOption& rightTgtOption, Hypothesis* m_prevTgtHypo, Hypothesis* m_nextTgtHypo, const FVector& deltaFV) {
  bool tgtSideContiguous = false; 
  Hypothesis *oldRightHypo = GetHypAtSourceIndex(leftTgtOption.GetSourceWordsRange().GetStartPos()); //this one used to be on the right
  Hypothesis *oldLeftHypo = GetHypAtSourceIndex(rightTgtOption.GetSourceWordsRange().GetStartPos());//this one used to be on the left
  
  //created the new left most tgt
  Hypothesis *newLeftHypo = CreateHypothesis(*m_prevTgtHypo, leftTgtOption);
  //are the options contiguous on the target side?
  Hypothesis *tgtSidePredecessor = const_cast<Hypothesis*>(oldRightHypo->GetPrevHypo()); //find its target side predecessor
  //If the flip is contiguous on the target side, then the predecessor is the flipped one 
  if (tgtSidePredecessor->GetCurrSourceWordsRange() == rightTgtOption.GetSourceWordsRange()) {
    tgtSidePredecessor = newLeftHypo;
    tgtSideContiguous = true;
  }
  //update the target side sample pointers now 
  if  (!tgtSideContiguous) {
    Hypothesis *leftHypoTgtSideSuccessor = const_cast<Hypothesis*>(oldLeftHypo->GetNextHypo());
    SetTgtNextHypo(newLeftHypo, leftHypoTgtSideSuccessor);
  }
  
  //update the target word ranges of the ones in between
  if (!tgtSideContiguous) {
    size_t startTgtPos = newLeftHypo->GetCurrTargetWordsRange().GetEndPos();
    for (Hypothesis *h = const_cast<Hypothesis*>(oldLeftHypo->GetNextHypo()); h != oldRightHypo ; h = const_cast<Hypothesis*>(h->GetNextHypo())) {
     WordsRange& range = h->GetCurrTargetWordsRange();
     size_t size = range.GetNumWordsCovered();
     range.SetStartPos(startTgtPos+1);
     range.SetEndPos(startTgtPos+size);
     startTgtPos += size;
    }
  }

  //now create the one that goes on the right 
  Hypothesis *newRightHypo = CreateHypothesis(*tgtSidePredecessor, rightTgtOption);
  SetTgtNextHypo(newRightHypo, m_nextTgtHypo);
  
  //update the source side sample pointers now 
  Hypothesis* newLeftSourcePrevHypo = GetHypAtSourceIndex(newLeftHypo->GetCurrSourceWordsRange().GetStartPos() - 1 );
  Hypothesis* newLeftSourceNextHypo = GetHypAtSourceIndex(newLeftHypo->GetCurrSourceWordsRange().GetEndPos() + 1 );
  
  SetSrcPrevHypo(newLeftHypo, newLeftSourcePrevHypo);
  SetSrcPrevHypo(newLeftSourceNextHypo, newLeftHypo);
  
  Hypothesis* newRightSourcePrevHypo = GetHypAtSourceIndex(newRightHypo->GetCurrSourceWordsRange().GetStartPos() - 1 );
  Hypothesis* newRightSourceNextHypo = GetHypAtSourceIndex(newRightHypo->GetCurrSourceWordsRange().GetEndPos() + 1 );

  SetSrcPrevHypo(newRightHypo, newRightSourcePrevHypo);
  SetSrcPrevHypo(newRightSourceNextHypo, newRightHypo);

  UpdateHead(oldRightHypo, newLeftHypo, source_head);
  UpdateHead(oldLeftHypo, newRightHypo, source_head);
  UpdateHead(oldRightHypo, newRightHypo, target_head);
  UpdateHead(oldLeftHypo, newRightHypo, target_head);
 
  UpdateFeatureValues(deltaFV);
  UpdateTargetWords();
  
  DeleteFromCache(oldRightHypo);
  DeleteFromCache(oldLeftHypo);
  
  //Sanity check
  IFVERBOSE(4) {
    float totalDistortion(0.0);
    for (Hypothesis* h = target_tail; h; h = const_cast<Hypothesis*>(h->GetNextHypo())) {
      Hypothesis *next = const_cast<Hypothesis*>(h->GetNextHypo());
      if (next) {
        totalDistortion += ComputeDistortionDistance(h->GetCurrSourceWordsRange(), next->GetCurrSourceWordsRange());   
      }
      else {
        break;
      }
    }
    VERBOSE(4, "Total distortion for this sample " << totalDistortion << endl);
  } 
  
}

  float Sample::ComputeDistortionDistance(const WordsRange& prev, const WordsRange& current) 
  {
    int dist = 0;
    if (prev.GetNumWordsCovered() == 0) {
      dist = current.GetStartPos();
    } else {
      dist = (int)prev.GetEndPos() - (int)current.GetStartPos() + 1 ;
    }
    return - (float) abs(dist);
  }  
  
void Sample::ChangeTarget(const TranslationOption& option, const FVector& deltaFV)  {
  size_t optionStartPos = option.GetSourceWordsRange().GetStartPos();
  Hypothesis *currHyp = GetHypAtSourceIndex(optionStartPos);
  Hypothesis& prevHyp = *(const_cast<Hypothesis*>(currHyp->GetPrevHypo()));

  Hypothesis *newHyp = CreateHypothesis(prevHyp, option);
  SetTgtNextHypo(newHyp, const_cast<Hypothesis*>(currHyp->GetNextHypo()));
  UpdateHead(currHyp, newHyp, target_head);
  
  SetSrcPrevHypo(newHyp, const_cast<Hypothesis*>(currHyp->GetSourcePrevHypo()));
  SetSrcPrevHypo(const_cast<Hypothesis*>(currHyp->GetSourceNextHypo()), newHyp);
  UpdateHead(currHyp, newHyp, source_head);
  
  //Update target word ranges
  int tgtSizeChange = static_cast<int> (option.GetTargetPhrase().GetSize()) - static_cast<int> (currHyp->GetTargetPhrase().GetSize());
  if (tgtSizeChange != 0) {
    UpdateTargetWordRange(newHyp, tgtSizeChange);  
  }
  
  DeleteFromCache(currHyp);
  UpdateFeatureValues(deltaFV);
  UpdateTargetWords();
}  

void Sample::MergeTarget(const TranslationOption& option, const FVector& deltaFV)  {
  size_t optionStartPos = option.GetSourceWordsRange().GetStartPos();
  size_t optionEndPos = option.GetSourceWordsRange().GetEndPos();
  
  Hypothesis *currStartHyp = GetHypAtSourceIndex(optionStartPos);
  Hypothesis *currEndHyp = GetHypAtSourceIndex(optionEndPos);
  
  assert(currStartHyp != currEndHyp);
  
  Hypothesis* prevHyp = NULL;
  Hypothesis* newHyp = NULL;
  
  if (currStartHyp->GetCurrTargetWordsRange() < currEndHyp->GetCurrTargetWordsRange()) {
    prevHyp = const_cast<Hypothesis*> (currStartHyp->GetPrevHypo());
    newHyp = CreateHypothesis(*prevHyp, option);
    
    //Set the target ptrs
    SetTgtNextHypo(newHyp, const_cast<Hypothesis*>(currEndHyp->GetNextHypo()));
    UpdateHead(currEndHyp, newHyp, target_head);
  } 
  else {
    prevHyp = const_cast<Hypothesis*> (currEndHyp->GetPrevHypo());
    newHyp = CreateHypothesis(*prevHyp, option);
    
    SetTgtNextHypo(newHyp, const_cast<Hypothesis*>(currStartHyp->GetNextHypo()));
    UpdateHead(currStartHyp, newHyp, target_head);
  }
  
  //Set the source ptrs
  SetSrcPrevHypo(newHyp, const_cast<Hypothesis*>(currStartHyp->GetSourcePrevHypo()));
  SetSrcPrevHypo(const_cast<Hypothesis*>(currEndHyp->GetSourceNextHypo()), newHyp);
  UpdateHead(currEndHyp, newHyp, source_head);
                    
  //Update target word ranges
  int newTgtSize = option.GetTargetPhrase().GetSize();
  int prevTgtSize = currStartHyp->GetTargetPhrase().GetSize() + currEndHyp->GetTargetPhrase().GetSize();
  int tgtSizeChange = newTgtSize - prevTgtSize;
  if (tgtSizeChange != 0) {
    UpdateTargetWordRange(newHyp, tgtSizeChange);  
  }
  
  DeleteFromCache(currStartHyp);
  DeleteFromCache(currEndHyp);
  
  UpdateFeatureValues(deltaFV);
  UpdateTargetWords();
}
  
void Sample::SplitTarget(const TranslationOption& leftTgtOption, const TranslationOption& rightTgtOption,  const FVector& deltaFV) {
  size_t optionStartPos = leftTgtOption.GetSourceWordsRange().GetStartPos();
  Hypothesis *currHyp = GetHypAtSourceIndex(optionStartPos);
  
  Hypothesis& prevHyp = *(const_cast<Hypothesis*>(currHyp->GetPrevHypo()));
  Hypothesis *newLeftHyp = CreateHypothesis(prevHyp, leftTgtOption);
  Hypothesis *newRightHyp = CreateHypothesis(*newLeftHyp, rightTgtOption);
  
  //Update tgt ptrs
  SetTgtNextHypo(newRightHyp, const_cast<Hypothesis*>(currHyp->GetNextHypo()));
  UpdateHead(currHyp, newRightHyp, target_head);
  
  //Update src ptrs
  assert (newLeftHyp->GetCurrSourceWordsRange() < newRightHyp->GetCurrSourceWordsRange()); //monotone  
  SetSrcPrevHypo(newLeftHyp, const_cast<Hypothesis*>(currHyp->GetSourcePrevHypo()));
  SetSrcPrevHypo(newRightHyp, newLeftHyp);
  SetSrcPrevHypo(const_cast<Hypothesis*>(currHyp->GetSourceNextHypo()), newRightHyp); 
  UpdateHead(currHyp, newRightHyp, source_head);
    
  //Update target word ranges
  int prevTgtSize = currHyp->GetTargetPhrase().GetSize();
  int newTgtSize = newLeftHyp->GetTargetPhrase().GetSize() + newRightHyp->GetTargetPhrase().GetSize();
  int tgtSizeChange = newTgtSize - prevTgtSize;
  if (tgtSizeChange != 0) {
    UpdateTargetWordRange(newRightHyp, tgtSizeChange);  
  }
  
  DeleteFromCache(currHyp);
  UpdateFeatureValues(deltaFV);
  UpdateTargetWords();
}  
  
void Sample::UpdateHead(Hypothesis* currHyp, Hypothesis* newHyp, Hypothesis *&head) {
  if (head == currHyp)
    head = newHyp;
}
  
void Sample::UpdateTargetWordRange(Hypothesis* hyp, int tgtSizeChange) {
  Hypothesis* nextHyp = const_cast<Hypothesis*>(hyp->GetNextHypo());
  if (!nextHyp)
    return;
    
  for (Hypothesis* h = nextHyp; h; h = const_cast<Hypothesis*>(h->GetNextHypo())){
    WordsRange& range = h->GetCurrTargetWordsRange();
    range.SetStartPos(range.GetStartPos()+tgtSizeChange);
    range.SetEndPos(range.GetEndPos()+tgtSizeChange);
  }
}  
  
void Sample::UpdateFeatureValues(const FVector& deltaFV) {
  feature_values +=deltaFV;
}

void Sample::CheckFeatureConsistency() const {
  FVector expected;
  for (FeatureFunctionVector::const_iterator i=m_featureFunctions.begin(); i!=m_featureFunctions.end(); ++i){
    (*i)->assignScore(expected);
  }
  if (expected != feature_values) {
    VERBOSE(1, "Expected: " << expected << endl);
    VERBOSE(1, "Actual: " << feature_values << endl);
    ostringstream msg;
    msg << "Score mismatch: e-a = " << (expected-feature_values);
    throw runtime_error(msg.str());
  }
}

//update  the bitmap of the predecessor
void Sample::UpdateCoverageVector(Hypothesis& hyp, const TranslationOption& option) {
 size_t startPos = option.GetSourceWordsRange().GetStartPos();
 size_t endPos = option.GetSourceWordsRange().GetEndPos();

 WordsBitmap & wordBitmap = hyp.GetWordsBitmap();
 wordBitmap.SetValue(startPos, endPos, false);
} 
  
void Sample::DeleteFromCache(Hypothesis *hyp) {
  set<Hypothesis*>::iterator it = find(cachedSampledHyps.begin(), cachedSampledHyps.end(), hyp);
  if (it != cachedSampledHyps.end()){
    delete *it;
    cachedSampledHyps.erase(it);
  }
}



bool Sample::DoRaoBlackwell() const {
    return m_doRaoBlackwell;
}

void Sample::AddConditionalFeatureValues( const FVector & fv ) {
    m_conditionalFeatureValues += fv;
    ++m_updates;
}

const FVector Sample::GetConditionalFeatureValues( ) const {
    if (m_doRaoBlackwell) {
        FVector fv(m_conditionalFeatureValues);
        fv /= m_updates;
        return fv;
    } else {
        return GetFeatureValues();
    }
}

void Sample::ResetConditionalFeatureValues(){
    m_updates = 0;
    m_conditionalFeatureValues.clear();
}




}

