// $Id$

#ifndef INPUT_H_
#define INPUT_H_
#include <string>
#include "TypeDef.h"
#include "Phrase.h"
#include "TargetPhraseCollection.h"

class WordsRange;
class Factor;
class PhraseDictionaryBase;
class TranslationOptionCollection;

// base class for sentences and confusion networks
class InputType 
{
protected:
	long m_translationId;
 public:

	InputType(long translationId=0);
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

	virtual int Read(std::istream& in,const std::vector<FactorType>& factorOrder, FactorCollection &factorCollection) =0;

	virtual void Print(std::ostream&) const=0;

	virtual TargetPhraseCollection const* CreateTargetPhraseCollection(PhraseDictionaryBase const& d,const WordsRange& r) const=0;
	virtual TranslationOptionCollection* CreateTranslationOptionCollection() const=0;

	virtual Phrase GetSubString(const WordsRange&) const =0;
	virtual const FactorArray& GetFactorArray(size_t pos) const=0;
};

std::ostream& operator<<(std::ostream&,InputType const&);
#endif
