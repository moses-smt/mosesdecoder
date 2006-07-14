#ifndef PHRASEDICTIONARYTREE_H_
#define PHRASEDICTIONARYTREE_H_
#include <string>
#include <vector>
#include <iostream>
#include "TypeDef.h"
#include "Dictionary.h"
#include "PhraseDictionary.h"

class Phrase;
class PDTimp;
class FactorCollection;


typedef std::pair<std::vector<const Factor*>,std::vector<float> > FactorTgtCand;


class PhraseDictionaryTree : public Dictionary {
	PDTimp *imp; //implementation
public:
	PhraseDictionaryTree(size_t noScoreComponent,FactorCollection* factorCollection=0,FactorType factorType=Surface);
	virtual ~PhraseDictionaryTree();

	DecodeType GetDecodeType() const
	{
		return Translate;
	}

	int CreateBinaryFileFromAsciiPhraseTable(std::istream& In,const std::string& OutputFileNamePrefix);
	int ReadBinary(const std::string& FileNamePrefix); 


	size_t GetSize() const
	{
		return 0;
	}
	
//	const TargetPhraseCollection *FindEquivPhrase(const Phrase &source) const;

	void GetTargetCandidates(const std::vector<const Factor*>& src,std::vector<FactorTgtCand>& rv) const;
	void PrintTargetCandidates(const std::vector<std::string>& src,std::ostream& out) const;

	// for mert
	void SetWeightTransModel(const std::vector<float> &weightT);

};
#endif /*PHRASEDICTIONARYTREE_H_*/
