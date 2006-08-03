// $Id$

#include "PhraseDictionaryTreeAdaptor.h"
#include <sys/stat.h>
#include "PhraseDictionaryTree.h"
#include "Phrase.h"
#include "FactorCollection.h"
#include "InputFileStream.h"
#include "Input.h"
#include "ConfusionNet.h"
#include "StaticData.h"
#include "UniqueObject.h"

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
	mutable MapSrc2Tgt m_cache;
	PhraseDictionaryTreeAdaptor *m_obj;
	int useCache;

	typedef std::vector<TargetPhraseCollection const*> vTPC;
	std::vector<vTPC> m_rangeCache;
	unsigned m_numInputScores;

	PDTAimp(PhraseDictionaryTreeAdaptor *p,unsigned nis) 
		: m_languageModels(0),m_weightWP(0.0),m_factorCollection(0),m_dict(0),
			m_obj(p),useCache(1),m_numInputScores(nis) {}

	void Factors2String(FactorArray const& w,std::string& s) const 
	{
		for(size_t j=0;j<m_input.size();++j)
			{
				assert(static_cast<size_t>(m_input[j]) < static_cast<size_t>(NUM_FACTORS));
				assert(w[m_input[j]]);
				if(s.size()) s+="|";
				s+=w[m_input[j]]->ToString();
			}
	}

	void CleanUp() 
	{
		assert(m_dict);
		m_dict->FreeMemory();
		for(size_t i=0;i<m_tgtColls.size();++i) delete m_tgtColls[i];
		m_tgtColls.clear();
		m_cache.clear();
		m_rangeCache.clear();
		Phrase dummy(Input); uniqueObject(dummy,1);
	}

	void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase) 
	{
		assert(GetTargetPhraseCollection(source)==0);
		TRACE_ERR("adding unk source phrase "<<source<<"\n");
		std::pair<MapSrc2Tgt::iterator,bool> p
			=m_cache.insert(std::make_pair(source,static_cast<TargetPhraseCollection const*>(0)));
		if(p.second || p.first->second==0) 
			{
				TargetPhraseCollection *ptr=new TargetPhraseCollection;
				ptr->push_back(targetPhrase);
				p.first->second=ptr;
				m_tgtColls.push_back(ptr);
			}
		else std::cerr<<"WARNING: you added an already existing phrase!\n";
	}

	TargetPhraseCollection const* 
	GetTargetPhraseCollection(Phrase const &src) const
	{
		assert(m_dict);
		if(src.GetSize()==0) return 0;

		std::pair<MapSrc2Tgt::iterator,bool> piter;
		if(useCache) 
			{
				piter=m_cache.insert(std::make_pair(src,static_cast<TargetPhraseCollection const*>(0)));
				if(!piter.second) return piter.first->second;
			}
		else if (m_cache.size()) 
			{
				MapSrc2Tgt::const_iterator i=m_cache.find(src);
				return (i!=m_cache.end() ? i->second : 0);
			}

		std::vector<std::string> srcString(src.GetSize());
		// convert source Phrase into vector of strings
		for(size_t i=0;i<srcString.size();++i)
			Factors2String(src.GetFactorArray(i),srcString[i]);

		// get target phrases in string representation
		std::vector<StringTgtCand> cands;
		m_dict->GetTargetCandidates(srcString,cands);
		if(cands.empty()) return 0;
			
		std::vector<TargetPhrase> tCands;tCands.reserve(cands.size());
		std::vector<std::pair<float,size_t> > costs;costs.reserve(cands.size());

		// convert into TargetPhrases
		for(size_t i=0;i<cands.size();++i) 
			{
				TargetPhrase targetPhrase(Output);

				StringTgtCand::first_type const& factorStrings=cands[i].first;
				StringTgtCand::second_type const& probVector=cands[i].second;

				std::vector<float> scoreVector(probVector.size());
				std::transform(probVector.begin(),probVector.end(),scoreVector.begin(),TransformScore);
				CreateTargetPhrase(targetPhrase,factorStrings,scoreVector);
				costs.push_back(std::make_pair(targetPhrase.GetFutureScore(),tCands.size()));
				tCands.push_back(targetPhrase);
			}

		TargetPhraseCollection *rv=PruneTargetCandidates(tCands,costs);

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
		m_dict=new PhraseDictionaryTree(weight.size()-m_numInputScores);
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

	typedef PhraseDictionaryTree::PrefixPtr PPtr;
	typedef std::pair<size_t,size_t> Range;
	struct State {
		PPtr ptr;
		Range range;
		float score;
		unsigned realWords;
		Phrase src;

		State() : range(0,0),score(0.0),realWords(0),src(Input) {}
		State(size_t b,size_t e,const PPtr& v,float sc=0.0,unsigned rw=0) : ptr(v),range(b,e),score(sc),realWords(rw),src(Input) {}
		State(Range const& r,const PPtr& v,float sc=0.0,unsigned rw=0) : ptr(v),range(r),score(sc),realWords(rw),src(Input) {}

		size_t begin() const {return range.first;}
		size_t end() const {return range.second;}
		float GetScore() const {return score;}

	};

	void CreateTargetPhrase(TargetPhrase& targetPhrase,
													StringTgtCand::first_type const& factorStrings,
													StringTgtCand::second_type const& scoreVector,
													Phrase const* srcPtr=0) const
	{

		for(size_t k=0;k<factorStrings.size();++k) 
			{
				std::vector<std::string> factors=Tokenize(*factorStrings[k],"|");
				FactorArray& fa=targetPhrase.AddWord();
				for(size_t l=0;l<m_output.size();++l)
					fa[m_output[l]]=m_factorCollection->AddFactor(Output, m_output[l], factors[l]);
			}
		targetPhrase.SetScore(m_obj, scoreVector, m_weights, *m_languageModels, m_weightWP);
		targetPhrase.SetSourcePhrase(srcPtr);
	}


	TargetPhraseCollection* PruneTargetCandidates(std::vector<TargetPhrase> const & tCands,
																								std::vector<std::pair<float,size_t> >& costs) const 
	{
		// prune target candidates and sort according to score
		std::vector<std::pair<float,size_t> >::iterator nth=costs.end();
		if(m_obj->m_maxTargetPhrase>0 && costs.size()>m_obj->m_maxTargetPhrase) {
			nth=costs.begin()+m_obj->m_maxTargetPhrase;
			std::nth_element(costs.begin(),nth,costs.end(),std::greater<std::pair<float,size_t> >());
		}
		std::sort(costs.begin(),nth,std::greater<std::pair<float,size_t> >());

		// convert into TargetPhraseCollection
		TargetPhraseCollection *rv=new TargetPhraseCollection;
		for(std::vector<std::pair<float,size_t> >::iterator it=costs.begin();it!=nth;++it) 
			rv->push_back(tCands[it->second]);
		return rv;
	}

	// POD for target phrase scores
	struct TScores {
		float total;
		StringTgtCand::second_type trans;
		Phrase const* src;

		TScores() : total(0.0),src(0) {}
	};

	void CacheSource(ConfusionNet const& src) 
	{
		assert(m_dict);
		std::vector<State> stack;
		for(size_t i=0;i<src.GetSize();++i) stack.push_back(State(i,i,m_dict->GetRoot()));

		typedef StringTgtCand::first_type sPhrase;
		typedef std::map<StringTgtCand::first_type,TScores> E2Costs;

		std::map<Range,E2Costs> cov2cand;

		while(!stack.empty()) 
			{
				State curr(stack.back());
				stack.pop_back();
		
				//std::cerr<<"processing state "<<curr<<" stack size: "<<stack.size()<<"\n";

				assert(curr.end()<src.GetSize());
				const ConfusionNet::Column &currCol=src[curr.end()];
				for(size_t colidx=0;colidx<currCol.size();++colidx) 
					{
						const Word& w=currCol[colidx].first;
						std::string s;
						Factors2String(w.GetFactorArray(),s);
						bool isEpsilon=(s=="" || s==EPSILON);
						PPtr nextP = (isEpsilon ? curr.ptr : m_dict->Extend(curr.ptr,s));
						unsigned newRealWords=curr.realWords + (isEpsilon ? 0 : 1);
						if(nextP) 
							{
								Range newRange(curr.begin(),curr.end()+1);
								float newScore=curr.GetScore()+currCol[colidx].second;
								Phrase newSrc(curr.src);
								if(!isEpsilon) newSrc.push_back(w);
								if(newRange.second<src.GetSize())
									{
										stack.push_back(State(newRange,nextP,newScore,newRealWords));
										stack.back().src=newSrc;
									}

								std::vector<StringTgtCand> tcands;
								m_dict->GetTargetCandidates(nextP,tcands);

								if(tcands.size()) 
									{
										E2Costs& e2costs=cov2cand[newRange];
										Phrase const* srcPtr=uniqueObject(newSrc);
										for(size_t i=0;i<tcands.size();++i)
											{
												std::vector<float> nscores(tcands[i].second.size()+m_numInputScores,0.0);
												std::transform(tcands[i].second.begin(),tcands[i].second.end(),nscores.begin(),TransformScore);
												switch(m_numInputScores)
													{
													case 2: nscores[nscores.size()-1-m_numInputScores+2]=-1.0*newRealWords; // do not use -newRealWords ! -- RZ
													case 1: nscores[nscores.size()-1-m_numInputScores+1]=newScore;
													case 0: break;
													default:
														std::cerr<<"ERROR: too many model scaling factors for input weights 'weight-i' : "<<m_numInputScores<<"\n";
														abort();
													}
												assert(nscores.size()==m_weights.size());
												float score=std::inner_product(nscores.begin(),nscores.end(),m_weights.begin(),0.0);

												score-=tcands[i].first.size() * m_weightWP;
												std::pair<E2Costs::iterator,bool> p=e2costs.insert(std::make_pair(tcands[i].first,TScores()));

												TScores & scores=p.first->second;
												if(p.second || scores.total<score)
													{
														scores.total=score;
														scores.trans=nscores;
														scores.src=srcPtr;
													}
											}
									}
							}
					}
			} // end while(!stack.empty()) 

		m_rangeCache.resize(src.GetSize(),vTPC(src.GetSize(),0));

		for(std::map<Range,E2Costs>::const_iterator i=cov2cand.begin();i!=cov2cand.end();++i)
			{
				assert(i->first.first<m_rangeCache.size());
				assert(i->first.second>0);
				assert(i->first.second-1<m_rangeCache[i->first.first].size());
				assert(m_rangeCache[i->first.first][i->first.second-1]==0);

				std::vector<TargetPhrase> tCands;tCands.reserve(i->second.size());
				std::vector<std::pair<float,size_t> > costs;costs.reserve(i->second.size());

				for(E2Costs::const_iterator j=i->second.begin();j!=i->second.end();++j)
					{
						TScores const & scores=j->second;
						TargetPhrase targetPhrase(Output);
						CreateTargetPhrase(targetPhrase,j->first,scores.trans,scores.src);
						costs.push_back(std::make_pair(targetPhrase.GetFutureScore(),tCands.size()));
						tCands.push_back(targetPhrase);
					}

				TargetPhraseCollection *rv=PruneTargetCandidates(tCands,costs);

				if(rv->empty()) 
					delete rv;
				else
					{
						m_rangeCache[i->first.first][i->first.second-1]=rv;
						m_tgtColls.push_back(rv);
					}
			}
	}
};




