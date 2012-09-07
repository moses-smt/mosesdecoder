// $Id$

#include <utility>
#include "util/check.hh"
#include "StaticData.h"
#include "CacheBasedLanguageModel.h"
//#include "WordsRange.h"
//#include "TranslationOption.h"

namespace Moses
{

CacheBasedLanguageModel::CacheBasedLanguageModel(const std::vector<float>& weights)
{
  const_cast<ScoreIndexManager&>  (StaticData::Instance().GetScoreIndexManager()).AddScoreProducer(this);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);

}

size_t CacheBasedLanguageModel::GetNumScoreComponents() const
{
  return 1;
}

std::string CacheBasedLanguageModel::GetScoreProducerDescription(unsigned) const
{
  return "CacheBasedLanguageModel";
}

std::string CacheBasedLanguageModel::GetScoreProducerWeightShortName(unsigned) const
{
  return "cblm";
}

void CacheBasedLanguageModel::Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const
{
//loop over all words in the TargetPhrase
// and compute the decaying_score for all words
// and return their sum

	decaying_cache_t::const_iterator it;
	float score = 0.0;
	for (size_t pos = 0 ; pos < tp.GetSize() ; ++pos) {
		std::string w = tp.GetWord(pos).GetFactor(0)->GetString();
		it = m_cache.find(w);

		if (it != m_cache.end()) //found!
		{
			score += ((*it).second).second;
		}
	}

	out->PlusEquals(this, score);
}


void CacheBasedLanguageModel::PrintCache()
{
        decaying_cache_t::iterator it;
	for ( it=m_cache.begin() ; it != m_cache.end(); it++ )
	{
		VERBOSE(1,"word:" << (*it).first << " age:" << ((*it).second).first << " score:" << ((*it).second).second << std::endl);
	}
}

void CacheBasedLanguageModel::Decay()
{
	decaying_cache_t::iterator it;

	for ( it=m_cache.begin() ; it != m_cache.end(); it++ )
	{
		int age=((*it).second).first + 1;
		float score = decaying_score(age);
		if (age < 1000){
			decaying_cache_value_t p (age, score);
	    		(*it).second = p;
		}
		else{
			m_cache.erase(it);
		}
	}
}

void CacheBasedLanguageModel::Update(std::vector<std::string> words, int age)
{
	for (size_t j=0; j<words.size(); j++)
	{
		decaying_cache_value_t p (age,decaying_score(age));
		std::pair<std::string, decaying_cache_value_t> e (words[j],p);
		m_cache.insert(e);
	}
}

void CacheBasedLanguageModel::Insert(std::vector<std::string> words)
{
	Decay();
	Update(words,1);
	PrintCache();
}

void CacheBasedLanguageModel::Load(const std::string file)
{
//file format
//age ||| word sequence
//age ||| word sequence
//....

    InputFileStream cacheFile(file);

    std::string line;
    int age;
    std::vector<std::string> words;

    while (getline(cacheFile, line)) {
      std::vector<std::string> vecStr = TokenizeMultiCharSeparator( line , "|||" );

      if (vecStr.size() == 2) {
        age = Scan<int>(vecStr[0]);
	words = Tokenize(vecStr[1]);

	Update(words,age);
      } else {
        CHECK(false);
      }
    }
}

float CacheBasedLanguageModel::decaying_score(const int age)
{
	return (float) exp((float) 1/age);
}

}
