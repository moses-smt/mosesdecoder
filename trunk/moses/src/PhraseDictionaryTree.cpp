// $Id$
#include "PhraseDictionaryTree.h"
#include <map>
#include <cassert>
#include <sstream>
#include <iostream>
#include <fstream>
#include <ext/hash_map>

#include "PrefixTree.h"
#include "File.h"
#include "FactorCollection.h"
#include "ObjectPool.h"

template<typename T>
std::ostream& operator<<(std::ostream& out,const std::vector<T>& x)
{
	out<<x.size()<<" ";
	typename std::vector<T>::const_iterator iend=x.end();
	for(typename std::vector<T>::const_iterator i=x.begin();i!=iend;++i) 
		out<<*i<<' ';
	return out;
}

typedef unsigned LabelId;
LabelId InvalidLabelId=std::numeric_limits<LabelId>::max();
LabelId Epsilon=InvalidLabelId-1;

typedef std::vector<LabelId> IPhrase;
typedef std::vector<float> Scores;
typedef PrefixTreeF<LabelId,off_t> PTF;

namespace __gnu_cxx {
	template <> struct hash<std::string> {
		size_t operator()(const std::string& s) const {return __gnu_cxx::__stl_hash_string(s.c_str());}
	};
}

template<typename A,typename B=std::map<A,LabelId> >
class LVoc {
  typedef A Key;
  typedef B M;
  typedef std::vector<Key> V;
  M m;
  V data;
public:
  LVoc() {}

  bool isKnown(const Key& k) const {return m.find(k)!=m.end();}
  LabelId index(const Key& k) const {
    typename M::const_iterator i=m.find(k);
    return i!=m.end()? i->second : InvalidLabelId;}
  LabelId add(const Key& k) {
    std::pair<typename M::iterator,bool> p
			=m.insert(std::make_pair(k,data.size()));
    if(p.second) data.push_back(k);
		assert(static_cast<size_t>(p.first->second)<data.size());
    return p.first->second;
  }
  Key const& symbol(LabelId i) const {
    assert(static_cast<size_t>(i)<data.size());
    return data[i];}

  typedef typename V::const_iterator const_iterator;
  const_iterator begin() const {return data.begin();}
  const_iterator end() const {return data.end();}
  
  void Write(const std::string& fname) const {
  	std::ofstream out(fname.c_str()); Write(out);}
  void Write(std::ostream& out) const {
  	for(int i=data.size()-1;i>=0;--i)
  		out<<i<<' '<<data[i]<<'\n';
  }
  void Read(const std::string& fname) {
  	std::ifstream in(fname.c_str());Read(in);}
  void Read(std::istream& in) {
  	Key k;size_t i;std::string line;
  	while(getline(in,line)) {
  		std::istringstream is(line);
  		if(is>>i>>k) {
				if(i>=data.size()) data.resize(i+1);
  			data[i]=k;
  			m[k]=i;
  		}
  	}
  }	
};

class TgtCand {
	IPhrase e;
	Scores sc;
public:
	TgtCand() {}
	TgtCand(const IPhrase& a,const Scores& b) : e(a),sc(b) {}
	TgtCand(FILE* f) {readBin(f);}
	
	const IPhrase& GetPhrase() const {return e;}
	const Scores& GetScores() const {return sc;}
	
	void writeBin(FILE* f) const {fWriteVector(f,e);fWriteVector(f,sc);}
	void readBin(FILE* f) {fReadVector(f,e);fReadVector(f,sc);}	
};

class TgtCands : public std::vector<TgtCand> {
	typedef std::vector<TgtCand> MyBase;
public:
	TgtCands() : MyBase() {}

	void writeBin(FILE* f) const 
	{
		unsigned s=size();fWrite(f,s);
		for(size_t i=0;i<s;++i) MyBase::operator[](i).writeBin(f);
	}
	void readBin(FILE* f) 
	{
		unsigned s;fRead(f,s);resize(s);
		for(size_t i=0;i<s;++i) MyBase::operator[](i).readBin(f);
	}
};


struct PPimp {
	PTF const*p;size_t idx;bool root;
	
