#include "MaxentReorderingTable.h"
#include "InputFileStream.h"
//#include "LVoc.h" //need IPhrase

#include "StaticData.h"
#include "PhraseDictionary.h"
#include "GenerationDictionary.h"
#include "TargetPhrase.h"
#include "TargetPhraseCollection.h"

namespace Moses
{
/* 
 * local helper functions
 */
//cleans str of leading and tailing spaces
std::string maxent_auxClearString(const std::string& str){
  int i = 0, j = str.size()-1;
  while(i <= j){
    if(' ' != str[i]){
      break;
    } else {
      ++i;
    }
  }
  while(j >= i){
    if(' ' != str[j]){
      break;
    } else {
      --j;
    }
  }
  return str.substr(i,j-i+1);
}

void maxent_auxAppend(IPhrase& head, const IPhrase& tail){
  head.reserve(head.size()+tail.size());
  for(size_t i = 0; i < tail.size(); ++i){
	head.push_back(tail[i]);
  }
}

/* 
 * functions for MaxentReorderingTable
 */

MaxentReorderingTable* MaxentReorderingTable::LoadAvailable(const std::string& filePath, const FactorList& f_factors, const FactorList& e_factors, const FactorList& c_factors){
	//decide use Tree or Memory table
	if(FileExists(filePath+".binlexr.idx")){
	  //there exists a binary version use that
	  return new MaxentReorderingTableTree(filePath, f_factors, e_factors, c_factors);
	} else {
	  //use plain memory
	  return new MaxentReorderingTableMemory(filePath, f_factors, e_factors, c_factors);
	}
  }

/* 
 * functions for MaxentReorderingTableMemory
 */
MaxentReorderingTableMemory::MaxentReorderingTableMemory( 
				const std::string& filePath,
				const std::vector<FactorType>& f_factors, 
				const std::vector<FactorType>& e_factors,
				const std::vector<FactorType>& c_factors)
  : MaxentReorderingTable(f_factors, e_factors, c_factors) 
{
  LoadFromFile(filePath);
}

MaxentReorderingTableMemory::~MaxentReorderingTableMemory(){
}

std::vector<float>  MaxentReorderingTableMemory::GetScore(const Phrase& f,
														   const Phrase& e,
														   const Phrase& c, const Phrase& f_context) {
  //rather complicated because of const can't use []... as [] might enter new things into std::map
  //also can't have to be careful with words range if c is empty can't use c.GetSize()-1 will underflow and be large
//  std::cerr << "GetScore()\n";
  TableType::const_iterator r;
  std::string key;
  // table look up with f_context
  if(0 == c.GetSize()){
		key = MakeKey(f,e,c,f_context);
		r = m_Table.find(key);
		if(m_Table.end() != r){
//			std::cerr << "key found: " << key << "\n";
	  	return r->second;			
		}
		else{
			// TODO: how should this case be handled?
//			std::cerr << "key not found in table\n";
		}
  } else {
		//right try from large to smaller context
		for(size_t i = 0; i <= c.GetSize(); ++i){
	  	Phrase sub_c(c.GetSubString(WordsRange(i,c.GetSize()-1)));
	  	key = MakeKey(f,e,sub_c,f_context);
	  	r = m_Table.find(key);
	  	if(m_Table.end() != r){
			  return r->second;
	  	}
		}
  }
  return Score(); 
}

void MaxentReorderingTableMemory::DbgDump(std::ostream* out) const{
  TableType::const_iterator i;
  for(i = m_Table.begin(); i != m_Table.end(); ++i){
	*out << " key: '" << i->first << "' score: ";
	*out << "(num scores: " << (i->second).size() << ")";
	for(size_t j = 0; j < (i->second).size(); ++j){
	  *out << (i->second)[j] << " ";
	}
	*out << "\n";
  }
};

std::string MaxentReorderingTableMemory::MakeKey(const Phrase& f, 
												   const Phrase& e,
												   const Phrase& c,
												   const Phrase& f_context) const {
  // keys for maxent don't include the complete phrases, but only the first two words
  std::string f_first2words, e_first2words, f_cont;
  if(f.GetSize() > 1 )
  	f_first2words = f.GetSubString( WordsRange(0,1) ).GetStringRep(m_FactorsF);
  else if(f.GetSize() > 0 )
		f_first2words = f.GetSubString( WordsRange(0,1) ).GetStringRep(m_FactorsF);
  if(e.GetSize() > 1 )
		e_first2words = e.GetSubString( WordsRange(0,1) ).GetStringRep(m_FactorsE);
  else if(e.GetSize() > 0 )
		e_first2words = e.GetSubString( WordsRange(0,1) ).GetStringRep(m_FactorsE);
  f_cont = f_context.GetStringRep(m_FactorsF);
  delete &f_context;
  	return MakeKey(maxent_auxClearString(f_first2words),
				 maxent_auxClearString(e_first2words),
				 maxent_auxClearString(c.GetStringRep(m_FactorsC)),
				 maxent_auxClearString(f_cont));
}

std::string  MaxentReorderingTableMemory::MakeKey(const std::string& f, 
												   const std::string& e,
												   const std::string& c,
												   const std::string& f_context) const{
  std::string key;
  if(!f.empty()){
    key += f;
  }
  if(!m_FactorsE.empty()){
    if(!key.empty()){
      key += "|||";
    }
    key += e;
  }
  if(!m_FactorsC.empty()){
    if(!key.empty()){
      key += "|||";
    }
    key += c;
  }
  if(!f_context.empty()){
    if(!key.empty()){
      key += "|||";
    }
    key += f_context;
  }
  return key;
}

void  MaxentReorderingTableMemory::LoadFromFile(const std::string& filePath){
  std::string fileName = filePath;
  if(!FileExists(fileName) && FileExists(fileName+".gz")){
	fileName += ".gz";
  }
  InputFileStream file(fileName);
  std::string line(""), key("");
  int numScores = -1;
  std::cerr << "Loading table into memory...";
  while(!getline(file, line).eof()){
    std::vector<std::string> tokens = TokenizeMultiCharSeparator(line, "|||");
    int t = 0 ; 
    std::string f(""),e(""),c(""),f_context("");
      
    if(!m_FactorsF.empty()){
      //there should be something for f
//     f = maxent_auxClearString(tokens.at(t));
      std::vector< std::string > f_tmp = Tokenize(tokens.at(t));
      if(f_tmp.size() > 1)
      	f += f_tmp[0] + " " + f_tmp[1];
      else
      	f = f_tmp[0]; 	
      ++t;
    }
    if(!m_FactorsE.empty()){
      //there should be something for e
 //     e = maxent_auxClearString(tokens.at(t));
 			std::vector< std::string > e_tmp = Tokenize(tokens.at(t));
 			if(e_tmp.size() > 1)
      	e += e_tmp[0] + " " + e_tmp[1];
      else
      	e = e_tmp[0]; 
      ++t;
    }
    if(!m_FactorsC.empty()){
      //there should be something for c
      c = maxent_auxClearString(tokens.at(t));
      ++t;
    }
    // t points to lexicalized probabilities, skip them
    
    ++t;	// now points to previous source context
    if(!m_FactorsF.empty()){
      //there should be something for c
      f_context = maxent_auxClearString(tokens.at(t));
    }
    
    ++t;  // no points to maxent probabilities
    // maxent probabilities
    std::vector<float> p = Scan<float>(Tokenize(tokens.at(t)));
    
    //sanity check: all lines must have equall number of probs
    if(-1 == numScores){
      numScores = (int)p.size(); //set in first line
    }
    if((int)p.size() != numScores){
      TRACE_ERR( "found inconsistent number of probabilities... found " << p.size() << " expected " << numScores << std::endl);
      exit(0);
    }
    std::transform(p.begin(),p.end(),p.begin(),TransformScore);
    std::transform(p.begin(),p.end(),p.begin(),FloorScore);
    //save it all into our map
    // make table entry with f_context
    m_Table[MakeKey(f,e,c,f_context)] = p;
    // make table entry without f_context, if f_context is non-empty
    // table lookup without f_context, if the context is not-empty
	  if(f_context != "" ){
  		m_Table[MakeKey(f,e,c,"")] = p;
  	}
  }
  std::cerr << "done.\n";
}

/* 
 * functions for MaxentReorderingTableTree
 */
MaxentReorderingTableTree::MaxentReorderingTableTree(
			    const std::string& filePath,
			    const std::vector<FactorType>& f_factors, 
				const std::vector<FactorType>& e_factors,
			    const std::vector<FactorType>& c_factors)
  : MaxentReorderingTable(f_factors, e_factors, c_factors) 
{
  m_Table.Read(filePath+".binlexr"); 
}

MaxentReorderingTableTree::~MaxentReorderingTableTree(){
}

Score MaxentReorderingTableTree::GetScore(const Phrase& f, const Phrase& e, const Phrase& c, const Phrase& f_context) {
  if(   (!m_FactorsF.empty() && 0 == f.GetSize())
     || (!m_FactorsE.empty() && 0 == e.GetSize())){
    //NOTE: no check for c as c might be empty, e.g. start of sentence
    //not a proper key
    // phi: commented out, since e may be empty (drop-unknown)
    //std::cerr << "Not a proper key!\n";
    return Score();
  }
  CacheType::iterator i;;
  if(m_UseCache){
    std::pair<CacheType::iterator, bool> r = m_Cache.insert(std::make_pair(MakeCacheKey(f,e),Candidates()));
    if(!r.second){
      return auxFindScoreForContext((r.first)->second, c);
    }
    i = r.first;
  } else if(!m_Cache.empty()) { 
    //although we might not be caching now, cache might be none empty!
	i = m_Cache.find(MakeCacheKey(f,e));
    if(i != m_Cache.end()){
      return auxFindScoreForContext(i->second, c);
	}
  }
  //not in cache go to file...
  Score      score;
  Candidates cands; 
  m_Table.GetCandidates(MakeTableKey(f,e), &cands);
  if(cands.empty()){
    return Score();
  } 

  if(m_FactorsC.empty()){
	assert(1 == cands.size());
	return cands[0].GetScore(0);
  } else {
	score = auxFindScoreForContext(cands, c);
  }
  //cache for future use
  if(m_UseCache){
    i->second = cands;
  }
  return score;
};

Score MaxentReorderingTableTree::auxFindScoreForContext(const Candidates& cands, const Phrase& context){
  if(m_FactorsC.empty()){
	assert(cands.size() <= 1);
	return (1 == cands.size())?(cands[0].GetScore(0)):(Score());
  } else {
	std::vector<std::string> cvec;
	for(size_t i = 0; i < context.GetSize(); ++i){
	  /* old code
      std::string s = context.GetWord(i).ToString(m_FactorsC);
	  cvec.push_back(s.substr(0,s.size()-1));
      */
	  cvec.push_back(context.GetWord(i).GetString(m_FactorsC, false));
	}
	IPhrase c = m_Table.ConvertPhrase(cvec,TargetVocId);
	IPhrase sub_c;
	IPhrase::iterator start = c.begin();
	for(size_t j = 0; j <= context.GetSize(); ++j, ++start){
	  sub_c.assign(start, c.end()); 
	  for(size_t cand = 0; cand < cands.size(); ++cand){
		IPhrase p = cands[cand].GetPhrase(0);
		if(cands[cand].GetPhrase(0) == sub_c){
		  return cands[cand].GetScore(0);
		}
	  }
	}
	return Score();
  }
}

/*
void MaxentReorderingTableTree::DbgDump(std::ostream* pout){
  std::ostream& out = *pout; 
  //TODO!  
}
*/

void MaxentReorderingTableTree::InitializeForInput(const InputType& input){
  ClearCache();
  if(ConfusionNet const* cn = dynamic_cast<ConfusionNet const*>(&input)){
    Cache(*cn);
  } else if(Sentence const* s = dynamic_cast<Sentence const*>(&input)){
    // Cache(*s); ... this just takes up too much memory, we cache elsewhere
    DisableCache();
  }
};
 
bool MaxentReorderingTableTree::Create(std::istream& inFile, 
                                        const std::string& outFileName){
  std::string line;
  //TRACE_ERR("Entering Create...\n");  
  std::string 
    ofn(outFileName+".binlexr.srctree"),
    oft(outFileName+".binlexr.tgtdata"),
    ofi(outFileName+".binlexr.idx"),
    ofsv(outFileName+".binlexr.voc0"),
	oftv(outFileName+".binlexr.voc1");
  

  FILE *os = fOpen(ofn.c_str(),"wb");
  FILE *ot = fOpen(oft.c_str(),"wb");

  //TRACE_ERR("opend files....\n");

  typedef PrefixTreeSA<LabelId,OFF_T> PSA;
  PSA *psa = new PSA;
  PSA::setDefault(InvalidOffT);
  WordVoc* voc[3];
    
  LabelId currFirstWord = InvalidLabelId;
  IPhrase currKey;

  Candidates         cands;
  std::vector<OFF_T> vo;
  size_t lnc = 0;
  size_t numTokens    = 0;
  size_t numKeyTokens = 0;
  while(getline(inFile, line)){
	//TRACE_ERR(lnc<<":"<<line<<"\n");
    ++lnc;
	if(0 == lnc % 10000){
	  TRACE_ERR(".");
	}
    IPhrase key;
    Score   score;

    std::vector<std::string> tokens = TokenizeMultiCharSeparator(line, "|||");
    std::string w;
	if(1 == lnc){
	  //do some init stuff in the first line
	  numTokens = tokens.size();
	  if(tokens.size() == 2){ //f ||| score
		numKeyTokens = 1;
		voc[0] = new WordVoc();
		voc[1] = 0;
	  } else if(3 == tokens.size() || 4 == tokens.size()){ //either f ||| e ||| score or f ||| e ||| c ||| score
		numKeyTokens = 2;
		voc[0] = new WordVoc(); //f voc
		voc[1] = new WordVoc(); //e voc
		voc[2] = voc[1];        //c & e share voc
	  }
	} else {
	  //sanity check ALL lines must have same number of tokens
	  assert(numTokens == tokens.size());
	}
	int phrase = 0;
    for(; phrase < numKeyTokens; ++phrase){
      //conditioned on more than just f... need |||
	  if(phrase >=1){
		key.push_back(PrefixTreeMap::MagicWord);
	  }
      std::istringstream is(tokens[phrase]);
      while(is >> w) {
		key.push_back(voc[phrase]->add(w));
      }
    }
    //collect all non key phrases, i.e. c
    std::vector<IPhrase> tgt_phrases;
    tgt_phrases.resize(numTokens - numKeyTokens - 1);
    for(int j = 0; j < tgt_phrases.size(); ++j, ++phrase){
      std::istringstream is(tokens[numKeyTokens + j]);
      while(is >> w) {
		tgt_phrases[j].push_back(voc[phrase]->add(w));
      }
    }
    //last token is score
    std::istringstream is(tokens[numTokens-1]);
    while(is >> w) {
      score.push_back(atof(w.c_str()));
    }
    //transform score now...
    std::transform(score.begin(),score.end(),score.begin(),TransformScore);
    std::transform(score.begin(),score.end(),score.begin(),FloorScore);
    std::vector<Score> scores;
    scores.push_back(score);
    
    if(key.empty()) {
      TRACE_ERR("WARNING: empty source phrase in line '"<<line<<"'\n");
      continue;
    }
    //first time inits
    if(currFirstWord == InvalidLabelId){ 
      currFirstWord = key[0];
    }
    if(currKey.empty()){
      currKey = key;
      //insert key into tree
      assert(psa);
      PSA::Data& d = psa->insert(key);
      if(d == InvalidOffT) { 
		d = fTell(ot);
      } else {
		TRACE_ERR("ERROR: source phrase already inserted (A)!\nline(" << lnc << "): '" << line << "\n");
		return false;
      }
    }
    if(currKey != key){
      //ok new key
      currKey = key;
      //a) write cands for old key
      cands.writeBin(ot);
      cands.clear();
      //b) check if we need to move on to new tree root
      if(key[0] != currFirstWord){
		// write key prefix tree to file and clear
		PTF pf;
		if(currFirstWord >= vo.size()){ 
		  vo.resize(currFirstWord+1,InvalidOffT);
		}
		vo[currFirstWord] = fTell(os);
		pf.create(*psa, os);
		// clear
		delete psa; psa = new PSA;
		currFirstWord = key[0];
      }
      //c) insert key into tree
      assert(psa);
      PSA::Data& d = psa->insert(key);
      if(d == InvalidOffT) { 
		d = fTell(ot);
      } else {
		TRACE_ERR("ERROR: source phrase already inserted (A)!\nline(" << lnc << "): '" << line << "\n");
		return false;
      }
    }
	cands.push_back(GenericCandidate(tgt_phrases, scores));
  }
  //flush remainders
  cands.writeBin(ot);
  cands.clear();
  //process last currFirstWord
  PTF pf;
  if(currFirstWord >= vo.size()) {
    vo.resize(currFirstWord+1,InvalidOffT);
  }
  vo[currFirstWord] = fTell(os);
  pf.create(*psa,os);
  delete psa;
  psa=0;
  
  fClose(os);
  fClose(ot);
  /*
  std::vector<size_t> inv;
  for(size_t i = 0; i < vo.size(); ++i){
    if(vo[i] == InvalidOffT){ 
      inv.push_back(i);
    }
  }
  if(inv.size()) {
    TRACE_ERR("WARNING: there are src voc entries with no phrase "
	      "translation: count "<<inv.size()<<"\n"
	      "There exists phrase translations for "<<vo.size()-inv.size()
	      <<" entries\n");
  }
  */
  FILE *oi = fOpen(ofi.c_str(),"wb");
  fWriteVector(oi,vo);
  fClose(oi);
  
  if(voc[0]){
	voc[0]->Write(ofsv);
	delete voc[0];
  }
  if(voc[1]){
	voc[1]->Write(oftv);
	delete voc[1];
  }
  return true;
}

std::string MaxentReorderingTableTree::MakeCacheKey(const Phrase& f, 
						     const Phrase& e) const {
  std::string key;
  if(!m_FactorsF.empty()){
    key += maxent_auxClearString(f.GetStringRep(m_FactorsF));
  }
  if(!m_FactorsE.empty()){
    if(!key.empty()){
      key += "|||";
    }
    key += maxent_auxClearString(e.GetStringRep(m_FactorsE));
  }
  return key;
};

IPhrase MaxentReorderingTableTree::MakeTableKey(const Phrase& f, 
						 const Phrase& e) const {
  IPhrase key;
  std::vector<std::string> keyPart;
  if(!m_FactorsF.empty()){
    for(int i = 0; i < f.GetSize(); ++i){
	  /* old code
      std::string s = f.GetWord(i).ToString(m_FactorsF);
      keyPart.push_back(s.substr(0,s.size()-1));
      */
	  keyPart.push_back(f.GetWord(i).GetString(m_FactorsF, false));
    }
    maxent_auxAppend(key, m_Table.ConvertPhrase(keyPart, SourceVocId));
	keyPart.clear();
  }
  if(!m_FactorsE.empty()){
	if(!key.empty()){
      key.push_back(PrefixTreeMap::MagicWord);
	}
    for(int i = 0; i < e.GetSize(); ++i){
	  /* old code
      std::string s = e.GetWord(i).ToString(m_FactorsE);
      keyPart.push_back(s.substr(0,s.size()-1));
      */
	  keyPart.push_back(e.GetWord(i).GetString(m_FactorsE, false));
    }      
	maxent_auxAppend(key, m_Table.ConvertPhrase(keyPart,TargetVocId));
	//keyPart.clear();
  }
  return key;
};


struct State {
  State(PPimp* t, const std::string& p) : pos(t), path(p){
  }
  PPimp*      pos;
  std::string path;
};

void MaxentReorderingTableTree::auxCacheForSrcPhrase(const Phrase& f){
  if(m_FactorsE.empty()){
	//f is all of key...
	Candidates cands;
	m_Table.GetCandidates(MakeTableKey(f,Phrase(Output)),&cands);
	m_Cache[MakeCacheKey(f,Phrase(Output))] = cands;
  } else {
	ObjectPool<PPimp>     pool;
	PPimp* pPos  = m_Table.GetRoot();
	//1) goto subtree for f
	for(int i = 0; i < f.GetSize() && 0 != pPos && pPos->isValid(); ++i){
	  /* old code
	  pPos = m_Table.Extend(pPos, maxent_auxClearString(f.GetWord(i).ToString(m_FactorsF)), SourceVocId);
	  */
	  pPos = m_Table.Extend(pPos, f.GetWord(i).GetString(m_FactorsF, false), SourceVocId);
	}
	if(0 != pPos && pPos->isValid()){
	  pPos = m_Table.Extend(pPos, PrefixTreeMap::MagicWord);
	}
	if(0 == pPos || !pPos->isValid()){
	  return;
	}
	//2) explore whole subtree depth first & cache
	std::string cache_key = maxent_auxClearString(f.GetStringRep(m_FactorsF)) + "|||";
	
	std::vector<State> stack;
	stack.push_back(State(pool.get(PPimp(pPos->ptr()->getPtr(pPos->idx),0,0)),""));
	Candidates cands;
	while(!stack.empty()){
	  if(stack.back().pos->isValid()){
		LabelId w = stack.back().pos->ptr()->getKey(stack.back().pos->idx);
		std::string next_path = stack.back().path + " " + m_Table.ConvertWord(w,TargetVocId);
		//cache this 
		m_Table.GetCandidates(*stack.back().pos,&cands);
		if(!cands.empty()){ 
		  m_Cache[cache_key + maxent_auxClearString(next_path)] = cands;
		}
		cands.clear();
		PPimp* next_pos = pool.get(PPimp(stack.back().pos->ptr()->getPtr(stack.back().pos->idx),0,0));
		++stack.back().pos->idx;
		stack.push_back(State(next_pos,next_path));
	  } else {
		stack.pop_back();
	  }
	}
  }
}

void MaxentReorderingTableTree::Cache(const ConfusionNet& input){
  return;
}

void MaxentReorderingTableTree::Cache(const Sentence& input){
  //only works with sentences...
  int prev_cache_size = m_Cache.size();
  int max_phrase_length = input.GetSize();
  for(size_t len = 0; len <= max_phrase_length; ++len){ 
	for(size_t start = 0; start+len <= input.GetSize(); ++start){
	  Phrase f    = input.GetSubString(WordsRange(start, start+len));
	  auxCacheForSrcPhrase(f);
	}
  }
  std::cerr << "Cached " << m_Cache.size() - prev_cache_size << " new primary reordering table keys\n"; 
}


}

