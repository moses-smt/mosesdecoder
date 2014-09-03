#include <vector>
#include "moses/StaticData.h"
#include "BloomFilter.hpp"
#include "CheckTargetNgrams.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"

using namespace std;

namespace Moses
{
int CheckTargetNgramsState::Compare(const FFState& other) const
{
  const CheckTargetNgramsState &otherState = static_cast<const CheckTargetNgramsState&>(other);

  if (m_history == otherState.m_history)
    return 0;
  return 1;
}

////////////////////////////////////////////////////////////////
CheckTargetNgrams::CheckTargetNgrams(const std::string &line)
  :StatefulFeatureFunction(line)
{
  VERBOSE(1, "Initializing feature " << GetScoreProducerDescription() << " ...");
  ReadParameters();
  m_minorder=m_maxorder-m_numScoreComponents+1;
  UTIL_THROW_IF2(m_minorder<=0, "num-features is too high for max-order");  

  std::ostringstream tmp_bos;
  tmp_bos << BOS_;
  m_bos = tmp_bos.str();
}

void CheckTargetNgrams::EvaluateInIsolation(const Phrase &source
                                  , const TargetPhrase &targetPhrase
                                  , ScoreComponentCollection &scoreBreakdown
                                  , ScoreComponentCollection &estimatedFutureScore) const
{}

void CheckTargetNgrams::EvaluateWithSourceContext(const InputType &input
                                  , const InputPath &inputPath
                                  , const TargetPhrase &targetPhrase
                                  , const StackVec *stackVec
                                  , ScoreComponentCollection &scoreBreakdown
                                  , ScoreComponentCollection *estimatedFutureScore) const
{}

FFState* CheckTargetNgrams::EvaluateWhenApplied(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  VERBOSE(4, GetScoreProducerDescription() << " EvaluateWhenApplied" << std::endl);

  const size_t currStartPos = cur_hypo.GetCurrTargetWordsRange().GetStartPos();
  const size_t m_words_number = cur_hypo.GetCurrTargetLength() + m_maxorder - 1;

  VERBOSE(5, GetScoreProducerDescription() << " currStartPos:|" << currStartPos << "| cur_hypo.GetCurrTargetLength():|" << cur_hypo.GetCurrTargetLength()<< "| m_words_number|" << m_words_number << "| m_maxorder:|" << m_maxorder << "|" << std::endl);

  // 1st n-gram
  vector<std::string> m_words(m_words_number);
  size_t word_index;

  int currPos=currStartPos - m_maxorder + 1;
  for (word_index = 0; word_index < m_words_number ; ++word_index, ++currPos) {
    if (currPos >= 0)
      m_words[word_index] = cur_hypo.GetWord(currPos).GetString(m_factorType).as_string();
    else {
      m_words[word_index] = m_bos;
    }
  }

  vector<float> newScores(m_numScoreComponents);
  vector<bool> matches(m_numScoreComponents);
  size_t score_index;

  std::string ngr;

  size_t offset, min_offset;
  for (offset=m_maxorder - 1; offset < m_words_number; ++offset){
    min_offset=offset - (m_maxorder -  m_numScoreComponents) + 1;
    word_index=offset;
    //join words of contextFactor into a string
    ngr = m_words[word_index];
    VERBOSE(5, GetScoreProducerDescription() << " word_index:|" << word_index << "| min_offset:|" << min_offset << " not computing score for:|" << ngr << "|" << std::endl);
    while (word_index > min_offset){
       --word_index;
       ngr = m_words[word_index] + " " + ngr;
       VERBOSE(6, GetScoreProducerDescription() << " word_index:|" << word_index << "| min_offset:|" << min_offset << "| not computing score for:|" << ngr << "|" << std::endl);
    }

    VERBOSE(5, GetScoreProducerDescription() << " separation point reached  word_index:|" << word_index << "|" << std::endl);

    min_offset=offset - m_maxorder + 1;
    for (score_index = 0; score_index < m_numScoreComponents - 1; ++score_index){
      matches[score_index] = false;
    }
    score_index=m_numScoreComponents - 1;
    while (word_index > min_offset){
      --word_index;
      ngr = m_words[word_index] + " " + ngr;
      VERBOSE(6, GetScoreProducerDescription() << " checking score_index:|" << score_index << "| word_index:|" << word_index << "| min_offset:|" << min_offset << "| computing score for:|" << ngr << "| score:|" << newScores[score_index] << "|" << std::endl);

      //compute score for string with (m_maxorder -  m_numScoreComponents) words
      matches[score_index] = m_bloomfilter.contains(ngr);
      VERBOSE(6, GetScoreProducerDescription() << " score_index:|" << score_index << "| match:|" << matches[score_index] << "|" << std::endl);

      //check consistency
      if (matches[score_index] == true){
        if ((score_index < m_numScoreComponents - 1) && (matches[score_index+1] == false)){
//          VERBOSE(6, "Data are not consistent: n-gram:|" << ngr << "| is present, but its shorter version is not");
          UTIL_THROW_IF2(1, "Data are not consistent: n-gram:|" << ngr << "| is present, but its shorter version is not");
        }

        newScores[score_index] += 1.0;
        VERBOSE(6, GetScoreProducerDescription() << " score_index:|" << score_index << "| word_index:|" << word_index << "| min_offset:|" << min_offset << "| computing score for:|" << ngr << "| score:|" << newScores[score_index] << "|" << std::endl);
        --score_index;
      }
      else{ //exit the loop because longer n-grams do not match as well due to consistency requirement 
        break;
      }
    }
  }

  score_index = 1;
  VERBOSE(5, GetScoreProducerDescription() << " looking for the longest match, score_index:|" << score_index << "|" << std::endl);
  while ( score_index < m_numScoreComponents && matches[score_index] == false){
    ++score_index;
  }
  VERBOSE(5, GetScoreProducerDescription() << " final matching score index:|" << score_index << "|" << std::endl);
  //score_index=0 is not considered
  //if score_index=1, then state is the hash of an n-gram of order (m_maxorder - 1)
  //if score_index=2, then state is the hash of an n-gram of order (m_maxorder - 2)
  //etc... up to (m_numScoreComponents-1)

  word_index = m_words_number - 1;
  min_offset=word_index - ( m_maxorder - score_index ) + 1;
  VERBOSE(5, GetScoreProducerDescription() << " creating string for the state from word_index:|" << word_index << "| up to  min_offset:|" << min_offset << "|" << std::endl);
  ngr = m_words[word_index];
  while (word_index > min_offset){
    --word_index;
    ngr = m_words[word_index] + " " + ngr;
  }
  accumulator->PlusEquals(this, newScores);

  unsigned int history = m_bloomfilter.simple_hash_key(ngr);

  VERBOSE(5, GetScoreProducerDescription() << " final string to hash, number of words:|" << (m_maxorder - score_index) << "| ngr:|" << ngr << "| state:|" << history << "|" << std::endl);
 
  return new CheckTargetNgramsState(history); 
}

FFState* CheckTargetNgrams::EvaluateWhenApplied(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  //this feature can be computed only when the LM is computed
  return new CheckTargetNgramsState(0);
}

void CheckTargetNgrams::SetParameter(const std::string& key, const std::string& value)
{
 if (key == "path") {
    m_filePath = Scan<std::string>(value);
  } else if (key == "factor") {
    //This feature refers to one factor only specified by parameter "factor"
    m_factorType = Scan<FactorType>(value);
  } else if (key == "max-order") {
    m_maxorder = Scan<unsigned int>(value);
   //m_numScoreComponents is read and set in the super-class
   //if >1 then it will check n-grams from max-order to max-order - numScoreComponents +1
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}


void CheckTargetNgrams::Load()
{
  UTIL_THROW_IF2(m_filePath == "", "Filename must be specified");

  ifstream inFile(m_filePath.c_str());
  UTIL_THROW_IF2(!inFile, "Can't open file " << m_filePath);
    
  //load bloom filter file
  m_bloomfilter.load(inFile);
  inFile.close();
}

}

