
#include "PhraseList.h"
#include "InputFileStream.h"
#include "StaticData.h"

void PhraseList::Load(std::string filePath, FactorCollection &factorCollection)
{
	InputFileStream inFile(filePath);
	
	const std::vector<FactorType> &factorOrder = StaticData::Instance()->GetInputFactorOrder();
	const std::string& factorDelimiter = StaticData::Instance()->GetFactorDelimiter();

	string line;
	while(getline(inFile, line)) 
	{
		Phrase phrase(Input);
		phrase.CreateFromString( factorOrder
													, line
													, factorCollection
													, factorDelimiter
													, NULL, NULL);
		push_back(phrase);
	}
}


