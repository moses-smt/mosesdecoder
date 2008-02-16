// $Id: PDTAimp.h 119 2007-09-25 16:44:43Z hieu $
// vim:tabstop=2

#pragma once

#include <vector>
#include <map>
#include "TypeDef.h"
#include "Phrase.h"
#include "UniqueObject.h"
#include "PhraseDictionaryTree.h"

class LMList;
class PhraseDictionaryTree;
class TargetPhraseCollection;
class TargetPhrase;
class PhraseDictionaryTreeAdaptor;

class PDTAimp 
{
	// only these classes are allowed to instantiate this class
	friend class PhraseDictionaryTreeAdaptor;
	
protected:
	PDTAimp(PhraseDictionaryTreeAdaptor *p,unsigned nis) 
		: m_languageModels(0),m_weightWP(0.0),m_dict(0),
			m_obj(p),useCache(1),m_numInputScores(nis),totalE(0),distinctE(0) {}
	
public:
	std::vector<float> m_weights;
	LMList const* m_languageModels;
	float m_weightWP;
	std::vector<FactorType> m_input,m_output;
	PhraseDictionaryTree *m_dict;
	typedef std::vector<TargetPhraseCollection const*> vTPC;
	mutable vTPC m_tgtColls;

	typedef std::map<Phrase,TargetPhraseCollection const*> MapSrc2Tgt;
	mutable MapSrc2Tgt m_cache;
	PhraseDictionaryTreeAdaptor *m_obj;
	int useCache;

	std::vector<vTPC> m_rangeCache;
	unsigned m_numInputScores;

	UniqueObjectManager<Phrase> uniqSrcPhr;

	size_t totalE,distinctE;
	std::vector<size_t> path1Best,pathExplored;
	std::vector<double> pathCN;

	~PDTAimp() ;

	void Factors2String(Word const& w,std::string& s) const;
	void CleanUp();
	void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase);

	TargetPhraseCollection const*GetTargetPhraseCollection(Phrase const &src) const;
	void Create(const std::vector<FactorType> &input
							, const std::vector<FactorType> &output
							, const std::string &filePath
							, const std::vector<float> &weight
							, const LMList &languageModels
							, float weightWP
							);
	void CreateTargetPhrase(TargetPhrase& targetPhrase,
													StringTgtCand::first_type const& factorStrings,
													StringTgtCand::second_type const& scoreVector,
													Phrase const* srcPtr=0) const;

	TargetPhraseCollection* PruneTargetCandidates(std::vector<TargetPhrase> const & tCands,
																								std::vector<std::pair<float,size_t> >& costs) const;
	void CacheSource(ConfusionNet const& src);
	size_t GetNumInputScores() const {return m_numInputScores;}
};

