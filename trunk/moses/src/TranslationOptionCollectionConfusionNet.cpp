// $Id$
#include "TranslationOptionCollectionConfusionNet.h"
#include "ConfusionNet.h"
#include "DecodeStep.h"
#include "LanguageModel.h"
#include "PhraseDictionary.h"
#include "FactorCollection.h"
#include "LMList.h"

TranslationOptionCollectionConfusionNet::
TranslationOptionCollectionConfusionNet(const ConfusionNet &input, size_t maxNoTransOptPerCoverage) 
	: TranslationOptionCollection(input, maxNoTransOptPerCoverage) {}

void TranslationOptionCollectionConfusionNet::
ProcessUnknownWord(		size_t sourcePos
											, FactorCollection &factorCollection) 
{
	ConfusionNet const& source=dynamic_cast<ConfusionNet const&>(m_source);

	ConfusionNet::Column const& coll=source.GetColumn(sourcePos);
	for(ConfusionNet::Column::const_iterator i=coll.begin();i!=coll.end();++i)
		ProcessOneUnknownWord(i->first,sourcePos,factorCollection);
		
}

