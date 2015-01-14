#include <fstream>
#include "GlobalLexicalModel.h"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "moses/TranslationOption.h"
#include "moses/FactorCollection.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
GlobalLexicalModel::GlobalLexicalModel(const std::string &line)
  : StatelessFeatureFunction(1, line)
{
  std::cerr << "Creating global lexical model...\n";
  ReadParameters();

  // define bias word
  FactorCollection &factorCollection = FactorCollection::Instance();
  m_bias = new Word();
  const Factor* factor = factorCollection.AddFactor( Input, m_inputFactorsVec[0], "**BIAS**" );
  m_bias->SetFactor( m_inputFactorsVec[0], factor );

}

void GlobalLexicalModel::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    m_filePath = value;
  } else if (key == "input-factor") {
    m_inputFactorsVec = Tokenize<FactorType>(value,",");
  } else if (key == "output-factor") {
    m_outputFactorsVec = Tokenize<FactorType>(value,",");
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

GlobalLexicalModel::~GlobalLexicalModel()
{
  // delete words in the hash data structure
  DoubleHash::const_iterator iter;
  for(iter = m_hash.begin(); iter != m_hash.end(); iter++ ) {
    map< const Word*, float, WordComparer >::const_iterator iter2;
    for(iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++ ) {
      delete iter2->first; // delete input word
    }
    delete iter->first; // delete output word
  }
}

void GlobalLexicalModel::Load()
{
  FactorCollection &factorCollection = FactorCollection::Instance();
  const std::string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();

  VERBOSE(2, "Loading global lexical model from file " << m_filePath << endl);

  m_inputFactors = FactorMask(m_inputFactorsVec);
  m_outputFactors = FactorMask(m_outputFactorsVec);
  InputFileStream inFile(m_filePath);

  // reading in data one line at a time
  size_t lineNum = 0;
  string line;
  while(getline(inFile, line)) {
    ++lineNum;
    vector<string> token = Tokenize<string>(line, " ");

    if (token.size() != 3) { // format checking
      UTIL_THROW2("Syntax error at " << m_filePath << ":" << lineNum << ":" << line);
    }

    // create the output word
    Word *outWord = new Word();
    vector<string> factorString = Tokenize( token[0], factorDelimiter );
    for (size_t i=0 ; i < m_outputFactorsVec.size() ; i++) {
      const FactorDirection& direction = Output;
      const FactorType& factorType = m_outputFactorsVec[i];
      const Factor* factor = factorCollection.AddFactor( direction, factorType, factorString[i] );
      outWord->SetFactor( factorType, factor );
    }

    // create the input word
    Word *inWord = new Word();
    factorString = Tokenize( token[1], factorDelimiter );
    for (size_t i=0 ; i < m_inputFactorsVec.size() ; i++) {
      const FactorDirection& direction = Input;
      const FactorType& factorType = m_inputFactorsVec[i];
      const Factor* factor = factorCollection.AddFactor( direction, factorType, factorString[i] );
      inWord->SetFactor( factorType, factor );
    }

    // maximum entropy feature score
    float score = Scan<float>(token[2]);

    // std::cerr << "storing word " << *outWord << " " << *inWord << " " << score << endl;

    // store feature in hash
    DoubleHash::iterator keyOutWord = m_hash.find( outWord );
    if( keyOutWord == m_hash.end() ) {
      m_hash[outWord][inWord] = score;
    } else { // already have hash for outword, delete the word to avoid leaks
      (keyOutWord->second)[inWord] = score;
      delete outWord;
    }
  }
}

void GlobalLexicalModel::InitializeForInput( Sentence const& in )
{
  m_local.reset(new ThreadLocalStorage);
  m_local->input = &in;
}

float GlobalLexicalModel::ScorePhrase( const TargetPhrase& targetPhrase ) const
{
  const Sentence& input = *(m_local->input);
  float score = 0;
  for(size_t targetIndex = 0; targetIndex < targetPhrase.GetSize(); targetIndex++ ) {
    float sum = 0;
    const Word& targetWord = targetPhrase.GetWord( targetIndex );
    VERBOSE(2,"glm " << targetWord << ": ");
    const DoubleHash::const_iterator targetWordHash = m_hash.find( &targetWord );
    if( targetWordHash != m_hash.end() ) {
      SingleHash::const_iterator inputWordHash = targetWordHash->second.find( m_bias );
      if( inputWordHash != targetWordHash->second.end() ) {
        VERBOSE(2,"*BIAS* " << inputWordHash->second);
        sum += inputWordHash->second;
      }

      set< const Word*, WordComparer > alreadyScored; // do not score a word twice
      for(size_t inputIndex = 0; inputIndex < input.GetSize(); inputIndex++ ) {
        const Word& inputWord = input.GetWord( inputIndex );
        if ( alreadyScored.find( &inputWord ) == alreadyScored.end() ) {
          SingleHash::const_iterator inputWordHash = targetWordHash->second.find( &inputWord );
          if( inputWordHash != targetWordHash->second.end() ) {
            VERBOSE(2," " << inputWord << " " << inputWordHash->second);
            sum += inputWordHash->second;
          }
          alreadyScored.insert( &inputWord );
        }
      }
    }
    // Hal Daume says: 1/( 1 + exp [ - sum_i w_i * f_i ] )
    VERBOSE(2," p=" << FloorScore( log(1/(1+exp(-sum))) ) << endl);
    score += FloorScore( log(1/(1+exp(-sum))) );
  }
  return score;
}

float GlobalLexicalModel::GetFromCacheOrScorePhrase( const TargetPhrase& targetPhrase ) const
{
  LexiconCache& m_cache = m_local->cache;
  const LexiconCache::const_iterator query = m_cache.find( &targetPhrase );
  if ( query != m_cache.end() ) {
    return query->second;
  }

  float score = ScorePhrase( targetPhrase );
  m_cache.insert( pair<const TargetPhrase*, float>(&targetPhrase, score) );
  //VERBOSE(2, "add to cache " << targetPhrase << ": " << score << endl);
  return score;
}

void GlobalLexicalModel::EvaluateInIsolation(const Phrase &source
    , const TargetPhrase &targetPhrase
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedFutureScore) const
{
  scoreBreakdown.PlusEquals( this, GetFromCacheOrScorePhrase(targetPhrase) );
}

bool GlobalLexicalModel::IsUseable(const FactorMask &mask) const
{
  for (size_t i = 0; i < m_outputFactors.size(); ++i) {
    if (m_outputFactors[i]) {
      if (!mask[i]) {
        return false;
      }
    }
  }

  return true;
}

}
