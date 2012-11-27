
////////////////////////////////////////////////////////////
//
// generate set of target candidates for confusion net
//
////////////////////////////////////////////////////////////



#include <numeric>
#include "moses/Word.h"
#include "moses/Phrase.h"
#include "moses/ConfusionNet.h"
#include "moses/WordsRange.h"
#include "moses/TranslationModel/PhraseDictionaryTree.h"

using namespace Moses;

#if 0
// Generates all tuples from  n indexes with ranges 0 to card[j]-1, respectively..
// Input: number of indexes and  ranges: ranges[0] ... ranges[num_idx-1]
// Output: number of tuples and monodimensional array of tuples.
// Reference: mixed-radix generation algorithm (D. E. Knuth, TAOCP v. 4.2)

size_t GenerateTuples(unsigned num_idx,unsigned* ranges,unsigned *&tuples)
{
  unsigned* single_tuple= new unsigned[num_idx+1];
  unsigned num_tuples=1;

  for (unsigned k=0; k<num_idx; ++k) {
    num_tuples *= ranges[k];
    single_tuple[k]=0;
  }

  tuples=new unsigned[num_idx * num_tuples];

  // we need this additional element for the last iteration
  single_tuple[num_idx]=0;
  unsigned j=0;
  for (unsigned n=0; n<num_tuples; ++n) {
    memcpy((void *)((tuples + n * num_idx)),(void *)single_tuple,num_idx * sizeof(unsigned));
    j=0;
    while (single_tuple[j]==ranges[j]-1) {
      single_tuple[j]=0;
      ++j;
    }
    ++single_tuple[j];
  }
  delete [] single_tuple;
  return num_tuples;
}


typedef PhraseDictionaryTree::PrefixPtr PPtr;
typedef std::vector<PPtr> vPPtr;
typedef std::vector<std::vector<Factor const*> > mPhrase;

std::ostream& operator<<(std::ostream& out,const mPhrase& p)
{
  for(size_t i=0; i<p.size(); ++i) {
    out<<i<<" - ";
    for(size_t j=0; j<p[i].size(); ++j)
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

  size_t begin() const {
    return range.GetStartPos();
  }
  size_t end() const {
    return range.GetEndPos();
  }
  float GetScore() const {
    return score;
  }

};

std::ostream& operator<<(std::ostream& out,const State& s)
{
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

    CHECK(pdicts.size()==weights.size());
    std::set<FactorType> distinctOutFset;
    inF.resize(pdicts.size());
    outF.resize(pdicts.size());
    root.resize(pdicts.size());
    for(size_t i=0; i<pdicts.size(); ++i) {
      root[i]=pdicts[i]->GetRoot();
      inF[i]=pdicts[i]->GetInputFactorType();
      outF[i]=pdicts[i]->GetOutputFactorType();
      distinctOutFset.insert(pdicts[i]->GetOutputFactorType());
    }
    distinctOutputFactors=distinctOutFset.size();
  }

  FactorType OutFT(size_t i) const {
    return outF[i];
  }
  FactorType InFT(size_t i) const {
    return inF[i];
  }
  size_t DistinctOutFactors() const {
    return distinctOutputFactors;
  }

  const vPPtr& GetRoot() const {
    return root;
  }

};

typedef std::vector<Factor const*> vFactor;
typedef std::vector<std::pair<float,vFactor> > TgtCandList;

typedef std::vector<TgtCandList> OutputFactor2TgtCandList;
typedef std::vector<OutputFactor2TgtCandList*> Len2Cands;

