// $Id$
#ifndef PHRASEDICTIONARYTREE_H_
#define PHRASEDICTIONARYTREE_H_
#include <string>
#include <vector>
#include <iostream>
#include "TypeDef.h"
#include "Dictionary.h"

class Phrase;
class FactorCollection;

// a FactorTgtCand is the Factor-phrase and the vector of scores
typedef std::pair<std::vector<const Factor*>,std::vector<float> > FactorTgtCand;

class PDTimp;
class PPimp;

class PhraseDictionaryTree : public Dictionary {
	PDTimp *imp; //implementation
public:

	class PrefixPtr {
		PPimp* imp;
		friend class PDTimp;
	public:
		PrefixPtr();
		operator bool() const;
	};

	PhraseDictionaryTree(size_t noScoreComponent,
											 FactorCollection* factorCollection=0,
											 FactorType factorType=Surface);

	virtual ~PhraseDictionaryTree();

	DecodeType GetDecodeType() const
	{
		return Translate;
	}
	size_t GetSize() const
	{
		return 0;
	}

	// convert from ascii phrase table format 
	int Create(std::istream& In,const std::string& OutFileNamePrefix);
	int Read(const std::string& FileNamePrefix); 

	// free memory used by the prefix tree
	void FreeMemory();

	// access with full src phrase
	void GetTargetCandidates(const std::vector<const Factor*>& src,
													 std::vector<FactorTgtCand>& rv) const;
	void PrintTargetCandidates(const std::vector<std::string>& src,
														 std::ostream& out) const;

	// access to prefix tree
	PrefixPtr GetRoot() const;
	PrefixPtr Extend(PrefixPtr,const std::string&) const;

	void GetTargetCandidates(PrefixPtr p,
													 std::vector<FactorTgtCand>& rv) const;
	void PrintTargetCandidates(PrefixPtr p,std::ostream& out) const;

};
#endif /*PHRASEDICTIONARYTREE_H_*/
