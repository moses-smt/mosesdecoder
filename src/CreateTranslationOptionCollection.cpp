// $Id$
#include "CreateTranslationOptionCollection.h"
#include "Sentence.h"
#include "ConfusionNet.h"
#include "TranslationOptionCollectionText.h"
#include "TranslationOptionCollectionConfusionNet.h"

TranslationOptionCollection* CreateTranslationOptionCollection(InputType const* src) 
{
	if(Sentence const * s=dynamic_cast<Sentence const*>(src)) 
		return new TranslationOptionCollectionText(*s);
	else if(ConfusionNet const* cn=dynamic_cast<ConfusionNet const*>(src)) 
		return new TranslationOptionCollectionConfusionNet(*cn);
	else 
		{
			std::cerr<<"ERROR: unknown InputType in "<<__FILE__<<"\n";
			abort();
		}
}