void GeneratePerFactorTgtList(size_t factorType,PPtr pptr,GCData& data,Len2Cands& len2cands)
{
  std::vector<FactorTgtCand> cands;
  data.pdicts[factorType]->GetTargetCandidates(pptr,cands);

  for(std::vector<FactorTgtCand>::const_iterator cand=cands.begin(); cand!=cands.end(); ++cand) {
    CHECK(data.weights[factorType].size()==cand->second.size());
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
  for(size_t j=0; gotCands && j<tCand.size(); ++j)
    gotCands &= !tCand[j].empty();

  if(gotCands) {
    // enumerate tuples
    CHECK(data.DistinctOutFactors()==tCand.size());
    std::vector<unsigned> radix(data.DistinctOutFactors());
    for(size_t i=0; i<tCand.size(); ++i) radix[i]=tCand[i].size();

    unsigned *tuples=0;
    size_t numTuples=GenerateTuples(radix.size(),&radix[0],tuples);

    data.totalTuples+=numTuples;

    for(size_t i=0; i<numTuples; ++i) {
      mPhrase e(radix.size());
      float costs=0.0;
      for(size_t j=0; j<radix.size(); ++j) {
        CHECK(tuples[radix.size()*i+j]<tCand[j].size());
        std::pair<float,vFactor> const& mycand=tCand[j][tuples[radix.size()*i+j]];
        e[j]=mycand.second;
        costs+=mycand.first;
      }
#ifdef DEBUG
      bool mismatch=0;
      for(size_t j=1; !mismatch && j<e.size(); ++j)
        if(e[j].size()!=e[j-1].size()) mismatch=1;
      CHECK(mismatch==0);
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
  for(size_t factorType=0; factorType<nextP.size(); ++factorType)
    if(nextP[factorType])
      GeneratePerFactorTgtList(factorType,nextP[factorType],data,len2cands);

  // for each length: enumerate tuples, compute score, and insert in e2costs
  for(size_t len=0; len<len2cands.size(); ++len) if(len2cands[len])
      GenerateTupleTgtCands(*len2cands[len],e2costs,data);
}

void GenerateCandidates(const ConfusionNet& src,
                        const std::vector<PhraseDictionaryTree const*>& pdicts,
                        const std::vector<std::vector<float> >& weights,
                        int verbose)
{
  GCData data(pdicts,weights);

  std::vector<State> stack;
  for(size_t i=0; i<src.GetSize(); ++i) stack.push_back(State(i,i,data.GetRoot()));

  std::map<WordsRange,E2Costs> cov2E;

  //	std::cerr<<"start while loop. initial stack size: "<<stack.size()<<"\n";

  while(!stack.empty()) {
    State curr(stack.back());
    stack.pop_back();

    //std::cerr<<"processing state "<<curr<<" stack size: "<<stack.size()<<"\n";

    CHECK(curr.end()<src.GetSize());
    const ConfusionNet::Column &currCol=src[curr.end()];
    for(size_t colidx=0; colidx<currCol.size(); ++colidx) {
      const Word& w=currCol[colidx].first;
      vPPtr nextP(curr.ptrs);
      for(size_t j=0; j<nextP.size(); ++j)
        nextP[j]=pdicts[j]->Extend(nextP[j],
                                   w.GetFactor(data.InFT(j))->GetString());

      bool valid=1;
      for(size_t j=0; j<nextP.size(); ++j) if(!nextP[j]) {
          valid=0;
          break;
        }

      if(valid) {
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
        i!=cov2E.end(); ++i) {
      std::cerr<<i->first<<" -- distinct cands: "
               <<i->second.size()<<"\n";
    }
    std::cerr<<"\n\n";
  }

  if(verbose>10) {
    std::cerr<<"full list:\n";
    for(std::map<WordsRange,E2Costs>::const_iterator i=cov2E.begin();
        i!=cov2E.end(); ++i) {
      std::cerr<<i->first<<" -- distinct cands: "
               <<i->second.size()<<"\n";
      for(E2Costs::const_iterator j=i->second.begin(); j!=i->second.end(); ++j)
        std::cerr<<j->first<<" -- "<<j->second<<"\n";
    }
  }
}

#else

void GenerateCandidates(const ConfusionNet&,
                        const std::vector<PhraseDictionaryTree const*>&,
                        const std::vector<std::vector<float> >&,
                        int)
{
  std::cerr<<"ERROR: GenerateCandidates is currently broken\n";
}

#endif
