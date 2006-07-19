// $Id$
#include "CreateTargetPhraseCollection.h"

#include "PhraseDictionary.h"
#include "Input.h"
#include "Sentence.h"
#include "WordsRange.h"
TargetPhraseCollection const* 
CreateTargetPhraseCollection(Dictionary const* d,
														 InputType const* i,
														 const WordsRange& r) 
{
	if(Sentence const * s=dynamic_cast<Sentence const*>(i)) {
		Phrase src=s->GetSubString(r);
		if(dynamic_cast<PhraseDictionary const*>(d))
			return dynamic_cast<PhraseDictionary const*>(d)->FindEquivPhrase(src);
	}

	// todo: implementation for PhraseDictionaryTree and Confusion Nets
	
	return 0;
}
