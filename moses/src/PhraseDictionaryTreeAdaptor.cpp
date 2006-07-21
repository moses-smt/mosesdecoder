// $Id$
#include "PhraseDictionaryTreeAdaptor.h"
#include <sys/stat.h>
#include "PhraseDictionaryTree.h"
#include "Phrase.h"
#include "FactorCollection.h"
#include "InputFileStream.h"

inline bool existsFile(const char* filename) {
  struct stat mystat;
  return  (stat(filename,&mystat)==0);
}

struct PDTAimp {
	std::vector<float> m_weights;
	LMList const* m_languageModels;
	float m_weightWP;
	std::vector<FactorType> m_input,m_output;
	FactorCollection *m_factorCollection;
	PhraseDictionaryTree *m_dict;
	mutable std::vector<TargetPhraseCollection const*> m_tgtColls;

	typedef std::map<Phrase,TargetPhraseCollection const*> MapSrc2Tgt;
	MapSrc2Tgt m_unks;
	mutable MapSrc2Tgt m_cache;
	PhraseDictionaryTreeAdaptor *m_obj;
	int useCache;

	PDTAimp(PhraseDictionaryTreeAdaptor *p) 
		: m_languageModels(0),m_weightWP(0.0),m_factorCollection(0),m_dict(0),
			m_obj(p),useCache(1) {}

	void CleanUp() 
	{
		assert(m_dict);
		m_dict->FreeMemory();
		for(size_t i=0;i<m_tgtColls.size();++i) delete m_tgtColls[i];
		m_tgtColls.clear();
		m_unks.clear();
		m_cache.clear();
	}

	void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase) 
	{
		assert(GetTargetPhraseCollection(source)==0);
		TRACE_ERR("adding unk source phrase "<<source<<"\n");
		std::pair<MapSrc2Tgt::iterator,bool> p
			=m_unks.insert(std::make_pair(source,static_cast<TargetPhraseCollection const*>(0)));
		if(p.second) 
			{
				TargetPhraseCollection *ptr=new TargetPhraseCollection;
				ptr->push_back(targetPhrase);
				p.first->second=ptr;
				m_tgtColls.push_back(ptr);
			}
		else std::cerr<<"WARNING: you added the same unknown phrase twice!\n";
	}

	TargetPhraseCollection const * FindEquivPhrase(const Phrase &source) const 
	{
		assert(GetTargetPhraseCollection(source)==0);
		MapSrc2Tgt::const_iterator i=m_unks.find(source);
		if(i==m_unks.end()) 
			std::cerr<<"WARNING: nothing found for unk phrase "<<source<<"\n";
		return (i!=m_unks.end() ? i->second : 0);
	}

	TargetPhraseCollection const* 
	GetTargetPhraseCollection(Phrase const &src) const
	{
		assert(m_dict);
		if(src.GetSize()==0) return 0;
		std::pair<MapSrc2Tgt::iterator,bool> piter;
		if(useCache) {
			piter=m_cache.insert(std::make_pair(src,static_cast<TargetPhraseCollection const*>(0)));
			if(!piter.second) return piter.first->second;
		}

		std::vector<std::string> srcString(src.GetSize());
		// convert source Phrase into vector of strings
		for(size_t i=0;i<srcString.size();++i)
			for(size_t j=0;j<m_input.size();++j)
				{
					if(srcString[i].size()) srcString[i]+="|";
					srcString[i]+=src.GetFactor(i,m_input[j])->ToString();
				}

		// get target phrases in string representation
		std::vector<StringTgtCand> cands;
		m_dict->GetTargetCandidates(srcString,cands);
		if(cands.empty()) return 0;
			
		std::vector<TargetPhrase> tCands;tCands.reserve(cands.size());
		std::vector<std::pair<float,size_t> > costs;costs.reserve(cands.size());

		// convert into TargetPhrases
		for(size_t i=0;i<cands.size();++i) 
			{
				TargetPhrase targetPhrase(Output, m_obj);

				StringTgtCand::first_type const& factorStrings=cands[i].first;
				StringTgtCand::second_type const& scoreVector=cands[i].second;

				for(size_t k=0;k<factorStrings.size();++k) 
					{
						std::vector<std::string> factors=Tokenize(*factorStrings[k],"|");
						FactorArray& fa=targetPhrase.AddWord();
						for(size_t l=0;l<m_output.size();++l)
							fa[m_output[l]]=m_factorCollection->AddFactor(Output, m_output[l], factors[l]);
					}
			
				targetPhrase.SetScore(scoreVector, m_weights, *m_languageModels, m_weightWP);
	
				costs.push_back(std::make_pair(targetPhrase.GetFutureScore(),tCands.size()));
				tCands.push_back(targetPhrase);
			}

		// prune target candidates and sort according to score
		std::vector<std::pair<float,size_t> >::iterator nth=costs.end();
		if(m_obj->m_maxTargetPhrase>0 && costs.size()>m_obj->m_maxTargetPhrase) {
			nth=costs.begin()+m_obj->m_maxTargetPhrase;
			std::nth_element(costs.begin(),nth,costs.end(),std::greater<std::pair<float,size_t> >());
		}
		std::sort(costs.begin(),nth,std::greater<std::pair<float,size_t> >());

		// convert into TargerPhraseCollection
		TargetPhraseCollection *rv=new TargetPhraseCollection;
		for(std::vector<std::pair<float,size_t> >::iterator i=costs.begin();i!=nth;++i) 
			rv->push_back(tCands[i->second]);
	
		if(rv->empty()) 
			{
				delete rv;
				return 0;
			} 
		else 
			{
				if(useCache) piter.first->second=rv;
				m_tgtColls.push_back(rv);
				return rv;
			}
	}



	void Create(const std::vector<FactorType> &input
							, const std::vector<FactorType> &output
							, FactorCollection &factorCollection
							, const std::string &filePath
							, const std::vector<float> &weight
							, const LMList &languageModels
							, float weightWP
							)
	{

		// set my members	
		m_factorCollection=&factorCollection;
		m_dict=new PhraseDictionaryTree(weight.size());
		m_input=input;
		m_output=output;
		m_languageModels=&languageModels;
		m_weightWP=weightWP;
		m_weights=weight;

		std::string binFname=filePath+".binphr.idx";
		if(!existsFile(binFname.c_str())) {
			TRACE_ERR("bin ttable does not exist -> create it\n");
			InputFileStream in(filePath);
			m_dict->Create(in,filePath);
		}
		TRACE_ERR("reading bin ttable\n");
		m_dict->Read(filePath); 	
	}
};




