#include "PDTAimp.h"

namespace Moses
{

PDTAimp::PDTAimp(PhraseDictionaryTreeAdaptor *p)
  : m_dict(0),
    m_obj(p),
    useCache(1),
    totalE(0),
    distinctE(0) {
  m_numInputScores = 0;
  m_inputFeature = &InputFeature::Instance();

  if (m_inputFeature) {
    const PhraseDictionary *firstPt = PhraseDictionary::GetColl()[0];
    if (firstPt == m_obj) {
      m_numInputScores = m_inputFeature->GetNumScoreComponents();
    }
  }
}

PDTAimp::~PDTAimp() {
  CleanUp();
  delete m_dict;

  if (StaticData::Instance().GetVerboseLevel() >= 2) {

    TRACE_ERR("tgt candidates stats:  total="<<totalE<<";  distinct="
              <<distinctE<<" ("<<distinctE/(0.01*totalE)<<");  duplicates="
              <<totalE-distinctE<<" ("<<(totalE-distinctE)/(0.01*totalE)
              <<")\n");

    TRACE_ERR("\npath statistics\n");

    if(path1Best.size()) {
      TRACE_ERR("1-best:        ");
      std::copy(path1Best.begin()+1,path1Best.end(),
                std::ostream_iterator<size_t>(std::cerr," \t"));
      TRACE_ERR("\n");
    }
    if(pathCN.size()) {
      TRACE_ERR("CN (full):     ");
      std::transform(pathCN.begin()+1
                     ,pathCN.end()
                     ,std::ostream_iterator<double>(std::cerr," \t")
                     ,Exp);
      TRACE_ERR("\n");
    }
    if(pathExplored.size()) {
      TRACE_ERR("CN (explored): ");
      std::copy(pathExplored.begin()+1,pathExplored.end(),
                std::ostream_iterator<size_t>(std::cerr," \t"));
      TRACE_ERR("\n");
    }
  }

}

void PDTAimp::CleanUp() {
  assert(m_dict);
  m_dict->FreeMemory();
  for(size_t i=0; i<m_tgtColls.size(); ++i) delete m_tgtColls[i];
  m_tgtColls.clear();
  m_cache.clear();
  m_rangeCache.clear();
  uniqSrcPhr.clear();
}

TargetPhraseCollectionWithSourcePhrase const*
PDTAimp::GetTargetPhraseCollection(Phrase const &src) const {

	assert(m_dict);
  if(src.GetSize()==0) return 0;

  std::pair<MapSrc2Tgt::iterator,bool> piter;
  if(useCache) {
    piter=m_cache.insert(std::make_pair(src,static_cast<TargetPhraseCollectionWithSourcePhrase const*>(0)));
    if(!piter.second) return piter.first->second;
  } else if (m_cache.size()) {
    MapSrc2Tgt::const_iterator i=m_cache.find(src);
    return (i!=m_cache.end() ? i->second : 0);
  }

  std::vector<std::string> srcString(src.GetSize());
  // convert source Phrase into vector of strings
  for(size_t i=0; i<srcString.size(); ++i) {
    Factors2String(src.GetWord(i),srcString[i]);
  }

  // get target phrases in string representation
  std::vector<StringTgtCand> cands;
  std::vector<std::string> wacands;
  m_dict->GetTargetCandidates(srcString,cands,wacands);
  if(cands.empty()) {
    return 0;
  }

  //TODO: Multiple models broken here
  std::vector<float> weights = StaticData::Instance().GetWeights(m_obj);

  std::vector<TargetPhrase> tCands;
  tCands.reserve(cands.size());

  std::vector<std::pair<float,size_t> > costs;
  costs.reserve(cands.size());

  std::vector<Phrase> sourcePhrases;
  sourcePhrases.reserve(cands.size());


  // convert into TargetPhrases
  for(size_t i=0; i<cands.size(); ++i) {
    TargetPhrase targetPhrase(m_obj);

    StringTgtCand::Tokens const& factorStrings=cands[i].tokens;
    Scores const& probVector=cands[i].scores;

    std::vector<float> scoreVector(probVector.size());
    std::transform(probVector.begin(),probVector.end(),scoreVector.begin(),
                   TransformScore);
    std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),
                   FloorScore);

