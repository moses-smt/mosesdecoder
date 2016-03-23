#include "Model1Feature.h"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/FactorCollection.h"


using namespace std;

namespace Moses
{

const std::string Model1Vocabulary::GIZANULL = "GIZANULL";

Model1Vocabulary::Model1Vocabulary()
{
  FactorCollection &factorCollection = FactorCollection::Instance();
  m_NULL = factorCollection.AddFactor(GIZANULL,false);
  Store(m_NULL,0);
}

bool Model1Vocabulary::Store(const Factor* word, const unsigned id)
{
  boost::unordered_map<const Factor*, unsigned>::iterator iter = m_lookup.find( word );
  if ( iter != m_lookup.end() ) {
    return false;
  }
  m_lookup[ word ] = id;
  if ( m_vocab.size() <= id ) {
    m_vocab.resize(id+1);
  }
  m_vocab[id] = word;
  return true;
}

unsigned Model1Vocabulary::StoreIfNew(const Factor* word)
{
  boost::unordered_map<const Factor*, unsigned>::iterator iter = m_lookup.find( word );

  if ( iter != m_lookup.end() ) {
    return iter->second;
  }

  unsigned id = m_vocab.size();
  m_vocab.push_back( word );
  m_lookup[ word ] = id;
  return id;
}

unsigned Model1Vocabulary::GetWordID(const Factor* word) const
{
  boost::unordered_map<const Factor*, unsigned>::const_iterator iter = m_lookup.find( word );
  if ( iter == m_lookup.end() ) {
    return INVALID_ID;
  }
  return iter->second;
}

const Factor* Model1Vocabulary::GetWord(unsigned id) const
{
  if (id >= m_vocab.size()) {
    return NULL;
  }
  return m_vocab[ id ];
}

void Model1Vocabulary::Load(const std::string& fileName)
{
  InputFileStream inFile(fileName);
  FactorCollection &factorCollection = FactorCollection::Instance();
  std::string line;

  unsigned i = 0;
  if ( getline(inFile, line) ) { // first line of MGIZA vocabulary files seems to be special : "1       UNK     0"  -- skip if it's this
    ++i;
    std::vector<std::string> tokens = Tokenize(line);
    UTIL_THROW_IF2(tokens.size()!=3, "Line " << i << " in " << fileName << " has wrong number of tokens.");
    unsigned id = atoll( tokens[0].c_str() );
    if (! ( (id == 1) && (tokens[1] == "UNK") )) {
      const Factor* factor = factorCollection.AddFactor(tokens[1],false); // TODO: can we assume that the vocabulary is know and filter the model on loading?
      bool stored = Store(factor, id);
      UTIL_THROW_IF2(!stored, "Line " << i << " in " << fileName << " overwrites existing vocabulary entry.");
    }
  }
  while ( getline(inFile, line) ) {
    ++i;
    std::vector<std::string> tokens = Tokenize(line);
    UTIL_THROW_IF2(tokens.size()!=3, "Line " << i << " in " << fileName << " has wrong number of tokens.");
    unsigned id = atoll( tokens[0].c_str() );
    const Factor* factor = factorCollection.AddFactor(tokens[1],false); // TODO: can we assume that the vocabulary is know and filter the model on loading?
    bool stored = Store(factor, id);
    UTIL_THROW_IF2(!stored, "Line " << i << " in " << fileName << " overwrites existing vocabulary entry.");
  }
  inFile.Close();
}


void Model1LexicalTable::Load(const std::string &fileName, const Model1Vocabulary& vcbS, const Model1Vocabulary& vcbT)
{
  InputFileStream inFile(fileName);
  std::string line;

  unsigned i = 0;
  while ( getline(inFile, line) ) {
    ++i;
    std::vector<std::string> tokens = Tokenize(line);
    UTIL_THROW_IF2(tokens.size()!=3, "Line " << i << " in " << fileName << " has wrong number of tokens.");
    unsigned idS = atoll( tokens[0].c_str() );
    unsigned idT = atoll( tokens[1].c_str() );
    const Factor* wordS = vcbS.GetWord(idS);
    const Factor* wordT = vcbT.GetWord(idT);
    float prob = std::atof( tokens[2].c_str() );
    if ( (wordS != NULL) && (wordT != NULL) ) {
      m_ltable[ wordS ][ wordT ] = prob;
    }
    UTIL_THROW_IF2((wordS == NULL) || (wordT == NULL), "Line " << i << " in " << fileName << " has unknown vocabulary."); // TODO: can we assume that the vocabulary is know and filter the model on loading? Then remove this line.
  }
  inFile.Close();
}

// p( wordT | wordS )
float Model1LexicalTable::GetProbability(const Factor* wordS, const Factor* wordT) const
{
  float prob = m_floor;

  boost::unordered_map< const Factor*, boost::unordered_map< const Factor*, float > >::const_iterator iter1 = m_ltable.find( wordS );

  if ( iter1 != m_ltable.end() ) {
    boost::unordered_map< const Factor*, float >::const_iterator iter2 = iter1->second.find( wordT );
    if ( iter2 != iter1->second.end() ) {
      prob = iter2->second;
      if ( prob < m_floor ) {
        prob = m_floor;
      }
    }
  }
  return prob;
}


Model1Feature::Model1Feature(const std::string &line)
  : StatelessFeatureFunction(1, line)
  , m_skipTargetPunctuation(false)
  , m_is_syntax(false)
{
  VERBOSE(1, "Initializing feature " << GetScoreProducerDescription() << " ...");
  ReadParameters();
  VERBOSE(1, " Done.");
}

void Model1Feature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    m_fileNameModel1 = value;
  } else if (key == "source-vocabulary") {
    m_fileNameVcbS = value;
  } else if (key == "target-vocabulary") {
    m_fileNameVcbT = value;
  } else if (key == "skip-target-punctuation") {
    m_skipTargetPunctuation = Scan<bool>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

void Model1Feature::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  m_is_syntax = is_syntax(opts->search.algo);

  FEATUREVERBOSE(2, GetScoreProducerDescription() << ": Loading source vocabulary from file " << m_fileNameVcbS << " ...");
  Model1Vocabulary vcbS;
  vcbS.Load(m_fileNameVcbS);
  FEATUREVERBOSE2(2, " Done." << std::endl);
  FEATUREVERBOSE(2, GetScoreProducerDescription() << ": Loading target vocabulary from file " << m_fileNameVcbT << " ...");
  Model1Vocabulary vcbT;
  vcbT.Load(m_fileNameVcbT);
  FEATUREVERBOSE2(2, " Done." << std::endl);
  FEATUREVERBOSE(2, GetScoreProducerDescription() << ": Loading model 1 lexical translation table from file " << m_fileNameModel1 << " ...");
  m_model1.Load(m_fileNameModel1,vcbS,vcbT);
  FEATUREVERBOSE2(2, " Done." << std::endl);
  FactorCollection &factorCollection = FactorCollection::Instance();
  m_emptyWord = factorCollection.GetFactor(Model1Vocabulary::GIZANULL,false);
  UTIL_THROW_IF2(m_emptyWord==NULL, GetScoreProducerDescription()
                 << ": Factor for GIZA empty word does not exist.");

  if (m_skipTargetPunctuation) {
    const std::string punctuation = ",;.:!?";
    for (size_t i=0; i<punctuation.size(); ++i) {
      const std::string punct = punctuation.substr(i,1);
      FactorCollection &factorCollection = FactorCollection::Instance();
      const Factor* punctFactor = factorCollection.AddFactor(punct,false);
      std::pair<std::set<const Factor*>::iterator,bool> inserted = m_punctuation.insert(punctFactor);
    }
  }
}

