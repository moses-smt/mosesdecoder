// $Id$
// vim:tabstop=2

#pragma once

#include "FactorCollection.h"

inline bool existsFile(const char* filePath) {
  struct stat mystat;
  return  (stat(filePath,&mystat)==0);
}

double addLogScale(double x,double y) 
{
	if(x>y) return addLogScale(y,x); else return x+log(1.0+exp(y-x));
}

double Exp(double x)
{
	return exp(x);
}

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

	~PDTAimp() 
	{
		CleanUp();
		delete m_dict;

		if (StaticData::Instance()->GetVerboseLevel() >= 2)
			{

				TRACE_ERR("tgt candidates stats:  total="<<totalE<<";  distinct="
								 <<distinctE<<" ("<<distinctE/(0.01*totalE)<<");  duplicates="
								 <<totalE-distinctE<<" ("<<(totalE-distinctE)/(0.01*totalE)
								 <<")\n");

				TRACE_ERR("\npath statistics\n");

				if(path1Best.size()) 
					{
						TRACE_ERR("1-best:        ");
						std::copy(path1Best.begin()+1,path1Best.end(),
											std::ostream_iterator<size_t>(std::cerr," \t")); 
						TRACE_ERR("\n");
					}
				if(pathCN.size())
					{
						TRACE_ERR("CN (full):     ");
						std::transform(pathCN.begin()+1
													,pathCN.end()
													,std::ostream_iterator<double>(std::cerr," \t")
													,Exp); 
						TRACE_ERR("\n");
					}
				if(pathExplored.size())
					{
						TRACE_ERR("CN (explored): ");
						std::copy(pathExplored.begin()+1,pathExplored.end(),
											std::ostream_iterator<size_t>(std::cerr," \t")); 
						TRACE_ERR("\n");
					}
			}

	}

	void Factors2String(Word const& w,std::string& s) const 
	{
		s=w.GetString(m_input,false);
	}

	void CleanUp() 
	{
		assert(m_dict);
		m_dict->FreeMemory();
		for(size_t i=0;i<m_tgtColls.size();++i) delete m_tgtColls[i];
		m_tgtColls.clear();
		m_cache.clear();
		m_rangeCache.clear();
		uniqSrcPhr.clear();
	}

	void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase) 
	{
		assert(GetTargetPhraseCollection(source)==0);
		TRACE_ERR( "adding unk source phrase "<<source<<"\n");
		std::pair<MapSrc2Tgt::iterator,bool> p
			=m_cache.insert(std::make_pair(source,static_cast<TargetPhraseCollection const*>(0)));
		if(p.second || p.first->second==0) 
			{
				TargetPhraseCollection *ptr=new TargetPhraseCollection;
				ptr->Add(new TargetPhrase(targetPhrase));
				p.first->second=ptr;
				m_tgtColls.push_back(ptr);
			}
		else TRACE_ERR("WARNING: you added an already existing phrase!\n");
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
		{
			Factors2String(src.GetWord(i),srcString[i]);
		}

		// get target phrases in string representation
		std::vector<StringTgtCand> cands;
		m_dict->GetTargetCandidates(srcString,cands);
		if(cands.empty()) 
		{
			return 0;
		}
			
		std::vector<TargetPhrase> tCands;tCands.reserve(cands.size());
		std::vector<std::pair<float,size_t> > costs;costs.reserve(cands.size());

		// convert into TargetPhrases
		for(size_t i=0;i<cands.size();++i) 
			{
				TargetPhrase targetPhrase(Output);

				StringTgtCand::first_type const& factorStrings=cands[i].first;
				StringTgtCand::second_type const& probVector=cands[i].second;

				std::vector<float> scoreVector(probVector.size());
				std::transform(probVector.begin(),probVector.end(),scoreVector.begin(),
											 TransformScore);
				std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),FloorScore);
				CreateTargetPhrase(targetPhrase,factorStrings,scoreVector);
				costs.push_back(std::make_pair(targetPhrase.GetFutureScore(),
																			 tCands.size()));
				tCands.push_back(targetPhrase);
			}

		TargetPhraseCollection *rv=PruneTargetCandidates(tCands,costs);

		if(rv->IsEmpty()) 
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
							, const std::string &filePath
							, const std::vector<float> &weight
							, const LMList &languageModels
							, float weightWP
							)
	{

		// set my members	
		m_dict=new PhraseDictionaryTree(weight.size()-m_numInputScores);
		m_input=input;
		m_output=output;
		m_languageModels=&languageModels;
		m_weightWP=weightWP;
		m_weights=weight;



		std::string binFname=filePath+".binphr.idx";
		if(!existsFile(binFname.c_str())) {
			TRACE_ERR( "bin ttable does not exist -> create it\n");
			InputFileStream in(filePath);
			m_dict->Create(in,filePath);
		}
		TRACE_ERR( "reading bin ttable\n");
		m_dict->Read(filePath);
	}

	typedef PhraseDictionaryTree::PrefixPtr PPtr;
	typedef unsigned short Position;
	typedef std::pair<Position,Position> Range;
	struct State {
		PPtr ptr;
		Range range;
		float score;
		Position realWords;
		Phrase src;

		State() : range(0,0),score(0.0),realWords(0),src(Input) {}
		State(Position b,Position e,const PPtr& v,float sc=0.0,Position rw=0) 
			: ptr(v),range(b,e),score(sc),realWords(rw),src(Input) {}
		State(Range const& r,const PPtr& v,float sc=0.0,Position rw=0) 
			: ptr(v),range(r),score(sc),realWords(rw),src(Input) {}

		Position begin() const {return range.first;}
		Position end() const {return range.second;}
		float GetScore() const {return score;}

		friend std::ostream& operator<<(std::ostream& out,State const& s) {
			out<<" R=("<<s.begin()<<","<<s.end()<<"),SC=("<<s.GetScore()<<","<<s.realWords<<")";
			return out;
		}

	};



	void CreateTargetPhrase(TargetPhrase& targetPhrase,
													StringTgtCand::first_type const& factorStrings,
													StringTgtCand::second_type const& scoreVector,
													Phrase const* srcPtr=0) const
	{
		FactorCollection &factorCollection = FactorCollection::Instance();

		for(size_t k=0;k<factorStrings.size();++k) 
			{
				std::vector<std::string> factors=Tokenize(*factorStrings[k],"|");
				Word& w=targetPhrase.AddWord();
				for(size_t l=0;l<m_output.size();++l)
					w[m_output[l]]=factorCollection.AddFactor(Output, m_output[l], factors[l]);
			}
		targetPhrase.SetScore(m_obj, scoreVector, m_weights, m_weightWP, *m_languageModels);
		targetPhrase.SetSourcePhrase(srcPtr);
	}


	TargetPhraseCollection* PruneTargetCandidates(std::vector<TargetPhrase> const & tCands,
																								std::vector<std::pair<float,size_t> >& costs) const 
	{
		// convert into TargetPhraseCollection
		TargetPhraseCollection *rv=new TargetPhraseCollection;
		for(std::vector<std::pair<float,size_t> >::iterator it=costs.begin();it!=costs.end();++it)
			rv->Add(new TargetPhrase(tCands[it->second]));
		rv->Sort(m_obj->m_tableLimit);
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
		const size_t srcSize=src.GetSize();

		std::vector<size_t> exploredPaths(srcSize+1,0);
		std::vector<double> exPathsD(srcSize+1,-1.0);

		// collect some statistics
		std::vector<size_t> cnDepths(srcSize,0);
		for(size_t i=0;i<srcSize;++i) cnDepths[i]=src[i].size();

		for(size_t len=1;len<=srcSize;++len)
			for(size_t i=0;i<=srcSize-len;++i)
				{
					double pd=0.0; for(size_t k=i;k<i+len;++k)	pd+=log(1.0*cnDepths[k]);
					exPathsD[len]=(exPathsD[len]>=0.0 ? addLogScale(pd,exPathsD[len]) : pd);
				}

		// update global statistics
		if(pathCN.size()<=srcSize) pathCN.resize(srcSize+1,-1.0);
		for(size_t len=1;len<=srcSize;++len) 
			pathCN[len]=pathCN[len]>=0.0 ? addLogScale(pathCN[len],exPathsD[len]) : exPathsD[len];

		if(path1Best.size()<=srcSize) path1Best.resize(srcSize+1,0);
		for(size_t len=1;len<=srcSize;++len) path1Best[len]+=srcSize-len+1;

		
		if (StaticData::Instance()->GetVerboseLevel() >= 2 && exPathsD.size())
			{
				TRACE_ERR("path stats for current CN: \n");
				TRACE_ERR("CN (full):     ");
				std::transform(exPathsD.begin()+1
											,exPathsD.end()
											,std::ostream_iterator<double>(std::cerr," ")
											,Exp);
				TRACE_ERR("\n");
			}

		typedef StringTgtCand::first_type sPhrase;
		typedef std::map<StringTgtCand::first_type,TScores> E2Costs;

		std::map<Range,E2Costs> cov2cand;
		std::vector<State> stack;
		for(Position i=0 ; i < srcSize ; ++i) 
			stack.push_back(State(i, i, m_dict->GetRoot()));

		while(!stack.empty()) 
			{
				State curr(stack.back());
				stack.pop_back();
		
				//TRACE_ERR("processing state "<<curr<<" stack size: "<<stack.size()<<"\n");

				assert(curr.end()<srcSize);
				const ConfusionNet::Column &currCol=src[curr.end()];
				// in a given column, loop over all possibilities
				for(size_t colidx=0;colidx<currCol.size();++colidx)
					{
						const Word& w=currCol[colidx].first; // w=the i^th possibility in column colidx
						std::string s;
						Factors2String(w,s);
						bool isEpsilon=(s=="" || s==EPSILON);
						
						// do not start with epsilon (except at first position)
						if(isEpsilon && curr.begin()==curr.end() && curr.begin()>0) continue; 

						// At a given node in the prefix tree, look to see if w defines an edge to
						// another node (Extend).  Stay at the same node if w==EPSILON
						PPtr nextP = (isEpsilon ? curr.ptr : m_dict->Extend(curr.ptr,s));
						unsigned newRealWords=curr.realWords + (isEpsilon ? 0 : 1);

						if(nextP) // w is a word that should be considered
							{
								Range newRange(curr.begin(),curr.end()+1);
								float newScore=curr.GetScore()+currCol[colidx].second;  // CN score
								Phrase newSrc(curr.src);
								if(!isEpsilon) newSrc.AddWord(w);
								if(newRange.second<srcSize && newScore>LOWEST_SCORE)
									{
									  // if there is more room to grow, add a new state onto the queue
										// to be explored that represents [begin, curEnd+1)
										stack.push_back(State(newRange,nextP,newScore,newRealWords));
										stack.back().src=newSrc;
									}

								std::vector<StringTgtCand> tcands;
								// now, look up the target candidates (aprx. TargetPhraseCollection) for
								// the current path through the CN
								m_dict->GetTargetCandidates(nextP,tcands);

								if(newRange.second>=exploredPaths.size()+newRange.first) 
									exploredPaths.resize(newRange.second-newRange.first+1,0);
								++exploredPaths[newRange.second-newRange.first];
	
								totalE+=tcands.size();

								if(tcands.size()) 
									{
										E2Costs& e2costs=cov2cand[newRange];
										Phrase const* srcPtr=uniqSrcPhr(newSrc);
										for(size_t i=0;i<tcands.size();++i)
										{
											std::vector<float> nscores(tcands[i].second.size()+m_numInputScores,0.0);
											switch(m_numInputScores)
											{
												case 2: nscores[1]= -1.0f * newRealWords; // do not use -newRealWords ! -- RZ
												case 1: nscores[0]= newScore;
												case 0: break;
												default:
													TRACE_ERR("ERROR: too many model scaling factors for input weights 'weight-i' : "<<m_numInputScores<<"\n");
													abort();
											}
											std::transform(tcands[i].second.begin(),tcands[i].second.end(),nscores.begin() + m_numInputScores,TransformScore);
											
											assert(nscores.size()==m_weights.size());
											float score=std::inner_product(nscores.begin(), nscores.end(), m_weights.begin(), 0.0f);

											score-=tcands[i].first.size() * m_weightWP;
											std::pair<E2Costs::iterator,bool> p=e2costs.insert(std::make_pair(tcands[i].first,TScores()));
											
											if(p.second) ++distinctE;
											
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


		if (StaticData::Instance()->GetVerboseLevel() >= 2 && exploredPaths.size())
			{
				TRACE_ERR("CN (explored): ");
				std::copy(exploredPaths.begin()+1,exploredPaths.end(),
									std::ostream_iterator<size_t>(std::cerr," ")); 
				TRACE_ERR("\n");
			}

		if(pathExplored.size()<exploredPaths.size()) 
			pathExplored.resize(exploredPaths.size(),0);
		for(size_t len=1;len<=srcSize;++len)
			pathExplored[len]+=exploredPaths[len];


		m_rangeCache.resize(src.GetSize(),vTPC(src.GetSize(),0));

		for(std::map<Range,E2Costs>::const_iterator i=cov2cand.begin();i!=cov2cand.end();++i)
			{
				assert(i->first.first<m_rangeCache.size());
				assert(i->first.second>0);
				assert(static_cast<size_t>(i->first.second-1)<m_rangeCache[i->first.first].size());
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

				if(rv->IsEmpty()) 
					delete rv;
				else
					{
						m_rangeCache[i->first.first][i->first.second-1]=rv;
						m_tgtColls.push_back(rv);
					}
			}
		// free memory
		m_dict->FreeMemory();
	}
	
	
	size_t GetNumInputScores() const {return m_numInputScores;}
};