    //sparse features.
    //These are already in log-space
    for (size_t j = 0; j < cands[i].fnames.size(); ++j) {
      targetPhrase.GetScoreBreakdown().Assign(m_obj, *cands[i].fnames[j], cands[i].fvalues[j]);
    }

    CreateTargetPhrase(targetPhrase,factorStrings,scoreVector, Scores(0), &wacands[i], &src);

    costs.push_back(std::make_pair(-targetPhrase.GetFutureScore(),tCands.size()));
    tCands.push_back(targetPhrase);

    sourcePhrases.push_back(src);
  }

  TargetPhraseCollectionWithSourcePhrase *rv;
  rv=PruneTargetCandidates(tCands,costs, sourcePhrases);
  if(rv->IsEmpty()) {
    delete rv;
    return 0;
  } else {
    if(useCache) piter.first->second=rv;
    m_tgtColls.push_back(rv);
    return rv;
  }

}

void PDTAimp::Create(const std::vector<FactorType> &input
            , const std::vector<FactorType> &output
            , const std::string &filePath
            , const std::vector<float> &weight
           ) {

  // set my members
  m_dict=new PhraseDictionaryTree();
  m_input=input;
  m_output=output;

  const StaticData &staticData = StaticData::Instance();
  m_dict->NeedAlignmentInfo(staticData.NeedAlignmentInfo());

  std::string binFname=filePath+".binphr.idx";
  if(!FileExists(binFname.c_str())) {
    UTIL_THROW2( "bin ttable does not exist");
    //TRACE_ERR( "bin ttable does not exist -> create it\n");
    //InputFileStream in(filePath);
    //m_dict->Create(in,filePath);
  }
  VERBOSE(1,"reading bin ttable\n");
//		m_dict->Read(filePath);
  bool res=m_dict->Read(filePath);
  if (!res) {
	std::cerr << "bin ttable was read in a wrong way\n";
    exit(1);
  }
}


