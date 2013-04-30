#include <utility>
#include "util/check.hh"
#include "StaticData.h"
#include "CacheBasedLanguageModel.h"

namespace Moses
{
//	CacheBasedLanguageModel::CacheBasedLanguageModel(const std::vector<std::string>& files, const size_t q_type, const size_t s_type):
	CacheBasedLanguageModel::CacheBasedLanguageModel(const std::vector<std::string>& files, const size_t q_type, const size_t s_type, const unsigned int age):
		StatelessFeatureFunction("CacheBasedLanguageModel",1){

		SetQueryType(q_type);	
		SetScoreType(s_type);	
                SetMaxAge(age);
                SetPreComputedScores();
		Load(files);
	}
	
        void CacheBasedLanguageModel::SetPreComputedScores()
        {
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif          
                precomputedScores.clear();
                for (size_t i=0; i<maxAge; i++)
                {
                        precomputedScores.push_back(decaying_score(i));
                }

		if ( score_type == CBLM_SCORE_TYPE_HYPERBOLA
			|| score_type == CBLM_SCORE_TYPE_POWER
			|| score_type == CBLM_SCORE_TYPE_EXPONENTIAL
			|| score_type == CBLM_SCORE_TYPE_COSINE )
		{
                        precomputedScores.push_back(decaying_score(maxAge));
		}else{  // score_type = CBLM_SCORE_TYPE_XXXXXXXXX_REWARD
                        precomputedScores.push_back(0.0);
		}
        }

	void CacheBasedLanguageModel::Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const
	{
		switch(query_type){
		case CBLM_QUERY_TYPE_WHOLESTRING:
			Evaluate_Whole_String(tp,out);
			break;
		case CBLM_QUERY_TYPE_ALLSUBSTRINGS:
			Evaluate_All_Substrings(tp,out);
			break;
		default:
			CHECK(false);
		}
	}
	
	void CacheBasedLanguageModel::Evaluate(const PhraseBasedFeatureContext& context, ScoreComponentCollection* accumulator) const
	{
		const TargetPhrase& tp = context.GetTargetPhrase();
		Evaluate(tp, accumulator);
	}
	
	void CacheBasedLanguageModel::EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const
	{
		const TargetPhrase& tp = context.GetTargetPhrase();
		Evaluate(tp, accumulator);
	}
	
	void CacheBasedLanguageModel::Evaluate_Whole_String(const TargetPhrase& tp, ScoreComponentCollection* out) const
	{
		//VERBOSE(1,"CacheBasedLanguageModel::Evaluate_Whole_String" << std::endl);
		//consider all words in the TargetPhrase as one n-gram
		// and compute the decaying_score for all words
		// and return their sum
		
		decaying_cache_t::const_iterator it;
		float score = 0.0;
		std::string w = "";
		size_t endpos = tp.GetSize();
		for (size_t pos = 0 ; pos < endpos ; ++pos) {
			if (pos > 0){ w += " "; }
			w += tp.GetWord(pos).GetFactor(0)->GetString();
		}
		it = m_cache.find(w);
		
		if (it != m_cache.end()) //found!
		{
			score = ((*it).second).second;
			VERBOSE(3,"cblm::Evaluate: found w:|" << w << "| score:|" << score << "|" << std::endl);
		}
		else{
			score = precomputedScores[maxAge]; // one score per phrase table 
			VERBOSE(3,"cblm::Evaluate: not found w:|" << w << "| score:|" << score << "|" << std::endl);
		}
		VERBOSE(3,"cblm::Evaluate: phrase:|" << tp << "| score:|" << score << "|" << std::endl);
		
		out->PlusEquals(this, score);
	}
	
	void CacheBasedLanguageModel::Evaluate_All_Substrings(const TargetPhrase& tp, ScoreComponentCollection* out) const
	{
		//VERBOSE(1,"CacheBasedLanguageModel::Evaluate_All_Substrings" << std::endl);
		//loop over all n-grams in the TargetPhrase (no matter of n)
		// and compute the decaying_score for all words
		// and return their sum
		
		decaying_cache_t::const_iterator it;
		float score = 0.0;
		size_t tp_size = tp.GetSize();
		for (size_t startpos = 0 ; startpos < tp_size ; ++startpos) {
			std::string w = "";
			for (size_t endpos = startpos; endpos < tp_size ; ++endpos) {
                                if (endpos > startpos){ w += " "; }
				w += tp.GetWord(endpos).GetFactor(0)->GetString();
				it = m_cache.find(w);
				
				float tmpsc;
				if (it != m_cache.end()) //found!
				{
					tmpsc = ((*it).second).second;
					VERBOSE(3,"cblm::Evaluate: found w:|" << w << "| score:|" << tmpsc << "|" << std::endl);
				}
		                else{
        		                tmpsc = precomputedScores[maxAge]; // one score per phrase table  
					VERBOSE(3,"cblm::Evaluate: not found w:|" << w << "| score:|" << tmpsc << "|" << std::endl);
                		}
				score += ( tmpsc /  ( tp_size + startpos - endpos ) );	
				VERBOSE(3,"cblm::Evaluate: actual score:|" << score << "|" << std::endl);
			}
		}
		VERBOSE(3,"cblm::Evaluate: phrase:|" << tp << "| score:|" << score << "|" << std::endl);
		out->PlusEquals(this, score);
	}
	
