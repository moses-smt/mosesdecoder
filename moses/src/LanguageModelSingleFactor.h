// $Id: LanguageModelSingleFactor.h 3719 2010-11-17 14:06:21Z chardmeier $

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

#include "LanguageModelImplementation.h"
#include "Phrase.h"

namespace Moses
{

class FactorCollection;
class Factor;

//! Abstract class for for single factor LM 
class LanguageModelSingleFactor : public LanguageModelImplementation
{
protected:	
	const Factor *m_sentenceStart, *m_sentenceEnd;
	FactorType	m_factorType;

public:
	virtual ~LanguageModelSingleFactor();
	virtual bool Load(const std::string &filePath
					, FactorType factorType
					, size_t nGramOrder) = 0;

	LMType GetLMType() const
	{
		return SingleFactor;
	}

	bool Useable(const Phrase &phrase) const
	{
		return (phrase.GetSize()>0 && phrase.GetFactor(0, m_factorType) != NULL);		
	}
	
	const Factor *GetSentenceStart() const
	{
		return m_sentenceStart;
	}
	const Factor *GetSentenceEnd() const
	{
		return m_sentenceEnd;
	}
	FactorType GetFactorType() const
	{
		return m_factorType;
	}
	std::string GetScoreProducerDescription() const;
};

// Single factor LM that uses a null pointer state.  
class LanguageModelPointerState : public LanguageModelSingleFactor
{
private:
  FFState *m_nullContextState;
  FFState *m_beginSentenceState;
protected:
  typedef const void *State;

  LanguageModelPointerState();

  virtual ~LanguageModelPointerState();

  virtual FFState *GetNullContextState() const;
  virtual FFState *GetBeginSentenceState() const;
  virtual FFState *NewState(const FFState *from = NULL) const;

  virtual float GetValueForgotState(const std::vector<const Word*> &contextFactor, FFState &outState, unsigned int* len = 0) const;

  virtual float GetValue(const std::vector<const Word*> &contextFactor, State* finalState = NULL, unsigned int* len=0) const = 0;
};



}

#endif
