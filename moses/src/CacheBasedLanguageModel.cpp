

#include <utility>
#include "util/check.hh"
#include "StaticData.h"
#include "CacheBasedLanguageModel.h"

namespace Moses
{

CacheBasedLanguageModel::CacheBasedLanguageModel(const std::vector<float>& weights)
{
  const_cast<ScoreIndexManager&>  (StaticData::Instance().GetScoreIndexManager()).AddScoreProducer(this);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);

  query_type = ALLSUBSTRING;
}

void CacheBasedLanguageModel::SetQueryType(size_t type)
{
  query_type = type;
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
        if (query_type == WHOLESTRING)
        {
                Evaluate_Whole_String(tp,out);
        }
        else if (query_type == ALLSUBSTRING)
	{
                Evaluate_All_Substrings(tp,out);
	}
        else
        {      
        	CHECK(false);
        }
}


void CacheBasedLanguageModel::Evaluate_Whole_String(const TargetPhrase& tp, ScoreComponentCollection* out) const
{
//consider all words in the TargetPhrase as one n-gram
// and compute the decaying_score for all words
// and return their sum

        decaying_cache_t::const_iterator it;
        float score = 0.0;
        std::string w = "";
	size_t endpos = tp.GetSize();
        for (size_t pos = 0 ; pos < endpos ; ++pos) {
                w += tp.GetWord(pos).GetFactor(0)->GetString();
                if ((pos == 0) && (endpos > 1)){
			w += " ";
                }
	}
        it = m_cache.find(w);
        VERBOSE(1,"cblm::Evaluate: cheching cache for w:|" << w << "|" << std::endl);

        if (it != m_cache.end()) //found!
        {
       		score += ((*it).second).second;
		VERBOSE(1,"cblm::Evaluate: found w:|" << w << "| actual score:|" << ((*it).second).second << "| score:|" << score << "|" << std::endl);
        }

        out->PlusEquals(this, score);
}

void CacheBasedLanguageModel::Evaluate_All_Substrings(const TargetPhrase& tp, ScoreComponentCollection* out) const
{
//loop over all n-grams in the TargetPhrase (no matter of n)
// and compute the decaying_score for all words
// and return their sum

        decaying_cache_t::const_iterator it;
        float score = 0.0;
        for (size_t startpos = 0 ; startpos < tp.GetSize() ; ++startpos) {
                std::string w = "";
                for (size_t endpos = startpos; endpos < tp.GetSize() ; ++endpos) {
                        w += tp.GetWord(endpos).GetFactor(0)->GetString();
                        it = m_cache.find(w);

                        VERBOSE(1,"cblm::Evaluate: cheching cache for w:|" << w << "|" << std::endl);
                        if (it != m_cache.end()) //found!
                        {
                                score += ((*it).second).second;
				VERBOSE(1,"cblm::Evaluate: found w:|" << w << "| actual score:|" << ((*it).second).second << "| score:|" << score << "|" << std::endl);
                        }

                        if (endpos == startpos){
                                w += " ";
                        }

                }
        }
        out->PlusEquals(this, score);
}
/*
void CacheBasedLanguageModel::Evaluate_all(const TargetPhrase& tp, ScoreComponentCollection* out) const
{
//loop over all n-grams in the TargetPhrase (no matter of n)
// and compute the decaying_score for all words
// and return their sum

	decaying_cache_t::const_iterator it;
	float score = 0.0;
	for (size_t startpos = 0 ; startpos < tp.GetSize() ; ++startpos) {
		std::string w = "";
		for (size_t endpos = startpos; endpos < tp.GetSize() ; ++endpos) {
			w += tp.GetWord(endpos).GetFactor(0)->GetString();
			it = m_cache.find(w);

				VERBOSE(1,"cblm::Evaluate: cheching cache for w:|" << w << "|" << std::endl);
	                if (it != m_cache.end()) //found!
	                {
        	                score += ((*it).second).second;
				VERBOSE(1,"cblm::Evaluate: found w:|" << w << "| actual score:|" << ((*it).second).second << "| score:|" << score << "|" << std::endl);
                	}

                        if (endpos == startpos){
                                w += " ";
                        }

		}
	}
	out->PlusEquals(this, score);
}
*/

void CacheBasedLanguageModel::PrintCache()
{
        decaying_cache_t::iterator it;
	std::cout << "Content of the cache of Cache-Based Language Model" << std::endl;
	for ( it=m_cache.begin() ; it != m_cache.end(); it++ )
	{
		std::cout << "word:|" << (*it).first << "| age:|" << ((*it).second).first << "| score:|" << ((*it).second).second << "|" << std::endl;
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
		words[j] = Trim(words[j]);
                VERBOSE(3,"CacheBasedLanguageModel::Update   word[" << j << "]:"<< words[j] << " age:" << age << " decaying_score(age):" << decaying_score(age) << std::endl);
                decaying_cache_value_t p (age,decaying_score(age));
                std::pair<std::string, decaying_cache_value_t> e (words[j],p);
                m_cache.erase(words[j]); //always erase the element (do nothing if the entry does not exist)
                m_cache.insert(e); //insert the entry
        }
}

void CacheBasedLanguageModel::Insert(std::vector<std::string> words)
{
	Decay(); 
	Update(words,1);
	PrintCache();
	IFVERBOSE (2)
	{
//		PrintCache();
	}
}

void CacheBasedLanguageModel::Load(const std::string file)
{
//file format
//age || n-gram 
//age || n-gram || n-gram || n-gram || ...
//....
//each n-gram is a sequence of n words (no matter of n)
//
//there is no limit on the size of n
//
//entries can be repeated, but the last entry overwrites the previous

    InputFileStream cacheFile(file);

    std::string line;
    int age;
    std::vector<std::string> words;

    while (getline(cacheFile, line)) {
      std::vector<std::string> vecStr = TokenizeMultiCharSeparator( line , "||" );
      if (vecStr.size() >= 2) {
        age = Scan<int>(vecStr[0]);
        //vecStr.pop_back();
        vecStr.erase(vecStr.begin());
        std::vector<std::string> entries = vecStr;
        //std::vector<std::string> entries = TokenizeMultiCharSeparator( vecStr[1] , "||" );
        Update(entries,age);
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