/*************************************************************
	function definitions of the interface class
	virtually everything is forwarded to the implementation class
*************************************************************/

PhraseDictionaryTreeAdaptor::
PhraseDictionaryTreeAdaptor(size_t noScoreComponent)
	: MyBase(noScoreComponent),imp(new PDTAimp(this)) {}

PhraseDictionaryTreeAdaptor::~PhraseDictionaryTreeAdaptor() 
{
	imp->CleanUp();
}

void PhraseDictionaryTreeAdaptor::CleanUp() 
{
	imp->CleanUp();
	MyBase::CleanUp();
}

void PhraseDictionaryTreeAdaptor::Create(const std::vector<FactorType> &input
																				 , const std::vector<FactorType> &output
																				 , FactorCollection &factorCollection
																				 , const std::string &filePath
																				 , const std::vector<float> &weight
																				 , size_t maxTargetPhrase
																				 , const LMList &languageModels
																				 , float weightWP
																				 )
{
	if(m_noScoreComponent!=weight.size()) {
		std::cerr<<"ERROR: mismatch of number of scaling factors: "<<weight.size()
						 <<" "<<m_noScoreComponent<<"\n";
		abort();
	}

	// set Dictionary members
	m_factorsUsed[Input]	= new FactorTypeSet(input);
	m_factorsUsed[Output]	= new FactorTypeSet(output);

	// set PhraseDictionaryBase members
	m_maxTargetPhrase=maxTargetPhrase;

	imp->Create(input,output,factorCollection,filePath,
							weight,languageModels,weightWP);
}

TargetPhraseCollection const* 
PhraseDictionaryTreeAdaptor::GetTargetPhraseCollection(Phrase const &src) const
{
	return imp->GetTargetPhraseCollection(src);
}

void PhraseDictionaryTreeAdaptor::
SetWeightTransModel(const std::vector<float> &weightT)
{
	CleanUp();
	imp->m_weights=weightT;
}

void PhraseDictionaryTreeAdaptor::
AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase) 
{
	imp->AddEquivPhrase(source,targetPhrase);
}

TargetPhraseCollection const * 
PhraseDictionaryTreeAdaptor::FindEquivPhrase(const Phrase &source) const 
{
	return imp->FindEquivPhrase(source);
}

void PhraseDictionaryTreeAdaptor::EnableCache()
{
	imp->useCache=1;
}
void PhraseDictionaryTreeAdaptor::DisableCache()
{
	imp->useCache=0;
}