void PDTAimp::CacheSource(ConfusionNet const& src) {
	assert(m_dict);
  const size_t srcSize=src.GetSize();

  std::vector<size_t> exploredPaths(srcSize+1,0);
  std::vector<double> exPathsD(srcSize+1,-1.0);

  // collect some statistics
  std::vector<size_t> cnDepths(srcSize,0);
  for(size_t i=0; i<srcSize; ++i) cnDepths[i]=src[i].size();

  for(size_t len=1; len<=srcSize; ++len)
    for(size_t i=0; i<=srcSize-len; ++i) {
      double pd=0.0;
      for(size_t k=i; k<i+len; ++k)	pd+=log(1.0*cnDepths[k]);
      exPathsD[len]=(exPathsD[len]>=0.0 ? addLogScale(pd,exPathsD[len]) : pd);
    }

  // update global statistics
  if(pathCN.size()<=srcSize) pathCN.resize(srcSize+1,-1.0);
  for(size_t len=1; len<=srcSize; ++len)
    pathCN[len]=pathCN[len]>=0.0 ? addLogScale(pathCN[len],exPathsD[len]) : exPathsD[len];

  if(path1Best.size()<=srcSize) path1Best.resize(srcSize+1,0);
  for(size_t len=1; len<=srcSize; ++len) path1Best[len]+=srcSize-len+1;


  if (StaticData::Instance().GetVerboseLevel() >= 2 && exPathsD.size()) {
    TRACE_ERR("path stats for current CN: \nCN (full):     ");
    std::transform(exPathsD.begin()+1
                   ,exPathsD.end()
                   ,std::ostream_iterator<double>(std::cerr," ")
                   ,Exp);
    TRACE_ERR("\n");
  }

  typedef StringTgtCand::Tokens sPhrase;
  typedef std::map<StringTgtCand::Tokens,TScores> E2Costs;

  std::map<Range,E2Costs> cov2cand;
  std::vector<State> stack;
  for(Position i=0 ; i < srcSize ; ++i)
    stack.push_back(State(i, i, m_dict->GetRoot(), std::vector<float>(m_numInputScores,0.0)));

  std::vector<float> weightTrans = StaticData::Instance().GetWeights(m_obj);
  std::vector<float> weightInput = StaticData::Instance().GetWeights(m_inputFeature);
  float weightWP = StaticData::Instance().GetWeightWordPenalty();

  while(!stack.empty()) {
    State curr(stack.back());
    stack.pop_back();

    UTIL_THROW_IF2(curr.end() >= srcSize, "Error");
    const ConfusionNet::Column &currCol=src[curr.end()];
    // in a given column, loop over all possibilities
    for(size_t colidx=0; colidx<currCol.size(); ++colidx) {
      const Word& w=currCol[colidx].first; // w=the i^th possibility in column colidx
      std::string s;
      Factors2String(w,s);
      bool isEpsilon=(s=="" || s==EPSILON);

      //assert that we have the right number of link params in this CN option
      UTIL_THROW_IF2(currCol[colidx].second.denseScores.size() < m_numInputScores,
      		"Incorrect number of input scores");

      // do not start with epsilon (except at first position)
      if(isEpsilon && curr.begin()==curr.end() && curr.begin()>0) continue;

      // At a given node in the prefix tree, look to see if w defines an edge to
      // another node (Extend).  Stay at the same node if w==EPSILON
      PPtr nextP = (isEpsilon ? curr.ptr : m_dict->Extend(curr.ptr,s));

      if(nextP) { // w is a word that should be considered
        Range newRange(curr.begin(),curr.end()+src.GetColumnIncrement(curr.end(),colidx));

        //add together the link scores from the current state and the new arc
        float inputScoreSum = 0;
        std::vector<float> newInputScores(m_numInputScores,0.0);
        if (m_numInputScores) {
          std::transform(currCol[colidx].second.denseScores.begin(), currCol[colidx].second.denseScores.end(),
                         curr.GetScores().begin(),
                         newInputScores.begin(),
                         std::plus<float>());


          //we need to sum up link weights (excluding realWordCount, which isn't in numLinkParams)
          //if the sum is too low, then we won't expand this.
          //TODO: dodgy! shouldn't we consider weights here? what about zero-weight params?
          inputScoreSum = std::accumulate(newInputScores.begin(),newInputScores.begin()+m_numInputScores,0.0);
        }

        Phrase newSrc(curr.src);
        if(!isEpsilon) newSrc.AddWord(w);
        if(newRange.second<srcSize && inputScoreSum>LOWEST_SCORE) {
          // if there is more room to grow, add a new state onto the queue
          // to be explored that represents [begin, curEnd+)
          stack.push_back(State(newRange,nextP,newInputScores));
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

        if(tcands.size()) {
          E2Costs& e2costs=cov2cand[newRange];
          Phrase const* srcPtr=uniqSrcPhr(newSrc);
          for(size_t i=0; i<tcands.size(); ++i) {
            //put input scores in first - already logged, just drop in directly
            std::vector<float> transcores(m_obj->GetNumScoreComponents());
            UTIL_THROW_IF2(transcores.size() != weightTrans.size(),
          		  "Incorrect number of translation scores");

            //put in phrase table scores, logging as we insert
            std::transform(tcands[i].scores.begin()
                           ,tcands[i].scores.end()
                           ,transcores.begin()
                           ,TransformScore);


            //tally up
            float score=std::inner_product(transcores.begin(), transcores.end(), weightTrans.begin(), 0.0f);

            // input feature
            score +=std::inner_product(newInputScores.begin(), newInputScores.end(), weightInput.begin(), 0.0f);

            //count word penalty
            score-=tcands[i].tokens.size() * weightWP;

            std::pair<E2Costs::iterator,bool> p=e2costs.insert(std::make_pair(tcands[i].tokens,TScores()));

            if(p.second) ++distinctE;

            TScores & scores=p.first->second;
            if(p.second || scores.total<score) {
              scores.total=score;
              scores.transScore=transcores;
              scores.inputScores=newInputScores;
              scores.src=srcPtr;
            }
          }
        }
      }
    }
  } // end while(!stack.empty())


  if (StaticData::Instance().GetVerboseLevel() >= 2 && exploredPaths.size()) {
    TRACE_ERR("CN (explored): ");
    std::copy(exploredPaths.begin()+1,exploredPaths.end(),
              std::ostream_iterator<size_t>(std::cerr," "));
    TRACE_ERR("\n");
  }

  if(pathExplored.size()<exploredPaths.size())
    pathExplored.resize(exploredPaths.size(),0);
  for(size_t len=1; len<=srcSize; ++len)
    pathExplored[len]+=exploredPaths[len];


  m_rangeCache.resize(src.GetSize(),vTPC(src.GetSize(),0));

  for(std::map<Range,E2Costs>::const_iterator i=cov2cand.begin(); i!=cov2cand.end(); ++i) {
    assert(i->first.first<m_rangeCache.size());
    assert(i->first.second>0);
    assert(static_cast<size_t>(i->first.second-1)<m_rangeCache[i->first.first].size());
    assert(m_rangeCache[i->first.first][i->first.second-1]==0);

    std::vector<TargetPhrase> tCands;
    tCands.reserve(i->second.size());

    std::vector<std::pair<float,size_t> > costs;
    costs.reserve(i->second.size());

    std::vector<Phrase> sourcePhrases;
    sourcePhrases.reserve(i->second.size());

    for(E2Costs::const_iterator j=i->second.begin(); j!=i->second.end(); ++j) {
      TScores const & scores=j->second;
      TargetPhrase targetPhrase(m_obj);
      CreateTargetPhrase(targetPhrase
                         , j ->first
                         , scores.transScore
                         , scores.inputScores
                         , NULL
                         , scores.src);
      costs.push_back(std::make_pair(-targetPhrase.GetFutureScore(),tCands.size()));
      tCands.push_back(targetPhrase);

      sourcePhrases.push_back(*scores.src);

      //std::cerr << i->first.first << "-" << i->first.second << ": " << targetPhrase << std::endl;
    }

    TargetPhraseCollectionWithSourcePhrase *rv=PruneTargetCandidates(tCands, costs, sourcePhrases);

    if(rv->IsEmpty())
      delete rv;
    else {
      m_rangeCache[i->first.first][i->first.second-1]=rv;
      m_tgtColls.push_back(rv);
    }
  }
  // free memory
  m_dict->FreeMemory();
}

void PDTAimp::CreateTargetPhrase(TargetPhrase& targetPhrase,
                        StringTgtCand::Tokens const& factorStrings,
                        Scores const& transVector,
                        Scores const& inputVector,
                        const std::string *alignmentString,
                        Phrase const* srcPtr) const {
  FactorCollection &factorCollection = FactorCollection::Instance();

  for(size_t k=0; k<factorStrings.size(); ++k) {
    util::TokenIter<util::MultiCharacter, false> word(*factorStrings[k], StaticData::Instance().GetFactorDelimiter());
    Word& w=targetPhrase.AddWord();
    for(size_t l=0; l<m_output.size(); ++l, ++word) {
      w[m_output[l]]= factorCollection.AddFactor(*word);
    }
  }

  if (alignmentString) {
    targetPhrase.SetAlignmentInfo(*alignmentString);
  }

  if (m_numInputScores) {
    targetPhrase.GetScoreBreakdown().Assign(m_inputFeature, inputVector);
  }

  targetPhrase.GetScoreBreakdown().Assign(m_obj, transVector);
  targetPhrase.EvaluateInIsolation(*srcPtr, m_obj->GetFeaturesToApply());
}

TargetPhraseCollectionWithSourcePhrase* PDTAimp::PruneTargetCandidates
(const std::vector<TargetPhrase> & tCands,
 std::vector<std::pair<float,size_t> >& costs,
 const std::vector<Phrase> &sourcePhrases) const {
  // convert into TargetPhraseCollection
  UTIL_THROW_IF2(tCands.size() != sourcePhrases.size(),
  		"Number of target phrases must equal number of source phrases");

  TargetPhraseCollectionWithSourcePhrase *rv=new TargetPhraseCollectionWithSourcePhrase;


  // set limit to tableLimit or actual size, whatever is smaller
  std::vector<std::pair<float,size_t> >::iterator nth =
    costs.begin() + ((m_obj->m_tableLimit>0 && // 0 indicates no limit
                      m_obj->m_tableLimit < costs.size()) ?
                     m_obj->m_tableLimit : costs.size());

  // find the nth phrase according to future cost
  NTH_ELEMENT3(costs.begin(),nth ,costs.end());

  // add n top phrases to the return list
  for(std::vector<std::pair<float,size_t> >::iterator
      it = costs.begin(); it != nth; ++it) {
    size_t ind = it->second;
    TargetPhrase *targetPhrase = new TargetPhrase(tCands[ind]);
    const Phrase &sourcePhrase = sourcePhrases[ind];
    rv->Add(targetPhrase, sourcePhrase);

  }

  return rv;
}

}