void Model1Feature::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedScores) const
{
  const Sentence& sentence = static_cast<const Sentence&>(input);
  float score = 0.0;
  float norm = TransformScore(1+sentence.GetSize());

  for (size_t posT=0; posT<targetPhrase.GetSize(); ++posT) {
    const Word &wordT = targetPhrase.GetWord(posT);
    if (m_skipTargetPunctuation) {
      std::set<const Factor*>::const_iterator foundPunctuation = m_punctuation.find(wordT[0]);
      if (foundPunctuation != m_punctuation.end()) {
        continue;
      }
    }
    if ( !wordT.IsNonTerminal() ) {
      float thisWordProb = m_model1.GetProbability(m_emptyWord,wordT[0]); // probability conditioned on empty word

      // cache lookup
      bool foundInCache = false;
      {
#ifdef WITH_THREADS
        boost::shared_lock<boost::shared_mutex> read_lock(m_accessLock);
#endif
        boost::unordered_map<const InputType*, boost::unordered_map<const Factor*, float> >::const_iterator sentenceCache = m_cache.find(&input);
        if (sentenceCache != m_cache.end()) {
          boost::unordered_map<const Factor*, float>::const_iterator cacheHit = sentenceCache->second.find(wordT[0]);
          if (cacheHit != sentenceCache->second.end()) {
            foundInCache = true;
            score += cacheHit->second;
            FEATUREVERBOSE(3, "Cached score( " << wordT << " ) = " << cacheHit->second << std::endl);
          }
        }
      }

      if (!foundInCache) {
        for (size_t posS=(m_is_syntax?1:0); posS<(m_is_syntax?sentence.GetSize()-1:sentence.GetSize()); ++posS) { // ignore <s> and </s>
          const Word &wordS = sentence.GetWord(posS);
          float modelProb = m_model1.GetProbability(wordS[0],wordT[0]);
          FEATUREVERBOSE(4, "p( " << wordT << " | " << wordS << " ) = " << modelProb << std::endl);
          thisWordProb += modelProb;
        }
        float thisWordScore = TransformScore(thisWordProb) - norm;
        FEATUREVERBOSE(3, "score( " << wordT << " ) = " << thisWordScore << std::endl);
        {
#ifdef WITH_THREADS
          // need to update cache; write lock
          boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
#endif
          m_cache[&input][wordT[0]] = thisWordScore;
        }
        score += thisWordScore;
      }
    }
  }

  scoreBreakdown.PlusEquals(this, score);
}

void Model1Feature::CleanUpAfterSentenceProcessing(const InputType& source)
{
#ifdef WITH_THREADS
  // need to update cache; write lock
  boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
#endif
  // clear cache
  boost::unordered_map<const InputType*, boost::unordered_map<const Factor*, float> >::iterator sentenceCache = m_cache.find(&source);
  if (sentenceCache != m_cache.end()) {
    sentenceCache->second.clear();
    m_cache.erase(sentenceCache);
  }
}

}

