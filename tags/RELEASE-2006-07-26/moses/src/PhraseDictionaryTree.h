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
class Word;
class ConfusionNet;

// a FactorTgtCand is the Factor-phrase and the vector of scores
typedef std::pair<std::vector<const Factor*>,std::vector<float> > FactorTgtCand;
typedef std::pair<std::vector<std::string const*>,std::vector<float> > StringTgtCand;


class PDTimp;
class PPimp;

class PhraseDictionaryTree : public Dictionary {
	PDTimp *imp; //implementation
	FactorType m_inFactorType,m_outFactorType;
public:
	PhraseDictionaryTree(size_t noScoreComponent,
											 FactorCollection* factorCollection=0,
											 FactorType inputFactorType=Surface,
											 FactorType outputFactorType=Surface);

	virtual ~PhraseDictionaryTree();

	DecodeType GetDecodeType() const {return Translate;}
	size_t GetSize() const {return 0;}
	FactorType GetInputFactorType() const {return m_inFactorType;}
	FactorType GetOutputFactorType() const {return m_outFactorType;}
	
	void SetInputFactorType(FactorType t) {m_inFactorType=t;}
	void SetOutputFactorType(FactorType t) {m_outFactorType=t;}
	void SetFactorCollection(FactorCollection*);

	// convert from ascii phrase table format 
	// note: only creates table, does not keep it in memory
	//        -> use Read(outFileNamePrefix);
	int Create(std::istream& in,const std::string& outFileNamePrefix);

	int Read(const std::string& fileNamePrefix); 

	// free memory used by the prefix tree etc.
	void FreeMemory() const;


	/**************************************
	 *   access with full source phrase   *
	 **************************************/
	// get the target candidates for a given factor sequence/phrase
	void GetTargetCandidates(const std::vector<const Factor*>& src,
													 std::vector<FactorTgtCand>& rv) const;

	// print target candidates for a given phrase, mainly for debugging
	void PrintTargetCandidates(const std::vector<std::string>& src,
														 std::ostream& out) const;

	void GetTargetCandidates(const std::vector<std::string>& src,
													 std::vector<StringTgtCand>& rv) const;

	/*****************************
	 *   access to prefix tree   *
	 *****************************/

	// 'pointer' into prefix tree
	// the only permitted direct operation is a check for NULL,
	// e.g. PrefixPtr p; if(p) ...
	// other usage only through PhraseDictionaryTree-functions below

	class PrefixPtr {
		PPimp* imp;
		friend class PDTimp;
	public:
		PrefixPtr(PPimp* x=0) : imp(x) {}
		operator bool() const;
	};

	// return pointer to root node
	PrefixPtr GetRoot() const;
	// extend pointer with a word/Factorstring and return the resulting successor
	// pointer. If there is no such successor node, the result will evaluate to 
	// false. Requirement: the input pointer p evaluates to true.
	PrefixPtr Extend(PrefixPtr p,const std::string& s) const;

	// get the target candidates for a given prefix pointer
	// requirement: the pointer has to evaluate to true
	void GetTargetCandidates(PrefixPtr p,
													 std::vector<FactorTgtCand>& rv) const;
	void GetTargetCandidates(PrefixPtr p,
													 std::vector<StringTgtCand>& rv) const;

	// print target candidates for a given prefix pointer to a stream, mainly 
	// for debugging
	void PrintTargetCandidates(PrefixPtr p,std::ostream& out) const;
};


void GenerateCandidates(const ConfusionNet& src,
												const std::vector<PhraseDictionaryTree const*>& pdicts,
												const std::vector<std::vector<float> >& weights,
												int verbose=0) ;


#endif /*PHRASEDICTIONARYTREE_H_*/