	PPimp(PTF const* x,size_t i,bool b) : p(x),idx(i),root(b) {}
	bool isValid() const {return root || (p && idx<p->size());}

	bool isRoot() const {return root;}
	PTF const* ptr() const {return p;}
};

PhraseDictionaryTree::PrefixPtr::operator bool() const 
{
	return imp && imp->isValid();
}


struct PDTimp {
  typedef PrefixTreeF<LabelId,off_t> PTF;
	typedef FilePtr<PTF> CPT;
  typedef std::vector<CPT> Data;
	//	typedef LVoc<std::string> WordVoc;
	typedef LVoc<std::string,__gnu_cxx::hash_map<std::string,LabelId> > WordVoc;

  Data data;
  std::vector<off_t> srcOffsets;

  FILE *os,*ot;
	WordVoc sv,tv;

	FactorCollection *m_factorCollection;
  ObjectPool<PPimp> pPool;

	PDTimp() : os(0),ot(0),m_factorCollection(0) {PTF::setDefault(InvalidOffT);}
	~PDTimp() {if(os) fClose(os);if(ot) fClose(ot);FreeMemory();}

	void FreeMemory() 
	{
		for(Data::iterator i=data.begin();i!=data.end();++i) (*i).free();
		pPool.reset();
	}

	int Read(const std::string& fn);
	
	void GetTargetCandidates(const IPhrase& f,TgtCands& tgtCands) 
	{
		if(f.empty()) return;
  	if(f[0]>=data.size()) return;
  	if(!data[f[0]]) return;
		assert(data[f[0]]->findKey(f[0])<data[f[0]]->size());
		off_t tCandOffset=data[f[0]]->find(f);
		if(tCandOffset==InvalidOffT) return;
  	fSeek(ot,tCandOffset);
   	tgtCands.readBin(ot);
	}

	typedef PhraseDictionaryTree::PrefixPtr PPtr;

	void GetTargetCandidates(PPtr p,TgtCands& tgtCands) 
	{
		assert(p);
		if(p.imp->isRoot()) return;
		off_t tCandOffset=p.imp->ptr()->getData(p.imp->idx);
		if(tCandOffset==InvalidOffT) return;
  	fSeek(ot,tCandOffset);
   	tgtCands.readBin(ot);
	}
	void PrintTgtCand(const TgtCands& tcands,std::ostream& out) const;

	// convert target candidates from internal data structure to the external one
	void ConvertTgtCand(const TgtCands& tcands,std::vector<FactorTgtCand>& rv,FactorType oft) const
	{
		for(TgtCands::const_iterator i=tcands.begin();i!=tcands.end();++i)
		{
			const IPhrase& iphrase=i->GetPhrase();
			std::vector<const Factor*> vf;
			vf.reserve(iphrase.size());
			for(size_t j=0;j<iphrase.size();++j)
				vf.push_back(m_factorCollection->AddFactor(Output,
																									 oft,tv.symbol(iphrase[j])));
			rv.push_back(FactorTgtCand(vf,i->GetScores()));
		}
	}
	// convert target candidates from internal data structure to the external one
	void ConvertTgtCand(const TgtCands& tcands,std::vector<StringTgtCand>& rv) const
	{
		for(TgtCands::const_iterator i=tcands.begin();i!=tcands.end();++i)
			{
				const IPhrase& iphrase=i->GetPhrase();
				std::vector<std::string const*> vs;
				vs.reserve(iphrase.size());
				for(size_t j=0;j<iphrase.size();++j)
					vs.push_back(&tv.symbol(iphrase[j]));
				rv.push_back(StringTgtCand(vs,i->GetScores()));
			}
	}

	PPtr GetRoot() 
	{
			return PPtr(pPool.get(PPimp(0,0,1)));
	}

	PPtr Extend(PPtr p,const std::string& w) 
	{
		assert(p);
		if(w.empty() || w==EPSILON) return p;
		LabelId wi=sv.index(w);
		if(wi==InvalidLabelId) return PPtr();
		else if(p.imp->isRoot()) 
			{
				if(wi<data.size() && data[wi])
					{
						assert(data[wi]->findKeyPtr(wi));
						return PPtr(pPool.get(PPimp(data[wi],data[wi]->findKey(wi),0)));
					}
			}
		else if(PTF const* nextP=p.imp->ptr()->getPtr(p.imp->idx)) 
			return PPtr(pPool.get(PPimp(nextP,nextP->findKey(wi),0)));
		
		return PPtr();
	}
};


