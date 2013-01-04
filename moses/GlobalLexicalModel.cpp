#include <fstream>
#include "GlobalLexicalModel.h"
#include "StaticData.h"
#include "InputFileStream.h"
#include "TranslationOption.h"
#include "UserMessage.h"

using namespace std;

namespace Moses
{
GlobalLexicalModel::GlobalLexicalModel(const string &filePath,
                                       const vector< FactorType >& inFactors,
                                       const vector< FactorType >& outFactors)
  : StatelessFeatureFunction("GlobalLexicalModel",1)
{
  std::cerr << "Creating global lexical model...\n";

  // load model
  LoadData( filePath, inFactors, outFactors );

  // define bias word
  FactorCollection &factorCollection = FactorCollection::Instance();
  m_bias = new Word();
  const Factor* factor = factorCollection.AddFactor( Input, inFactors[0], "**BIAS**" );
  m_bias->SetFactor( inFactors[0], factor );
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

void GlobalLexicalModel::LoadData(const string &filePath,
                                  const vector< FactorType >& inFactors,
                                  const vector< FactorType >& outFactors)
{
  FactorCollection &factorCollection = FactorCollection::Instance();
  const std::string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();

  VERBOSE(2, "Loading global lexical model from file " << filePath << endl);

  m_inputFactors = FactorMask(inFactors);
  m_outputFactors = FactorMask(outFactors);
  InputFileStream inFile(filePath);

  // reading in data one line at a time
  size_t lineNum = 0;
  string line;
  while(getline(inFile, line)) {
    ++lineNum;
    vector<string> token = Tokenize<string>(line, " ");

    if (token.size() != 3) { // format checking
      stringstream errorMessage;
      errorMessage << "Syntax error at " << filePath << ":" << lineNum << endl << line << endl;
      UserMessage::Add(errorMessage.str());
      abort();
    }

    // create the output word
    Word *outWord = new Word();
    vector<string> factorString = Tokenize( token[0], factorDelimiter );
    for (size_t i=0 ; i < outFactors.size() ; i++) {
      const FactorDirection& direction = Output;
      const FactorType& factorType = outFactors[i];
      const Factor* factor = factorCollection.AddFactor( direction, factorType, factorString[i] );
      outWord->SetFactor( factorType, factor );
    }

    // create the input word
    Word *inWord = new Word();
    factorString = Tokenize( token[1], factorDelimiter );
    for (size_t i=0 ; i < inFactors.size() ; i++) {
      const FactorDirection& direction = Input;
      const FactorType& factorType = inFactors[i];
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

  void GlobalLexicalModel::Evaluate
               (const PhraseBasedFeatureContext& context,
  							ScoreComponentCollection* accumulator) const
{
  accumulator->PlusEquals( this,  
      GetFromCacheOrScorePhrase(context.GetTargetPhrase()) );
}
}
