// $Id$
#include "CreateTargetPhraseCollection.h"

#include "PhraseDictionary.h"
#include "Input.h"
#include "Sentence.h"
#include "WordsRange.h"
#include "PhraseDictionaryTreeAdaptor.h"
TargetPhraseCollection const* 
CreateTargetPhraseCollection(Dictionary const* d,
														 InputType const* i,
														 const WordsRange& r) 
{
	if(Sentence const * s=dynamic_cast<Sentence const*>(i)) {
		Phrase src=s->GetSubString(r);
		if(PhraseDictionary const* pdict=dynamic_cast<PhraseDictionary const*>(d))
			return pdict->FindEquivPhrase(src);
		else if(PhraseDictionaryTreeAdaptor const* pdict=dynamic_cast<PhraseDictionaryTreeAdaptor const*>(d))
			return pdict->GetTargetPhraseCollection(src);
		else 
			std::cerr<<"ERROR: unknown phrase dictionary type in "<<__FILE__<<"\n";
	}

	// todo: implementation for PhraseDictionaryTree and Confusion Nets
	
	return 0;
}
