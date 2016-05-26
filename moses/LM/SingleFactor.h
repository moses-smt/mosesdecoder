// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
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

#ifndef moses_LanguageModelSingleFactor_h
#define moses_LanguageModelSingleFactor_h

#include "Implementation.h"
#include "moses/Phrase.h"

namespace Moses
{

class FactorCollection;
class Factor;

//! Abstract class for for single factor LM
class LanguageModelSingleFactor : public LanguageModelImplementation
{
protected:
  typedef const void *State;

  const Factor *m_sentenceStart, *m_sentenceEnd;

  FactorType	m_factorType;
  FFState *m_nullContextState;
  FFState *m_beginSentenceState;

  LanguageModelSingleFactor(const std::string &line);

public:
  virtual ~LanguageModelSingleFactor();
  bool IsUseable(const FactorMask &mask) const;
  virtual void SetParameter(const std::string& key, const std::string& value);

  const Factor *GetSentenceStart() const {
    return m_sentenceStart;
  }
  const Factor *GetSentenceEnd() const {
    return m_sentenceEnd;
  }
  FactorType GetFactorType() const {
    return m_factorType;
  }

  virtual const FFState *GetNullContextState() const;
  virtual const FFState *GetBeginSentenceState() const;
  virtual FFState *NewState(const FFState *from = NULL) const;

  virtual LMResult GetValueForgotState(const std::vector<const Word*> &contextFactor, FFState &outState) const;

protected:
  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = NULL) const = 0;
public:
  std::string DebugContextFactor(const std::vector<const Word*> &contextFactor) const;
};



}

#endif