////////////////////////////////////////////////////////////
//
// member functions of PDTimp
//
////////////////////////////////////////////////////////////

int PDTimp::Read(const std::string& fn) 
{
	std::string ifs(fn+".binphr.srctree"),
		ift(fn+".binphr.tgtdata"),
		ifi(fn+".binphr.idx"),
		ifsv(fn+".binphr.srcvoc"),
		iftv(fn+".binphr.tgtvoc");

	FILE *ii=fOpen(ifi.c_str(),"rb");
	fReadVector(ii,srcOffsets);
	fClose(ii);
	
	os=fOpen(ifs.c_str(),"rb");
	ot=fOpen(ift.c_str(),"rb");

	data.resize(srcOffsets.size());
	for(size_t i=0;i<data.size();++i)
		data[i]=CPT(os,srcOffsets[i]);
  
	sv.Read(ifsv);
	tv.Read(iftv);
  
	std::cerr<<"binary phrasefile loaded, default off_t: "<<PTF::getDefault()
					 <<"\n";
	return 1;
}

void PDTimp::PrintTgtCand(const TgtCands& tcand,std::ostream& out) const
{
	for(size_t i=0;i<tcand.size();++i) 
		{
		  out<<i<<" -- "<<tcand[i].GetScores()<<" -- ";
		  const IPhrase& iphr=tcand[i].GetPhrase();
		  for(size_t j=0;j<iphr.size();++j)
				out<<tv.symbol(iphr[j])<<" ";
		  out<<'\n';		
		}
}

////////////////////////////////////////////////////////////
//
// member functions of PhraseDictionaryTree
//
////////////////////////////////////////////////////////////

PhraseDictionaryTree::PhraseDictionaryTree(size_t noScoreComponent,
																					 FactorCollection *fc,
																					 FactorType ift,FactorType oft)
	: Dictionary(noScoreComponent),imp(new PDTimp),m_inFactorType(ift),m_outFactorType(oft) 
{
	imp->m_factorCollection=fc;
	if(sizeof(off_t)!=8)
		{
			std::cerr<<"ERROR: size of type 'off_t' has to be 64 bit!\n"
				"use compiler settings '-D_FILE_OFFSET_BITS=64 -D_LARGE_FILES'\n"
				" -> abort \n\n";
			abort();
		}
}

PhraseDictionaryTree::~PhraseDictionaryTree() 
{
	delete imp;
}

void PhraseDictionaryTree::SetFactorCollection(FactorCollection* fc) 
{
	imp->m_factorCollection=fc;
}

void PhraseDictionaryTree::FreeMemory() const
{
	imp->FreeMemory();
}

void PhraseDictionaryTree::
GetTargetCandidates(const std::vector<const Factor*>& src,
										std::vector<FactorTgtCand>& rv) const 
{
	IPhrase f(src.size());
	for(size_t i=0;i<src.size();++i) 
		{
			f[i]=imp->sv.index(src[i]->GetString());
			if(f[i]==InvalidLabelId) return;
		}

	TgtCands tgtCands;
	imp->GetTargetCandidates(f,tgtCands);

	imp->ConvertTgtCand(tgtCands,rv,m_outFactorType);

}
void PhraseDictionaryTree::
GetTargetCandidates(const std::vector<std::string>& src,
										std::vector<StringTgtCand>& rv) const 
{
	IPhrase f(src.size());
	for(size_t i=0;i<src.size();++i) 
		{
			f[i]=imp->sv.index(src[i]);
			if(f[i]==InvalidLabelId) return;
		}

	TgtCands tgtCands;
	imp->GetTargetCandidates(f,tgtCands);
	imp->ConvertTgtCand(tgtCands,rv);
}

