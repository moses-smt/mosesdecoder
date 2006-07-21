// $Id$
#ifndef INPUT_H_
#define INPUT_H_
#include <string>
#include "TypeDef.h"
#include "Phrase.h"
class WordsRange;
class Factor;

// base class for sentences and confusion networks
class InputType 
{
protected:
	long m_translationId;
 public:

	InputType(long translationId=0);
	//Input();
	virtual ~InputType();

	// for db stuff
	long GetTranslationId()
	{
		return m_translationId;
	}
	void SetTranslationId(long translationId)
	{	// for db stuff;
		m_translationId = translationId;
	}
	virtual size_t GetSize() const=0;


	// these functions are not (yet) well-defined for confusion networks
	virtual Phrase GetSubString(const WordsRange&) const =0;
	virtual const FactorArray& GetFactorArray(size_t pos) const=0;
};
#endif
