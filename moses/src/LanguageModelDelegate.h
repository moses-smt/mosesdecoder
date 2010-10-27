// $Id: LanguageModel.h 3078 2010-04-08 17:16:10Z hieuhoang1972 $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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

#ifndef moses_LanguageModelDelegate_h
#define moses_LanguageModelDelegate_h

#include "LanguageModelSingleFactor.h"

namespace Moses {

//! A language model which delegates all its calculation to another language model.
//! Used when you want to have the same language model with two different weights.
class LanguageModelDelegate: public LanguageModelSingleFactor {
  
  public:
    LanguageModelDelegate(bool registerScore, ScoreIndexManager &scoreIndexManager, LanguageModelSingleFactor* delegate) :
      LanguageModelSingleFactor(registerScore, scoreIndexManager), m_delegate(delegate)
      {
        m_nGramOrder = m_delegate->GetNGramOrder();
        m_factorType = m_delegate->GetFactorType();
        m_sentenceStart = m_delegate->GetSentenceStart();
        m_sentenceEnd = m_delegate->GetSentenceEnd();
        m_sentenceStartArray = m_delegate->GetSentenceStartArray();
        m_sentenceEndArray = m_delegate->GetSentenceEndArray();

      }
      
    bool Load(const std::string &
          , FactorType
          , size_t)
    { 
      /* do nothing */
      return true;
    }

    float GetValueGivenState(const std::vector<const Word*> &contextFactor, FFState &state, unsigned int* len = 0) const
    {
      return m_delegate->GetValueGivenState(contextFactor, state, len);
    }
    float GetValueForgotState(const std::vector<const Word*> &contextFactor, FFState &state, unsigned int* len = 0) const
    {
      return m_delegate->GetValueForgotState(contextFactor, state, len);
    }
    void GetState(const std::vector<const Word*> &contextFactor, FFState &outState) const 
    {
      m_delegate->GetState(contextFactor, outState);
    }
    FFState *NewState(const FFState *from = NULL) const
    {
      return m_delegate->NewState(from);
    }
    
  private:
    LanguageModelSingleFactor* m_delegate;
};

}



#endif