void PhraseDictionaryTree::
PrintTargetCandidates(const std::vector<std::string>& src,
											std::ostream& out) const 
{
	IPhrase f(src.size());
	for(size_t i=0;i<src.size();++i)
	{
		f[i]=imp->sv.index(src[i]);
		if(f[i]==InvalidLabelId) 
			{
				std::cerr<<"the source phrase '"<<src<<"' contains an unknown word '"
								 <<src[i]<<"'\n";
				return;
			}
	}

	TgtCands tcand;
	imp->GetTargetCandidates(f,tcand);
	out<<"there are "<<tcand.size()<<" target candidates\n";
	imp->PrintTgtCand(tcand,out);
}

int PhraseDictionaryTree::Create(std::istream& inFile,const std::string& out) 
{
	std::string line;
	size_t count = 0;

	std::string ofn(out+".binphr.srctree"),
		oft(out+".binphr.tgtdata"),
		ofi(out+".binphr.idx"),
		ofsv(out+".binphr.srcvoc"),
		oftv(out+".binphr.tgtvoc");

  FILE *os=fOpen(ofn.c_str(),"wb"),
    *ot=fOpen(oft.c_str(),"wb");

  typedef PrefixTreeSA<LabelId,off_t> PSA;
  PSA *psa=new PSA;PSA::setDefault(InvalidOffT);

	LabelId currFirstWord=InvalidLabelId;
	IPhrase currF;
	TgtCands tgtCands;
	std::vector<off_t> vo;
	size_t lnc=0;
	while(getline(inFile, line)) 	
		{
			++lnc;
			std::istringstream is(line);std::string w;
			IPhrase f,e;Scores sc;
			
			while(is>>w && w!="|||") f.push_back(imp->sv.add(w));
			while(is>>w && w!="|||") e.push_back(imp->tv.add(w));
			while(is>>w && w!="|||") sc.push_back(atof(w.c_str()));
			

			if(f.empty()) 
				{
					std::cerr<<"WARNING: empty source phrase in line '"<<line<<"'\n";
					continue;
				}
			
			if(currFirstWord==InvalidLabelId) currFirstWord=f[0];
			if(currF.empty()) 
				{
					currF=f;
					// insert src phrase in prefix tree
					assert(psa);
					PSA::Data& d=psa->insert(f);
					if(d==InvalidOffT) d=fTell(ot);
					else 
						{
							std::cerr<<"ERROR: source phrase already inserted (A)!\nline: '"
											 <<line<<"'\nf: "<<f<<"\n";
							abort();
						}
				}

			if(currF!=f) 
				{
					// new src phrase
					currF=f;
					tgtCands.writeBin(ot);
					tgtCands.clear();
				
					if(++count%10000==0) 
						{
							std::cerr<<".";
							if(count%500000==0) std::cerr<<"[phrase:"<<count<<"]\n";
						}

					if(f[0]!=currFirstWord) 
						{
							// write src prefix tree to file and clear
							PTF pf;
							if(currFirstWord>=vo.size()) 
								vo.resize(currFirstWord+1,InvalidOffT);
							vo[currFirstWord]=fTell(os);
							pf.create(*psa,os);
							// clear
							delete psa;psa=new PSA;
							currFirstWord=f[0];
						}

					// insert src phrase in prefix tree
					assert(psa);
					PSA::Data& d=psa->insert(f);
					if(d==InvalidOffT) d=fTell(ot);
					else 
						{
							std::cerr<<"ERROR: source phrase already inserted (B)!\nline: '"
											 <<line<<"'\nf: "<<f<<"\n";
							abort();
						}
				}
			tgtCands.push_back(TgtCand(e,sc));
			assert(currFirstWord!=InvalidLabelId);
		}
  tgtCands.writeBin(ot);tgtCands.clear();

  std::cerr<<"total word count: "<<count<<" -- "<<vo.size()<<"  line count: "
					 <<lnc<<"  -- "<<currFirstWord<<"\n";

  PTF pf;
  if(currFirstWord>=vo.size()) vo.resize(currFirstWord+1,InvalidOffT);
  vo[currFirstWord]=fTell(os);
  pf.create(*psa,os);
  delete psa;psa=0;

 	fClose(os);
  fClose(ot);

  std::vector<size_t> inv;
  for(size_t i=0;i<vo.size();++i)
    if(vo[i]==InvalidOffT) inv.push_back(i);

  if(inv.size()) 
		{
			std::cerr<<"WARNING: there are src voc entries with no phrase "
				"translation: count "<<inv.size()<<"\n"
				"There exists phrase translations for "<<vo.size()-inv.size()
							 <<" entries\n";
		}
  
  FILE *oi=fOpen(ofi.c_str(),"wb");
  fWriteVector(oi,vo);
	fClose(oi);

	imp->sv.Write(ofsv);
	imp->tv.Write(oftv);

  return 1;
}