	void CacheBasedLanguageModel::Print() const
	{
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif          
		decaying_cache_t::const_iterator it;
		std::cout << "Content of the cache of Cache-Based Language Model" << std::endl;
		for ( it=m_cache.begin() ; it != m_cache.end(); it++ )
		{
			std::cout << "word:|" << (*it).first << "| age:|" << ((*it).second).first << "| score:|" << ((*it).second).second << "|" << std::endl;
		}
	}
	
	void CacheBasedLanguageModel::Decay()
	{
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif          
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
	
	void CacheBasedLanguageModel::Update(std::vector<std::string> words, int age)
	{
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif          
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
	
	void CacheBasedLanguageModel::Insert(std::vector<std::string> ngrams)
	{
		Decay();
		Update(ngrams,1);
		IFVERBOSE(2) Print();
	}
	
	void CacheBasedLanguageModel::Execute(std::vector<std::string> commands)
	{
		for (size_t j=0; j<commands.size(); j++)
		{
			Execute(commands[j]);
		}
		IFVERBOSE(2) Print();
	}
	
	void CacheBasedLanguageModel::Execute(std::string command)
	{
		if (command == "clear")
		{
			VERBOSE(1,"CacheBasedLanguageModel Execute command:|"<< command << "|. Cache cleared." << std::endl);
			Clear();
		}
		else if (command == "settype_wholestring")
		{
			VERBOSE(1,"CacheBasedLanguageModel Execute command:|"<< command << "|." << std::endl);
			SetQueryType(CBLM_QUERY_TYPE_WHOLESTRING);
		}
		else if (command == "settype_allsubstrings")
		{
			VERBOSE(1,"CacheBasedLanguageModel Execute command:|"<< command << "|." << std::endl);
			SetQueryType(CBLM_QUERY_TYPE_ALLSUBSTRINGS);
		}
		else
		{
			VERBOSE(1,"CacheBasedLanguageModel Execute command:|"<< command << "| is unknown. Skipped." << std::endl);
		}        
	}
	
  void CacheBasedLanguageModel::Load(std::vector<std::string> files)
	{
		for(size_t j = 0; j < files.size(); ++j)
		{
			Load(files[j]);
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
	
        void CacheBasedLanguageModel::SetQueryType(size_t type) {
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif

		query_type = type;
                if ( query_type != CBLM_QUERY_TYPE_WHOLESTRING
                        && query_type != CBLM_QUERY_TYPE_ALLSUBSTRINGS )
                        {
                               VERBOSE(2, "This query type " << query_type << " is unknown. Instead used " << CBLM_QUERY_TYPE_ALLSUBSTRINGS << "." << std::endl);
                               query_type = CBLM_QUERY_TYPE_ALLSUBSTRINGS;
                }
                VERBOSE(2, "CacheBasedLanguageModel QueryType:  " << query_type << std::endl);

	};

        void CacheBasedLanguageModel::SetScoreType(size_t type) {
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
                score_type = type;
                if ( score_type != CBLM_SCORE_TYPE_HYPERBOLA
                        && score_type != CBLM_SCORE_TYPE_POWER
                        && score_type != CBLM_SCORE_TYPE_EXPONENTIAL
                        && score_type != CBLM_SCORE_TYPE_COSINE
                        && score_type != CBLM_SCORE_TYPE_HYPERBOLA_REWARD
                        && score_type != CBLM_SCORE_TYPE_POWER_REWARD
                        && score_type != CBLM_SCORE_TYPE_EXPONENTIAL_REWARD )
                        {
                               VERBOSE(2, "This score type " << score_type << " is unknown. Instead used " << CBLM_SCORE_TYPE_HYPERBOLA << "." << std::endl);
                 	       score_type = CBLM_SCORE_TYPE_HYPERBOLA;
                }
		VERBOSE(2, "CacheBasedLanguageModel ScoreType:  " << score_type << std::endl);
        };

        void CacheBasedLanguageModel::SetMaxAge(unsigned int age) {
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
                maxAge = age;
                VERBOSE(2, "CacheBasedLanguageModel MaxAge:  " << maxAge << std::endl);
        };



        void CacheBasedLanguageModel::Clear() {
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
                m_cache.clear();
        };

	float CacheBasedLanguageModel::decaying_score(const int age)
	{
		float sc;
                switch(score_type){
                case CBLM_SCORE_TYPE_HYPERBOLA:
                        sc = (float) 1.0/age - 1.0;
                        break;
                case CBLM_SCORE_TYPE_POWER:
                        sc = (float) pow(age, -0.25) - 1.0;
                        break;
                case CBLM_SCORE_TYPE_EXPONENTIAL:
                        sc = (age == 1) ? 0.0 : (float) exp( 1.0/age ) / exp(1.0) - 1.0;
                        break;
                case CBLM_SCORE_TYPE_COSINE:
                        sc = (float) cos( (age-1) * (PI/2) / maxAge ) - 1.0;
                        break;
                case CBLM_SCORE_TYPE_HYPERBOLA_REWARD:
                        sc = (float) 1.0/age;
                        break;
                case CBLM_SCORE_TYPE_POWER_REWARD:
                        sc = (float) pow(age, -0.25);
                        break;
                case CBLM_SCORE_TYPE_EXPONENTIAL_REWARD:
                        sc = (age == 1) ? 1.0 : (float) exp( 1.0/age ) / exp(1.0);
                        break;
                default:
                        sc = -1.0;
                }
		return sc;
	
	}
}