/*************************************************************
	function definitions of the interface class
	virtually everything is forwarded to the implementation class
*************************************************************/

PhraseDictionaryTreeAdaptor::
PhraseDictionaryTreeAdaptor(size_t noScoreComponent,unsigned numInputScores)
	: MyBase(noScoreComponent),imp(new PDTAimp(this,numInputScores)) {}

PhraseDictionaryTreeAdaptor::~PhraseDictionaryTreeAdaptor() 
{
	imp->CleanUp();
}

void PhraseDictionaryTreeAdaptor::CleanUp() 
{
	imp->CleanUp();
	MyBase::CleanUp();
}

void PhraseDictionaryTreeAdaptor::InitializeForInput(InputType const& source)
{
	// only required for confusion net
	if(ConfusionNet const* cn=dynamic_cast<ConfusionNet const*>(&source))
		imp->CacheSource(*cn);
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
	m_filename = filePath;

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
TargetPhraseCollection const* 
PhraseDictionaryTreeAdaptor::GetTargetPhraseCollection(InputType const& src,WordsRange const &range) const
{
	if(imp->m_rangeCache.empty())
		return imp->GetTargetPhraseCollection(src.GetSubString(range));
	else
		return imp->m_rangeCache[range.GetStartPos()][range.GetEndPos()];
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
void PhraseDictionaryTreeAdaptor::EnableCache()
{
	imp->useCache=1;
}
void PhraseDictionaryTreeAdaptor::DisableCache()
{
	imp->useCache=0;
}