int PhraseDictionaryTree::Read(const std::string& fn) 
{
  std::cerr<<"size of off_t "<<sizeof(off_t)<<"\n";
	return imp->Read(fn);
} 


PhraseDictionaryTree::PrefixPtr PhraseDictionaryTree::GetRoot() const 
{
  return imp->GetRoot(); 
}

PhraseDictionaryTree::PrefixPtr 
PhraseDictionaryTree::Extend(PrefixPtr p, const std::string& w) const 
{
	return imp->Extend(p,w);
}

void PhraseDictionaryTree::PrintTargetCandidates(PrefixPtr p,std::ostream& out) const 
{
	
	TgtCands tcand;
	imp->GetTargetCandidates(p,tcand);
	out<<"there are "<<tcand.size()<<" target candidates\n";
	imp->PrintTgtCand(tcand,out);
}

void 
PhraseDictionaryTree::GetTargetCandidates(PrefixPtr p,
																					std::vector<FactorTgtCand>& rv) const
{
	TgtCands tcands;
	imp->GetTargetCandidates(p,tcands);
	imp->ConvertTgtCand(tcands,rv,m_outFactorType);
}
void PhraseDictionaryTree::
GetTargetCandidates(PrefixPtr p,
										std::vector<StringTgtCand>& rv) const 
{
	TgtCands tcands;
	imp->GetTargetCandidates(p,tcands);
	imp->ConvertTgtCand(tcands,rv);
}

////////////////////////////////////////////////////////////
//
// generate set of target candidates for confusion net
// later this part should be moved to a different file 
//
////////////////////////////////////////////////////////////


#include <numeric>
#include "Word.h"
#include "Phrase.h"
#include "ConfusionNet.h"
#include "WordsRange.h"

// Generates all tuples from  n indexes with ranges 0 to card[j]-1, respectively..
// Input: number of indexes and  ranges: ranges[0] ... ranges[num_idx-1] 
// Output: number of tuples and monodimensional array of tuples.
// Reference: mixed-radix generation algorithm (D. E. Knuth, TAOCP v. 4.2)

size_t GenerateTuples(unsigned num_idx,unsigned* ranges,unsigned *&tuples)
{
  unsigned* single_tuple= new unsigned[num_idx+1];
  unsigned num_tuples=1;

  for (unsigned k=0;k<num_idx;++k)
    {
      num_tuples *= ranges[k];
      single_tuple[k]=0;
    }

  tuples=new unsigned[num_idx * num_tuples];

  // we need this additional element for the last iteration
  single_tuple[num_idx]=0; 
  unsigned j=0;
  for (unsigned n=0;n<num_tuples;++n){
    memcpy((void *)((tuples + n * num_idx)),(void *)single_tuple,num_idx * sizeof(unsigned));
    j=0;
    while (single_tuple[j]==ranges[j]-1){single_tuple[j]=0; ++j;}
    ++single_tuple[j];
  }
	delete [] single_tuple;
  return num_tuples;
}


typedef PhraseDictionaryTree::PrefixPtr PPtr;
typedef std::vector<PPtr> vPPtr;
typedef std::vector<std::vector<Factor const*> > mPhrase;

std::ostream& operator<<(std::ostream& out,const mPhrase& p) {
	for(size_t i=0;i<p.size();++i) {
		out<<i<<" - ";
		for(size_t j=0;j<p[i].size();++j)
			out<<p[i][j]->ToString()<<" ";
		out<<"|";
	}

	return out;
}

