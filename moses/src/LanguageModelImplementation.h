// $Id: LanguageModelImplementation.h 3720 2010-11-18 10:27:30Z bhaddow $

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

#ifndef moses_LanguageModelImplementation_h
#define moses_LanguageModelImplementation_h

#include <string>
#include <vector>
#include "Factor.h"
#include "TypeDef.h"
#include "Util.h"
#include "FeatureFunction.h"
#include "Word.h"

namespace Moses
{

class FactorCollection;
class Factor;
class Phrase;

//! Abstract base class which represent a language model on a contiguous phrase
class LanguageModelImplementation
{
#ifndef WITH_THREADS
protected:
	/** constructor to be called by inherited class
	 */
	LanguageModelImplementation() : m_referenceCount(0) {}

private:
	// ref counting handled by boost if we have threads
	unsigned int m_referenceCount;
#else
	// default constructor is ok
#endif

protected:	
	std::string	m_filePath; //! for debugging purposes
	size_t			m_nGramOrder; //! max n-gram length contained in this LM
	Word m_sentenceStartArray, m_sentenceEndArray; //! Contains factors which represents the beging and end words for this LM. 
																								//! Usually <s> and </s>

public:
	virtual ~LanguageModelImplementation() {}

	//! Single or multi-factor
	virtual LMType GetLMType() const = 0;

	/* whether this LM can be used on a particular phrase. 
	 * Should return false if phrase size = 0 or factor types required don't exists
	 */
	virtual bool Useable(const Phrase &phrase) const = 0;

	/* get score of n-gram. n-gram should not be bigger than m_nGramOrder
	 * Specific implementation can return State and len data to be used in hypothesis pruning
	 * \param contextFactor n-gram to be scored
	 * \param state LM state.  Input and output.  state must be initialized.  If state isn't initialized, you want GetValueWithoutState.
	 * \param len If non-null, the n-gram length is written here.  
	 */
	virtual float GetValueGivenState(const std::vector<const Word*> &contextFactor, FFState &state, unsigned int* len = 0) const;

  // Like GetValueGivenState but state may not be initialized (however it is non-NULL). 
  // For example, state just came from NewState(NULL).   
	virtual float GetValueForgotState(
      const std::vector<const Word*> &contextFactor,
      FFState &outState,
      unsigned int* len = 0) const = 0;

	//! get State for a particular n-gram.  We don't care what the score is.  
  // This is here so models can implement a shortcut to GetValueAndState.  
  virtual void GetState(
      const std::vector<const Word*> &contextFactor,
      FFState &outState) const;

	virtual FFState *GetNullContextState() const = 0;
	virtual FFState *GetBeginSentenceState() const = 0;
  virtual FFState *NewState(const FFState *from = NULL) const = 0;

	//! max n-gram order of LM
	size_t GetNGramOrder() const
	{
		return m_nGramOrder;
	}
	
	//! Contains factors which represents the beging and end words for this LM. Usually <s> and </s>
	const Word &GetSentenceStartArray() const
	{
		return m_sentenceStartArray;
	}
	const Word &GetSentenceEndArray() const
	{
		return m_sentenceEndArray;
	}
	
	virtual std::string GetScoreProducerDescription() const = 0;

	float GetWeight() const;

	std::string GetScoreProducerWeightShortName() const 
	{ 
		return "lm";
	}
  
	//! overrideable funtions for IRST LM to cleanup. Maybe something to do with on demand/cache loading/unloading
	virtual void InitializeBeforeSentenceProcessing(){};
	virtual void CleanUpAfterSentenceProcessing() {};

#ifndef WITH_THREADS
	// ref counting handled by boost otherwise

	unsigned int IncrementReferenceCount()
	{
		return ++m_referenceCount;
	}

	unsigned int DecrementReferenceCount()
	{
		return --m_referenceCount;
	}
#endif
};

}

#endif
