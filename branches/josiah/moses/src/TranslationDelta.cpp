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

#include "TranslationDelta.h"

using namespace std;

namespace Moses {

/**
  Compute the change in language model score by adding this target phrase
  into the hypothesis at the given target position.
  **/
void  TranslationDelta::addLanguageModelScore(const Hypothesis* hypothesis, const Phrase& targetPhrase, const WordsRange& targetSegment) {
    const LMList& languageModels = StaticData::Instance().GetAllLM();
    for (LMList::const_iterator i = languageModels.begin(); i != languageModels.end(); ++i) {
    LanguageModel* lm = *i;
    size_t order = lm->GetNGramOrder();
    vector<const Word*> lmcontext(targetPhrase.GetSize() + 2*(order-1));
    
    //fill in the pre-context
    const Hypothesis* currHypo = hypothesis;
    size_t currHypoPos = 0;
    for (int currPos = order-2; currPos >= 0; --currPos) {
      if (!currHypoPos && currHypo->GetPrevHypo()) {
        //go back if we can
        currHypo = currHypo->GetPrevHypo();
        currHypoPos = currHypo->GetCurrTargetLength()-1;
      } else {
        --currHypoPos;
      }
      if (currHypo->GetPrevHypo()) {
        lmcontext[currPos] = &(currHypo->GetCurrTargetPhrase().GetWord(currHypoPos));
      } else {
        lmcontext[currPos] = &(lm->GetSentenceStartArray());
      }
    }
    
    //fill in the target phrase
    for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
      lmcontext[i + (order-1)] = &(targetPhrase.GetWord(i));
    }
    
    //fill in the postcontext
    currHypo = hypothesis;
    currHypoPos = currHypo->GetCurrTargetLength() - 1;
    size_t eoscount = 0;
    //cout  << "Target phrase: " << targetPhrase <<  endl;
    for (size_t currPos = targetPhrase.GetSize() + (order-1); currPos < lmcontext.size(); ++currPos) {
      //cout << "currPos " << currPos << endl;
      ++currHypoPos;
      if (currHypo && currHypoPos >= currHypo->GetCurrTargetLength()) {
        //next hypo
        currHypo = currHypo->GetNextHypo();
        currHypoPos = 0;
      }
      if (currHypo) {
        assert(currHypo->GetCurrTargetLength());
        lmcontext[currPos] = &(currHypo->GetCurrTargetPhrase().GetWord(currHypoPos));
      } else {
         lmcontext[currPos] = &(lm->GetSentenceEndArray());
         ++eoscount;
      }
    }
    
    //score lm
    cout << "LM context ";
    for (size_t j = 0;  j < lmcontext.size(); ++j) {
      if (j == order-1) {
        cout << "[ ";
      }
      if (j == (targetPhrase.GetSize() + (order-1))) {
        cout << "] ";
      }
      cout << *(lmcontext[j]) << " ";
      
    }
    cout << endl;
    double lmscore = 0;
    vector<const Word*> ngram;
    //remember to only include max of 1 eos marker
    size_t maxend = min(lmcontext.size(), lmcontext.size() - (eoscount-1));
    for (size_t ngramstart = 0; ngramstart < lmcontext.size() - (order -1); ++ngramstart) {
      ngram.clear();
      for (size_t j = ngramstart; j < ngramstart+order && j < maxend; ++j) {
        ngram.push_back(lmcontext[j]);
      }
      lmscore += lm->GetValue(ngram);
    }
    cout << "Language model score: " << lmscore << endl; 
    m_scores.Assign(lm,lmscore);
  }
}

}