struct State {
	vPPtr ptrs;
	WordsRange range;
	float score;

	State() : range(0,0),score(0.0) {}
	State(size_t b,size_t e,const vPPtr& v,float sc=0.0) : ptrs(v),range(b,e),score(sc) {}
	
	size_t begin() const {return range.GetStartPos();}
	size_t end() const {return range.GetEndPos();}
	float GetScore() const {return score;}

};

std::ostream& operator<<(std::ostream& out,const State& s) {
	out<<"["<<s.ptrs.size()<<" ("<<s.begin()<<","<<s.end()<<") "<<s.GetScore()<<"]";

	return out;
}

typedef std::map<mPhrase,float> E2Costs;


struct GCData {
	const std::vector<PhraseDictionaryTree const*>& pdicts;
	const std::vector<std::vector<float> >& weights;
	std::vector<FactorType> inF,outF;
	size_t distinctOutputFactors;
	vPPtr root;
	size_t totalTuples,distinctTuples;


	GCData(const std::vector<PhraseDictionaryTree const*>& a,
				 const std::vector<std::vector<float> >& b) 
		: pdicts(a),weights(b),totalTuples(0),distinctTuples(0) {

		assert(pdicts.size()==weights.size());
		std::set<FactorType> distinctOutFset;
		inF.resize(pdicts.size());
		outF.resize(pdicts.size());
		root.resize(pdicts.size());
		for(size_t i=0;i<pdicts.size();++i) 
			{
				root[i]=pdicts[i]->GetRoot();
				inF[i]=pdicts[i]->GetInputFactorType();
				outF[i]=pdicts[i]->GetOutputFactorType();
				distinctOutFset.insert(pdicts[i]->GetOutputFactorType());
			}
		distinctOutputFactors=distinctOutFset.size();
	}

	FactorType OutFT(size_t i) const {return outF[i];}
	FactorType InFT(size_t i) const {return inF[i];}
	size_t DistinctOutFactors() const {return distinctOutputFactors;}

	const vPPtr& GetRoot() const {return root;}

};

typedef std::vector<Factor const*> vFactor;
typedef std::vector<std::pair<float,vFactor> > TgtCandList;

typedef std::vector<TgtCandList> OutputFactor2TgtCandList;
typedef std::vector<OutputFactor2TgtCandList*> Len2Cands;

void GeneratePerFactorTgtList(size_t factorType,PPtr pptr,GCData& data,Len2Cands& len2cands)
{
	std::vector<FactorTgtCand> cands;
	data.pdicts[factorType]->GetTargetCandidates(pptr,cands);

	for(std::vector<FactorTgtCand>::const_iterator cand=cands.begin();cand!=cands.end();++cand) {
		assert(data.weights[factorType].size()==cand->second.size());
		float costs=std::inner_product(data.weights[factorType].begin(),
																	 data.weights[factorType].end(),
																	 cand->second.begin(),
																	 0.0);

		size_t len=cand->first.size();
		if(len>=len2cands.size()) len2cands.resize(len+1,0);
		if(!len2cands[len]) len2cands[len]=new OutputFactor2TgtCandList(data.DistinctOutFactors());
		OutputFactor2TgtCandList &outf2tcandlist=*len2cands[len];

		outf2tcandlist[data.OutFT(factorType)].push_back(std::make_pair(costs,cand->first));
	}
}

