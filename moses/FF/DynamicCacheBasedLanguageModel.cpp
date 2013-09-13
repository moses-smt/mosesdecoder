#include <utility>
#include "util/check.hh"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "DynamicCacheBasedLanguageModel.h"

namespace Moses
{
	
	DynamicCacheBasedLanguageModel::DynamicCacheBasedLanguageModel(const std::string &line)
	: StatelessFeatureFunction("DynamicCacheBasedLanguageModel", line)
	{
		query_type = ALLSUBSTRINGS;

		ReadParameters();
	}
	
	void DynamicCacheBasedLanguageModel::SetParameter(const std::string& key, const std::string& value)
	{
		if (key == "cblm-query-type") {
			query_type = Scan<size_t>(value);
		}
		if (key == "cblm-score-type") {
			score_type = Scan<size_t>(value);
		}
		else if (key == "cblm-file") {
			m_initfiles = Scan<std::string>(value);
		} else {
			StatelessFeatureFunction::SetParameter(key, value);
		}
	}

	void DynamicCacheBasedLanguageModel::Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const
	{
		if (query_type == WHOLESTRING)
		{
			Evaluate_Whole_String(tp,out);
		}
		else if (query_type == ALLSUBSTRINGS)
		{
			Evaluate_All_Substrings(tp,out);
		}
		else
		{      
			CHECK(false);
		}
	}
	
	void DynamicCacheBasedLanguageModel::Evaluate(const PhraseBasedFeatureContext& context, ScoreComponentCollection* accumulator) const
	{
		const TargetPhrase& tp = context.GetTargetPhrase();
		Evaluate(tp, accumulator);
	}
	
	void DynamicCacheBasedLanguageModel::EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const
	{
		const TargetPhrase& tp = context.GetTargetPhrase();
		Evaluate(tp, accumulator);
	}
	
	void DynamicCacheBasedLanguageModel::Evaluate_Whole_String(const TargetPhrase& tp, ScoreComponentCollection* out) const
	{
		//consider all words in the TargetPhrase as one n-gram
		// and compute the decaying_score for all words
		// and return their sum
		
		decaying_cache_t::const_iterator it;
		float score = 0.0;

		std::string w = "";
		size_t endpos = tp.GetSize();
		for (size_t pos = 0 ; pos < endpos ; ++pos) {
			w += tp.GetWord(pos).GetFactor(0)->GetString().as_string();
			if ((pos == 0) && (endpos > 1)){
				w += " ";
			}
		}
		it = m_cache.find(w);
//		VERBOSE(1,"cblm::Evaluate: cheching cache for w:|" << w << "|" << std::endl);
		
		if (it != m_cache.end()) //found!
		{
			score += ((*it).second).second;
			VERBOSE(3,"cblm::Evaluate: found w:|" << w << "| actual score:|" << ((*it).second).second << "| score:|" << score << "|" << std::endl);
		}
		
		out->PlusEquals(this, score);
	}
	
	void DynamicCacheBasedLanguageModel::Evaluate_All_Substrings(const TargetPhrase& tp, ScoreComponentCollection* out) const
	{
		//loop over all n-grams in the TargetPhrase (no matter of n)
		// and compute the decaying_score for all words
		// and return their sum
		
		decaying_cache_t::const_iterator it;
		float score = 0.0;
		for (size_t startpos = 0 ; startpos < tp.GetSize() ; ++startpos) {
			std::string w = "";
			for (size_t endpos = startpos; endpos < tp.GetSize() ; ++endpos) {
				w += tp.GetWord(endpos).GetFactor(0)->GetString().as_string();
				//w += tp.GetWord(endpos).GetFactor(0)->GetString();
				it = m_cache.find(w);
				
//				VERBOSE(1,"cblm::Evaluate: cheching cache for w:|" << w << "|" << std::endl);
				if (it != m_cache.end()) //found!
				{
					score += ((*it).second).second;
					VERBOSE(3,"cblm::Evaluate: found w:|" << w << "| actual score:|" << ((*it).second).second << "| score:|" << score << "|" << std::endl);
				}
				
				if (endpos == startpos){
					w += " ";
				}
				
			}
		}
		out->PlusEquals(this, score);
	}
	
