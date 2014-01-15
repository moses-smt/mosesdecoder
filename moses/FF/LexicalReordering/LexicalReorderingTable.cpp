#include "LexicalReorderingTable.h"
#include "moses/InputFileStream.h"
//#include "LVoc.h" //need IPhrase

#include "moses/StaticData.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/GenerationDictionary.h"
#include "moses/TargetPhrase.h"
#include "moses/TargetPhraseCollection.h"

#ifdef HAVE_CMPH
#include "moses/TranslationModel/CompactPT/LexicalReorderingTableCompact.h"
#endif

namespace Moses
{
/*
 * local helper functions
 */
//cleans str of leading and tailing spaces
std::string auxClearString(const std::string& str)
{
  int i = 0, j = str.size()-1;
  while(i <= j) {
    if(' ' != str[i]) {
      break;
    } else {
      ++i;
    }
  }
  while(j >= i) {
    if(' ' != str[j]) {
      break;
    } else {
      --j;
    }
  }
  return str.substr(i,j-i+1);
}

void auxAppend(IPhrase& head, const IPhrase& tail)
{
  head.reserve(head.size()+tail.size());
  for(size_t i = 0; i < tail.size(); ++i) {
    head.push_back(tail[i]);
  }
}
/*
 * functions for LexicalReorderingTable
 */

LexicalReorderingTable* LexicalReorderingTable::LoadAvailable(const std::string& filePath, const FactorList& f_factors, const FactorList& e_factors, const FactorList& c_factors)
{
  //decide use Compact or Tree or Memory table
  LexicalReorderingTable *compactLexr = NULL;
#ifdef HAVE_CMPH
  compactLexr = LexicalReorderingTableCompact::CheckAndLoad(filePath + ".minlexr", f_factors, e_factors, c_factors);
#endif
  if(compactLexr)
    return compactLexr;
  if(FileExists(filePath+".binlexr.idx")) {
    //there exists a binary version use that
    return new LexicalReorderingTableTree(filePath, f_factors, e_factors, c_factors);
  } else {
    //use plain memory
    return new LexicalReorderingTableMemory(filePath, f_factors, e_factors, c_factors);
  }
}

/*
 * functions for LexicalReorderingTableMemory
 */
LexicalReorderingTableMemory::LexicalReorderingTableMemory(
  const std::string& filePath,
  const std::vector<FactorType>& f_factors,
  const std::vector<FactorType>& e_factors,
  const std::vector<FactorType>& c_factors)
  : LexicalReorderingTable(f_factors, e_factors, c_factors)
{
  LoadFromFile(filePath);
}

LexicalReorderingTableMemory::~LexicalReorderingTableMemory()
{
}

std::vector<float>  LexicalReorderingTableMemory::GetScore(const Phrase& f,
    const Phrase& e,
    const Phrase& c)
{
  //rather complicated because of const can't use []... as [] might enter new things into std::map
  //also can't have to be careful with words range if c is empty can't use c.GetSize()-1 will underflow and be large
  TableType::const_iterator r;
  std::string key;
  if(0 == c.GetSize()) {
    key = MakeKey(f,e,c);
    r = m_Table.find(key);
    if(m_Table.end() != r) {
      return r->second;
    }
  } else {
    //right try from large to smaller context
    for(size_t i = 0; i <= c.GetSize(); ++i) {
      Phrase sub_c(c.GetSubString(WordsRange(i,c.GetSize()-1)));
      key = MakeKey(f,e,sub_c);
      r = m_Table.find(key);
      if(m_Table.end() != r) {
        return r->second;
      }
    }
  }
  return Scores();
}

void LexicalReorderingTableMemory::DbgDump(std::ostream* out) const
{
  TableType::const_iterator i;
  for(i = m_Table.begin(); i != m_Table.end(); ++i) {
    *out << " key: '" << i->first << "' score: ";
    *out << "(num scores: " << (i->second).size() << ")";
    for(size_t j = 0; j < (i->second).size(); ++j) {
      *out << (i->second)[j] << " ";
    }
    *out << "\n";
  }
};

std::string  LexicalReorderingTableMemory::MakeKey(const Phrase& f,
    const Phrase& e,
    const Phrase& c) const
{
  /*
  std::string key;
  if(!m_FactorsF.empty()){
    key += f.GetStringRep(m_FactorsF);
  }
  if(!m_FactorsE.empty()){
    if(!key.empty()){
      key += " ||| ";
    }
    key += e.GetStringRep(m_FactorsE);
  }
  */
  return MakeKey(auxClearString(f.GetStringRep(m_FactorsF)),
                 auxClearString(e.GetStringRep(m_FactorsE)),
                 auxClearString(c.GetStringRep(m_FactorsC)));
}

std::string  LexicalReorderingTableMemory::MakeKey(const std::string& f,
    const std::string& e,
    const std::string& c) const
{
  std::string key;
  if(!f.empty()) {
    key += f;
  }
  if(!m_FactorsE.empty()) {
    if(!key.empty()) {
      key += "|||";
    }
    key += e;
  }
  if(!m_FactorsC.empty()) {
    if(!key.empty()) {
      key += "|||";
    }
    key += c;
  }
  return key;
}

void  LexicalReorderingTableMemory::LoadFromFile(const std::string& filePath)
{
  std::string fileName = filePath;
  if(!FileExists(fileName) && FileExists(fileName+".gz")) {
    fileName += ".gz";
  }
  InputFileStream file(fileName);
  std::string line(""), key("");
  int numScores = -1;
  std::cerr << "Loading table into memory...";
  while(!getline(file, line).eof()) {
    std::vector<std::string> tokens = TokenizeMultiCharSeparator(line, "|||");
    int t = 0 ;
    std::string f(""),e(""),c("");

    if(!m_FactorsF.empty()) {
      //there should be something for f
      f = auxClearString(tokens.at(t));
      ++t;
    }
    if(!m_FactorsE.empty()) {
      //there should be something for e
      e = auxClearString(tokens.at(t));
      ++t;
    }
    if(!m_FactorsC.empty()) {
      //there should be something for c
      c = auxClearString(tokens.at(t));
      ++t;
    }
    //last token are the probs
    std::vector<float> p = Scan<float>(Tokenize(tokens.at(t)));
    //sanity check: all lines must have equall number of probs
    if(-1 == numScores) {
      numScores = (int)p.size(); //set in first line
    }
    if((int)p.size() != numScores) {
      TRACE_ERR( "found inconsistent number of probabilities... found " << p.size() << " expected " << numScores << std::endl);
      exit(0);
    }
    std::transform(p.begin(),p.end(),p.begin(),TransformScore);
    std::transform(p.begin(),p.end(),p.begin(),FloorScore);
    //save it all into our map
    m_Table[MakeKey(f,e,c)] = p;
  }
  std::cerr << "done.\n";
}

/*
 * functions for LexicalReorderingTableTree
 */
LexicalReorderingTableTree::LexicalReorderingTableTree(
  const std::string& filePath,
  const std::vector<FactorType>& f_factors,
  const std::vector<FactorType>& e_factors,
  const std::vector<FactorType>& c_factors)
  : LexicalReorderingTable(f_factors, e_factors, c_factors), m_UseCache(false), m_FilePath(filePath)
{
  m_Table.reset(new PrefixTreeMap());
  m_Table->Read(m_FilePath+".binlexr");
}

LexicalReorderingTableTree::~LexicalReorderingTableTree()
{
}

Scores LexicalReorderingTableTree::GetScore(const Phrase& f, const Phrase& e, const Phrase& c)
{
  if(   (!m_FactorsF.empty() && 0 == f.GetSize())
        || (!m_FactorsE.empty() && 0 == e.GetSize())) {
    //NOTE: no check for c as c might be empty, e.g. start of sentence
    //not a proper key
    // phi: commented out, since e may be empty (drop-unknown)
    //std::cerr << "Not a proper key!\n";
    return Scores();
  }
  CacheType::iterator i;;
  if(m_UseCache) {
    std::pair<CacheType::iterator, bool> r = m_Cache.insert(std::make_pair(MakeCacheKey(f,e),Candidates()));
    if(!r.second) {
      return auxFindScoreForContext((r.first)->second, c);
    }
    i = r.first;
  } else if(!m_Cache.empty()) {
    //although we might not be caching now, cache might be none empty!
    i = m_Cache.find(MakeCacheKey(f,e));
    if(i != m_Cache.end()) {
      return auxFindScoreForContext(i->second, c);
    }
  }
  //not in cache go to file...
  Scores      score;
  Candidates cands;
  m_Table->GetCandidates(MakeTableKey(f,e), &cands);
  if(cands.empty()) {
    return Scores();
  }

  if(m_FactorsC.empty()) {
	UTIL_THROW_IF2(1 != cands.size(), "Error");
    return cands[0].GetScore(0);
  } else {
    score = auxFindScoreForContext(cands, c);
  }
  //cache for future use
  if(m_UseCache) {
    i->second = cands;
  }
  return score;
};

Scores LexicalReorderingTableTree::auxFindScoreForContext(const Candidates& cands, const Phrase& context)
{
  if(m_FactorsC.empty()) {
	UTIL_THROW_IF2(cands.size() > 1, "Error");

    return (1 == cands.size())?(cands[0].GetScore(0)):(Scores());
  } else {
    std::vector<std::string> cvec;
    for(size_t i = 0; i < context.GetSize(); ++i) {
      /* old code
        std::string s = context.GetWord(i).ToString(m_FactorsC);
      cvec.push_back(s.substr(0,s.size()-1));
        */
      cvec.push_back(context.GetWord(i).GetString(m_FactorsC, false));
    }
    IPhrase c = m_Table->ConvertPhrase(cvec,TargetVocId);
    IPhrase sub_c;
    IPhrase::iterator start = c.begin();
    for(size_t j = 0; j <= context.GetSize(); ++j, ++start) {
      sub_c.assign(start, c.end());
      for(size_t cand = 0; cand < cands.size(); ++cand) {
        IPhrase p = cands[cand].GetPhrase(0);
        if(cands[cand].GetPhrase(0) == sub_c) {
          return cands[cand].GetScore(0);
        }
      }
    }
    return Scores();
  }
}

void LexicalReorderingTableTree::InitializeForInput(const InputType& input)
{
  ClearCache();
  if(ConfusionNet const* cn = dynamic_cast<ConfusionNet const*>(&input)) {
    Cache(*cn);
  } else if(Sentence const* s = dynamic_cast<Sentence const*>(&input)) {
    // Cache(*s); ... this just takes up too much memory, we cache elsewhere
    DisableCache();
  }
  if (!m_Table.get()) {
    //load thread specific table.
    m_Table.reset(new PrefixTreeMap());
    m_Table->Read(m_FilePath+".binlexr");
  }
};

bool LexicalReorderingTableTree::Create(std::istream& inFile,
                                        const std::string& outFileName)
{
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
  while(getline(inFile, line)) {
    ++lnc;
    if(0 == lnc % 10000) {
      TRACE_ERR(".");
    }
    IPhrase key;
    Scores   score;

    std::vector<std::string> tokens = TokenizeMultiCharSeparator(line, "|||");
    std::string w;
    if(1 == lnc) {
      //do some init stuff in the first line
      numTokens = tokens.size();
      if(tokens.size() == 2) { //f ||| score
        numKeyTokens = 1;
        voc[0] = new WordVoc();
        voc[1] = 0;
      } else if(3 == tokens.size() || 4 == tokens.size()) { //either f ||| e ||| score or f ||| e ||| c ||| score
        numKeyTokens = 2;
        voc[0] = new WordVoc(); //f voc
        voc[1] = new WordVoc(); //e voc
        voc[2] = voc[1];        //c & e share voc
      }
    } else {
      //sanity check ALL lines must have same number of tokens
      UTIL_THROW_IF2(numTokens != tokens.size(),
    		  "Lines do not have the same number of tokens");
    }
    size_t phrase = 0;
    for(; phrase < numKeyTokens; ++phrase) {
      //conditioned on more than just f... need |||
      if(phrase >=1) {
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
    for(size_t j = 0; j < tgt_phrases.size(); ++j, ++phrase) {
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
    std::vector<Scores> scores;
    scores.push_back(score);

    if(key.empty()) {
      TRACE_ERR("WARNING: empty source phrase in line '"<<line<<"'\n");
      continue;
    }
    //first time inits
    if(currFirstWord == InvalidLabelId) {
      currFirstWord = key[0];
    }
    if(currKey.empty()) {
      currKey = key;
      //insert key into tree
      UTIL_THROW_IF2(psa == NULL, "Object not yet created");
      PSA::Data& d = psa->insert(key);
      if(d == InvalidOffT) {
        d = fTell(ot);
      } else {
        TRACE_ERR("ERROR: source phrase already inserted (A)!\nline(" << lnc << "): '" << line << "\n");
        return false;
      }
    }
    if(currKey != key) {
      //ok new key
      currKey = key;
      //a) write cands for old key
      cands.writeBin(ot);
      cands.clear();
      //b) check if we need to move on to new tree root
      if(key[0] != currFirstWord) {
        // write key prefix tree to file and clear
        PTF pf;
        if(currFirstWord >= vo.size()) {
          vo.resize(currFirstWord+1,InvalidOffT);
        }
        vo[currFirstWord] = fTell(os);
        pf.create(*psa, os);
        // clear
        delete psa;
        psa = new PSA;
        currFirstWord = key[0];
      }
      //c) insert key into tree
      UTIL_THROW_IF2(psa == NULL, "Object not yet created");
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
  if (lnc == 0) {
    TRACE_ERR("ERROR: empty lexicalised reordering file\n" << std::endl);
    return false;
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

  if(voc[0]) {
    voc[0]->Write(ofsv);
    delete voc[0];
  }
  if(voc[1]) {
    voc[1]->Write(oftv);
    delete voc[1];
  }
  return true;
}

std::string LexicalReorderingTableTree::MakeCacheKey(const Phrase& f,
    const Phrase& e) const
{
  std::string key;
  if(!m_FactorsF.empty()) {
    key += auxClearString(f.GetStringRep(m_FactorsF));
  }
  if(!m_FactorsE.empty()) {
    if(!key.empty()) {
      key += "|||";
    }
    key += auxClearString(e.GetStringRep(m_FactorsE));
  }
  return key;
};

IPhrase LexicalReorderingTableTree::MakeTableKey(const Phrase& f,
    const Phrase& e) const
{
  IPhrase key;
  std::vector<std::string> keyPart;
  if(!m_FactorsF.empty()) {
    for(size_t i = 0; i < f.GetSize(); ++i) {
      /* old code
        std::string s = f.GetWord(i).ToString(m_FactorsF);
        keyPart.push_back(s.substr(0,s.size()-1));
        */
      keyPart.push_back(f.GetWord(i).GetString(m_FactorsF, false));
    }
    auxAppend(key, m_Table->ConvertPhrase(keyPart, SourceVocId));
    keyPart.clear();
  }
  if(!m_FactorsE.empty()) {
    if(!key.empty()) {
      key.push_back(PrefixTreeMap::MagicWord);
    }
    for(size_t i = 0; i < e.GetSize(); ++i) {
      /* old code
        std::string s = e.GetWord(i).ToString(m_FactorsE);
        keyPart.push_back(s.substr(0,s.size()-1));
        */
      keyPart.push_back(e.GetWord(i).GetString(m_FactorsE, false));
    }
    auxAppend(key, m_Table->ConvertPhrase(keyPart,TargetVocId));
    //keyPart.clear();
  }
  return key;
};


struct State {
  State(PPimp* t, const std::string& p) : pos(t), path(p) {
  }
  PPimp*      pos;
  std::string path;
};

void LexicalReorderingTableTree::auxCacheForSrcPhrase(const Phrase& f)
{
  if(m_FactorsE.empty()) {
    //f is all of key...
    Candidates cands;
    m_Table->GetCandidates(MakeTableKey(f,Phrase(ARRAY_SIZE_INCR)),&cands);
    m_Cache[MakeCacheKey(f,Phrase(ARRAY_SIZE_INCR))] = cands;
  } else {
    ObjectPool<PPimp>     pool;
    PPimp* pPos  = m_Table->GetRoot();
    //1) goto subtree for f
    for(size_t i = 0; i < f.GetSize() && 0 != pPos && pPos->isValid(); ++i) {
      /* old code
      pPos = m_Table.Extend(pPos, auxClearString(f.GetWord(i).ToString(m_FactorsF)), SourceVocId);
      */
      pPos = m_Table->Extend(pPos, f.GetWord(i).GetString(m_FactorsF, false), SourceVocId);
    }
    if(0 != pPos && pPos->isValid()) {
      pPos = m_Table->Extend(pPos, PrefixTreeMap::MagicWord);
    }
    if(0 == pPos || !pPos->isValid()) {
      return;
    }
    //2) explore whole subtree depth first & cache
    std::string cache_key = auxClearString(f.GetStringRep(m_FactorsF)) + "|||";

    std::vector<State> stack;
    stack.push_back(State(pool.get(PPimp(pPos->ptr()->getPtr(pPos->idx),0,0)),""));
    Candidates cands;
    while(!stack.empty()) {
      if(stack.back().pos->isValid()) {
        LabelId w = stack.back().pos->ptr()->getKey(stack.back().pos->idx);
        std::string next_path = stack.back().path + " " + m_Table->ConvertWord(w,TargetVocId);
        //cache this
        m_Table->GetCandidates(*stack.back().pos,&cands);
        if(!cands.empty()) {
          m_Cache[cache_key + auxClearString(next_path)] = cands;
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

void LexicalReorderingTableTree::Cache(const ConfusionNet& /*input*/)
{
  return;
}

void LexicalReorderingTableTree::Cache(const Sentence& input)
{
  //only works with sentences...
  size_t prev_cache_size = m_Cache.size();
  size_t max_phrase_length = input.GetSize();
  for(size_t len = 0; len <= max_phrase_length; ++len) {
    for(size_t start = 0; start+len <= input.GetSize(); ++start) {
      Phrase f    = input.GetSubString(WordsRange(start, start+len));
      auxCacheForSrcPhrase(f);
    }
  }
  std::cerr << "Cached " << m_Cache.size() - prev_cache_size << " new primary reordering table keys\n";
}
/*
Pre fetching implementation using Phrase and Generation Dictionaries
*//*
void LexicalReorderingTableTree::Cache(const ConfusionNet& input){
  typedef TargetPhraseCollection::iterator Iter;
  typedef TargetPhraseCollection::const_iterator ConstIter;
  //not implemented for confusion networks...
  Sentence const* s = dynamic_cast<Sentence const*>(&input);
  if(!s){
	return;
  }
  int max_phrase_length = input.GetSize();

  std::vector<PhraseDictionaryBase*> PhraseTables = StaticData::Instance()->GetPhraseDictionaries();
  //new code:
  //std::vector<PhraseDictionary*> PhraseTables = StaticData::Instance()->GetPhraseDictionaries();
  std::vector<GenerationDictionary*> GenTables = StaticData::Instance()->GetGenerationDictionaries();
  for(size_t len = 1; len <= max_phrase_length; ++len){
	for(size_t start = 0; start+len <= input.GetSize(); ++start){
	  Phrase f = s->GetSubString(WordsRange(start, start+len));
	  //find all translations of f
	  TargetPhraseCollection list;

	  for(size_t t = 0; t < PhraseTables.size(); ++t){
		//if(doIntersect(PhraseTables[t]->GetOutputFactorMask(),FactorMask(m_FactorsE))){
		  //this table gives us something we need

		  const TargetPhraseCollection* new_list = PhraseTables[t]->GetTargetPhraseCollection(f);
		  TargetPhraseCollection curr_list;
		  for(ConstIter i = new_list->begin(); i != new_list->end(); ++i){
			for(Iter j = list.begin(); j != list.end(); ++j){
			  curr_list.Add((*j)->MergeNext(*(*i)));
			}
		  }
		  if(list.IsEmpty()){
			list = *new_list;
		  } else {
			list = curr_list;
		  }
		  //}
	  }
	  for(size_t g = 0; g < GenTables.size(); ++g){
		//if(doIntersect(GenTables[g]->GetOutputFactorMask(),FactorMask(m_FactorsE))){
		  TargetPhraseCollection curr_list;
		  for(Iter j = list.begin(); j != list.end(); ++j){
			for(size_t w = 0; w < (*j)->GetSize(); ++w){
			  const OutputWordCollection* words = GenTables[g]->FindWord((*j)->GetWord(w));
			  for(OutputWordCollection::const_iterator i = words->begin(); i != words->end(); ++i){
				TargetPhrase* p = new TargetPhrase(*(*j));
				Word& pw = p->GetWord(w);
				pw.Merge(i->first);
				curr_list.Add(p);
			  }
			}
		  }
		  list = curr_list;
		  //}
	  }
	  //cache for each translation
	  for(Iter e = list.begin(); e < list.end(); ++e){
		Candidates cands;
		m_Table.GetCandidates(MakeTableKey(f,*(*e)), &cands);
		m_Cache.insert(std::make_pair(MakeCacheKey(f,*(*e)),cands));
	  }
	}
  }
};
*/

}