void GenerateTupleTgtCands(OutputFactor2TgtCandList& tCand,E2Costs& e2costs,GCData& data) 
{
	// check if candidates are non-empty
	bool gotCands=1;
	for(size_t j=0;gotCands && j<tCand.size();++j)
		gotCands &= !tCand[j].empty();
				
	if(gotCands) {
		// enumerate tuples
		assert(data.DistinctOutFactors()==tCand.size());
		std::vector<unsigned> radix(data.DistinctOutFactors());
		for(size_t i=0;i<tCand.size();++i) radix[i]=tCand[i].size();

		unsigned *tuples=0;
		size_t numTuples=GenerateTuples(radix.size(),&radix[0],tuples);

		data.totalTuples+=numTuples;

		for(size_t i=0;i<numTuples;++i)
			{
				mPhrase e(radix.size());float costs=0.0;
				for(size_t j=0;j<radix.size();++j)
					{
						assert(tuples[radix.size()*i+j]<tCand[j].size());
						std::pair<float,vFactor> const& mycand=tCand[j][tuples[radix.size()*i+j]];
						e[j]=mycand.second;
						costs+=mycand.first;
					}
#ifdef DEBUG
				bool mismatch=0;
				for(size_t j=1;!mismatch && j<e.size();++j)
					if(e[j].size()!=e[j-1].size()) mismatch=1;
				assert(mismatch==0);
#endif
				std::pair<E2Costs::iterator,bool> p=e2costs.insert(std::make_pair(e,costs));
				if(p.second) ++data.distinctTuples;
				else {
					// entry known, take min of costs, alternative: sum probs
					if(costs<p.first->second) p.first->second=costs;
				}
			}
		delete [] tuples;
	}
}

void GenerateCandidates_(E2Costs& e2costs,const vPPtr& nextP,GCData& data) 
{
	Len2Cands len2cands;
	// generate candidates for each element of nextP:
	for(size_t factorType=0;factorType<nextP.size();++factorType) 
		if(nextP[factorType]) 
			GeneratePerFactorTgtList(factorType,nextP[factorType],data,len2cands);

	// for each length: enumerate tuples, compute score, and insert in e2costs
	for(size_t len=0;len<len2cands.size();++len) if(len2cands[len]) 
		GenerateTupleTgtCands(*len2cands[len],e2costs,data);
}

void GenerateCandidates(const ConfusionNet& src,
												const std::vector<PhraseDictionaryTree const*>& pdicts,
												const std::vector<std::vector<float> >& weights,
												int verbose) {
	GCData data(pdicts,weights);

	std::vector<State> stack;
	for(size_t i=0;i<src.GetSize();++i) stack.push_back(State(i,i,data.GetRoot()));

	std::map<WordsRange,E2Costs> cov2E;

	//	std::cerr<<"start while loop. initial stack size: "<<stack.size()<<"\n";

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
			vPPtr nextP(curr.ptrs);
			for(size_t j=0;j<nextP.size();++j)
				nextP[j]=pdicts[j]->Extend(nextP[j],
																	 w.GetFactor(data.InFT(j))->GetString());
	
			bool valid=1;
			for(size_t j=0;j<nextP.size();++j) if(!nextP[j]) {valid=0;break;}

			if(valid) 
			{
				if(curr.end()+1<src.GetSize())
					stack.push_back(State(curr.begin(),curr.end()+1,nextP,
																curr.GetScore()+currCol[colidx].second));

				E2Costs &e2costs=cov2E[WordsRange(curr.begin(),curr.end()+1)];
				GenerateCandidates_(e2costs,nextP,data);
			}
		}
			
		// check if there are translations of one-word phrases ...
		//if(curr.begin()==curr.end() && tCand.empty()) {}		

	} // end while(!stack.empty()) 

	if(verbose) {
		// print statistics for debugging purposes
		std::cerr<<"tuple stats:  total: "<<data.totalTuples
						 <<" distinct: "<<data.distinctTuples<<" ("
						 <<(data.distinctTuples/(0.01*data.totalTuples))
						 <<"%)\n";
		std::cerr<<"per coverage set:\n";
		for(std::map<WordsRange,E2Costs>::const_iterator i=cov2E.begin();
				i!=cov2E.end();++i) {
			std::cerr<<i->first<<" -- distinct cands: "
							 <<i->second.size()<<"\n";
		}
		std::cerr<<"\n\n";
	}

	if(verbose>10) {
		std::cerr<<"full list:\n";
		for(std::map<WordsRange,E2Costs>::const_iterator i=cov2E.begin();
				i!=cov2E.end();++i) {
			std::cerr<<i->first<<" -- distinct cands: "
							 <<i->second.size()<<"\n";
			for(E2Costs::const_iterator j=i->second.begin();j!=i->second.end();++j)
				std::cerr<<j->first<<" -- "<<j->second<<"\n";
		}
	}
}