	void DynamicCacheBasedLanguageModel::Print() const
	{
		decaying_cache_t::const_iterator it;
		std::cout << "Content of the cache of Cache-Based Language Model" << std::endl;
		for ( it=m_cache.begin() ; it != m_cache.end(); it++ )
		{
			std::cout << "word:|" << (*it).first << "| age:|" << ((*it).second).first << "| score:|" << ((*it).second).second << "|" << std::endl;
		}
	}
	
	void DynamicCacheBasedLanguageModel::Decay()
	{
		decaying_cache_t::iterator it;
		
		int age;
		float score;
		for ( it=m_cache.begin() ; it != m_cache.end(); it++ )
		{
			age=((*it).second).first + 1;
			if (age > 1000)
			{
				m_cache.erase(it);
				it--;
			}
			else
			{
				score = decaying_score(age);
				decaying_cache_value_t p (age, score);
				(*it).second = p;
			}
		}
	}
	
	void DynamicCacheBasedLanguageModel::Update(std::vector<std::string> words, int age)
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

        void DynamicCacheBasedLanguageModel::Insert(std::string &entries)
        {
            if (entries != "")
            {
              VERBOSE(1,"entries:|" << entries << "|" << std::endl);
              std::vector<std::string> elements = TokenizeMultiCharSeparator(entries, "||");
              VERBOSE(1,"elements.size() after:|" << elements.size() << "|" << std::endl);
              Insert(elements);
            }
        }
	
	void DynamicCacheBasedLanguageModel::Insert(std::vector<std::string> ngrams)
	{
		VERBOSE(1,"CacheBasedLanguageModel Insert ngrams.size():|" << ngrams.size() << "|" << std::endl);
		Decay();
		Update(ngrams,1);
		IFVERBOSE(2) Print();
	}
	
	void DynamicCacheBasedLanguageModel::Execute(std::vector<std::string> commands)
	{
		for (size_t j=0; j<commands.size(); j++)
		{
			Execute(commands[j]);
		}
		IFVERBOSE(2) Print();
	}
	
	void DynamicCacheBasedLanguageModel::Execute(std::string command)
	{
		if (command == "clear")
		{
			VERBOSE(1,"CacheBasedLanguageModel Execute command:|"<< command << "|. Cache cleared." << std::endl);
			m_cache.clear();
		}
		else if (command == "settype_wholestring")
		{
			VERBOSE(1,"CacheBasedLanguageModel Execute command:|"<< command << "|. Query type set to " << WHOLESTRING << " (WHOLESTRING)." << std::endl);
			SetQueryType(WHOLESTRING);
		}
		else if (command == "settype_allsubstrings")
		{
			VERBOSE(1,"CacheBasedLanguageModel Execute command:|"<< command << "|. Query type set to " << ALLSUBSTRINGS << " (ALLSUBSTRINGS)." << std::endl);
			SetQueryType(ALLSUBSTRINGS);
		}
		else
		{
			VERBOSE(1,"CacheBasedLanguageModel Execute command:|"<< command << "| is unknown. Skipped." << std::endl);
		}        
	}
	
	void DynamicCacheBasedLanguageModel::Load(std::vector<std::string> files)
	{
		for(size_t j = 0; j < files.size(); ++j)
		{
			Load(files[j]);
		}
	}
	
	void DynamicCacheBasedLanguageModel::Load(const std::string file)
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
		
		
		VERBOSE(2,"Loading data from the cache file " << file << std::endl);
		InputFileStream cacheFile(file);
		
		std::string line;
		int age;
		std::vector<std::string> words;
		
		while (getline(cacheFile, line)) {
			std::vector<std::string> vecStr = TokenizeMultiCharSeparator( line , "||" );
			if (vecStr.size() >= 2) {
				age = Scan<int>(vecStr[0]);
				vecStr.erase(vecStr.begin());
				Update(vecStr,age);
			} else {
				TRACE_ERR("ERROR: The format of the loaded file is wrong: " << line << std::endl);
				CHECK(false);
			}
		}
		IFVERBOSE(2) Print();
	}
	
	float DynamicCacheBasedLanguageModel::decaying_score(const int age)
	{
		return (float) exp((float) 1/age);
//		return (float) 1/age;
	}
}
