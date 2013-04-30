/*
 * PhraseDictionaryCache.cpp
 *
 *  Created on: Dec 5, 2012
 *      Author: prashant
 */
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <memory>
#include <sys/stat.h>
#include <stdlib.h>
#include <utility>

#include "util/file_piece.hh"
#include "util/tokenize_piece.hh"
#include "util/check.hh"

#include "moses/FactorCollection.h"
#include "moses/Word.h"
#include "moses/Util.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "moses/WordsRange.h"
#include "moses/UserMessage.h"
#include "moses/SparsePhraseDictionaryFeature.h"
#include "moses/TranslationModel/PhraseDictionaryCache.h"

namespace Moses {
	
	namespace {
		void ParserDeath(const std::string &file, size_t line_num) {
			std::stringstream strme;
			strme << "Syntax error at " << file << ":" << line_num;
			UserMessage::Add(strme.str());
			abort();
		}
		template <class It> StringPiece GrabOrDie(It &it, const std::string &file, size_t line_num) {
			if (!it) ParserDeath(file, line_num);
			return *it++;
		}
	} // namespace
	
//	PhraseDictionaryCache::PhraseDictionaryCache(size_t numScoreComponent, const std::string filePath, PhraseDictionaryFeature* feature): PhraseDictionary(numScoreComponent,feature)	{
	PhraseDictionaryCache::PhraseDictionaryCache(size_t numScoreComponent, const std::string filePath, PhraseDictionaryFeature* feature, size_t s_type, unsigned int age): PhraseDictionary(numScoreComponent,feature)	{

                SetScoreType(s_type);

		SetMaxAge(age);
                SetPreComputedScores(numScoreComponent);
                VERBOSE(2, "PhraseDictionaryCache scorecomponent:" << numScoreComponent << std::endl);

		if (filePath != "_NULL_")
		{
			LoadCacheFile(filePath);
		}
		
		
	}
	
	
	PhraseDictionaryCache::~PhraseDictionaryCache()
	{
		Clear();
	}
	
	void PhraseDictionaryCache::LoadCacheFile(const std::string &filePath)
	{
		VERBOSE(1,"PhraseDictionaryCache loading initial cache entries from " << filePath << std::endl);
		const StaticData &staticData = StaticData::Instance();
		
		util::FilePiece inFile(filePath.c_str(), staticData.GetVerboseLevel() >= 1 ? &std::cerr : NULL);
		
		size_t line_num = 0;
		const std::string& factorDelimiter = staticData.GetFactorDelimiter();
		
		while(true) {
			++line_num;
			StringPiece line;
			try {
				line = inFile.ReadLine();
			} catch (util::EndOfFileException &e) {
				break;
			}
			
			util::TokenIter<util::MultiCharacter> pipes(line, util::MultiCharacter("|||"));
			StringPiece ageString(GrabOrDie(pipes, filePath, line_num));
			StringPiece sourcePhraseString(GrabOrDie(pipes, filePath, line_num));
			StringPiece targetPhraseString(GrabOrDie(pipes, filePath, line_num));

			Update(sourcePhraseString.as_string(), targetPhraseString.as_string(), ageString.as_string());
		}
		return;
	}
	
	
	bool PhraseDictionaryCache::Load(const std::vector<FactorType> &input
																	 , const std::vector<FactorType> &output)
	{
		return true;
	}
	
	
	void PhraseDictionaryCache::Update(std::string sourcePhraseString, std::string targetPhraseString, std::string ageString)
	{
		const StaticData &staticData = StaticData::Instance();
		const std::string& factorDelimiter = staticData.GetFactorDelimiter();
		
		Phrase sourcePhrase(0);
		Phrase targetPhrase(0);
		
		char *err_ind_temp;
		int age = strtod(ageString.c_str(), &err_ind_temp);
		
		//target
		targetPhrase.Clear();
		targetPhrase.CreateFromString(staticData.GetOutputFactorOrder(), targetPhraseString, factorDelimiter);
		
		//TODO: Would be better to reuse source phrases, but ownership has to be
		//consistent across phrase table implementations
		sourcePhrase.Clear();
		sourcePhrase.CreateFromString(staticData.GetInputFactorOrder(), sourcePhraseString, factorDelimiter);
		Update(sourcePhrase, targetPhrase, age);
	}

        void PhraseDictionaryCache::Execute(std::vector<std::string> commands)
        {
                for (size_t j=0; j<commands.size(); j++)
                {
                        Execute(commands[j]);
                }
                IFVERBOSE(2) Print();
        }

        void PhraseDictionaryCache::Execute(std::string command)
        {
                if (command == "clear")
                {
                        VERBOSE(1,"PhraseDictionaryCache Execute command:|"<< command << "|. Cache cleared." << std::endl);
                        Clear();
                }
                else
                {
                        VERBOSE(1,"CacheBasedLanguageModel Execute command:|"<< command << "| is unknown. Skipped." << std::endl);
                }
        }


	std::string PhraseDictionaryCache::GetScoreProducerDescription(unsigned) const
	{
		return "PhraseDictionaryCache";
	}
	
	std::string PhraseDictionaryCache::GetScoreProducerWeightShortName(unsigned) const
	{
		return "cbtm";
	}
	size_t PhraseDictionaryCache::GetNumScoreComponents() const
	{
		return m_numScoreComponent;
	}
	/*
	 * gets the collection given the target phrase
	 */
	const TargetPhraseCollection *PhraseDictionaryCache::GetTargetPhraseCollection(const Phrase &source) const
	{
#ifdef WITH_THREADS
		boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
		const TargetPhraseCollection* tpc = NULL;
		std::map<Phrase, TargetCollectionAgePair>::const_iterator it = m_cacheTM.find(source);
		if(it != m_cacheTM.end())
		{
			tpc = (it->second).first;
		}
		return tpc;
	}
	
	void PhraseDictionaryCache::Print() const
	{
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif          
		std::map<Phrase, TargetCollectionAgePair>::const_iterator it;
		for(it = m_cacheTM.begin(); it!=m_cacheTM.end(); it++)
		{
			std::vector<size_t>::size_type sz;
			std::string source = (it->first).ToString();
			TargetPhraseCollection* tpc = (it->second).first;
			std::vector<TargetPhrase*> TPCvector;// = tpc->GetCollection();
			sz = TPCvector.capacity();
			TPCvector.reserve(tpc->GetSize());
			TPCvector = tpc->GetCollection();
			std::vector<TargetPhrase*>::iterator itr;
			for(itr = TPCvector.begin(); itr != TPCvector.end(); itr++)
			{
				std::string target = (*itr)->ToString();
				VERBOSE(1, source << " ||| " << target << std::endl);
			}
			TPCvector.clear();
			source.clear();
		}
	}
	
	/*
	 * Updates the cache table with new entry
	 */
	void PhraseDictionaryCache::Update(Phrase sp, Phrase tp, int age)
	{
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif          
		VERBOSE(2, "PhraseDictionaryCache inserting sp:" << sp << " tp:" << tp << " age:" << age << std::endl);
		
		std::map<Phrase, TargetCollectionAgePair>::const_iterator it = m_cacheTM.find(sp);
		if(it!=m_cacheTM.end())
		{
			// p is found
			// here we have to remove the target phrase from targetphrasecollection and from the TargetAgeMap
			// and then add new entry
			TargetCollectionAgePair TgtCollAgePair = it->second;
			TargetPhraseCollection* tpc = TgtCollAgePair.first;
			TargetAgeMap* tam = TgtCollAgePair.second;
			TargetAgeMap::iterator tam_it = tam->find(tp);
			if (tam_it!=tam->end())
			{
				//tp is found
				size_t tp_pos = ((*tam_it).second).second;
				((*tam_it).second).first = age;
				TargetPhrase* tp_ptr = tpc->GetTargetPhrase(tp_pos);
				tp_ptr->SetScore(m_feature,precomputedScores[age]);
			}
			else
			{
				//tp is not found
		    std::auto_ptr<TargetPhrase> targetPhrase(new TargetPhrase(tp));
		    //Now that the source phrase is ready, we give the target phrase a copy
		    targetPhrase->SetSourcePhrase(sp);
		    targetPhrase->SetScore(m_feature,precomputedScores[age]);
		    tpc->Add(targetPhrase.release());
		    size_t tp_pos = tpc->GetSize()-1;
		    AgePosPair app(age,tp_pos);
		    TargetAgePosPair taap(tp,app);
		    tam->insert(taap);
			}
		}
		else
		{
			// p is not found
			// create target collection
			// we have to create new target collection age pair and add new entry to target collection age pair
			
			TargetPhraseCollection* tpc = new TargetPhraseCollection();
			TargetAgeMap* tam = new TargetAgeMap();
			m_cacheTM.insert(make_pair(sp,make_pair(tpc,tam)));
			
			//tp is not found
	    std::auto_ptr<TargetPhrase> targetPhrase(new TargetPhrase(tp));
	    //Now that the source phrase is ready, we give the target phrase a copy
	    targetPhrase->SetSourcePhrase(sp);
			
	    targetPhrase->SetScore(m_feature,precomputedScores.at(age));
	    tpc->Add(targetPhrase.release());
	    size_t tp_pos = tpc->GetSize()-1;
	    AgePosPair app(age,tp_pos);
	    TargetAgePosPair taap(tp,app);
	    tam->insert(taap);
		}
	}
	
	void PhraseDictionaryCache::SetPreComputedScores(int numScoreComponent)
	{
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif        
		float sc;
		for (size_t i=0; i<=maxAge; i++)
		{
			if (i==maxAge){
	                	if ( score_type == CBTM_SCORE_TYPE_HYPERBOLA
        	        	        || score_type == CBTM_SCORE_TYPE_POWER
                		        || score_type == CBTM_SCORE_TYPE_EXPONENTIAL
                        		|| score_type == CBTM_SCORE_TYPE_COSINE )
	                	{
        	        	        sc = decaying_score(maxAge)/numScoreComponent;
                		}else{  // score_type = CBTM_SCORE_TYPE_XXXXXXXXX_REWARD
                		        sc = 0.0;
                		}
			}
			else{
				sc = decaying_score(i)/numScoreComponent;
			}

			Scores sc_vec; 
			for (size_t j=0; j<numScoreComponent; j++)
			{
				sc_vec.push_back(sc);   //CHECK THIS SCORE
			}
			precomputedScores.push_back(sc_vec);
		}
	}

        void PhraseDictionaryCache::Insert(std::vector<std::string> entries)
        {
                Decay();

		std::vector<std::string>::iterator itr = entries.begin();
                                                                      
                std::vector<std::string> pp;

                while(itr != entries.end())
                {
                	pp.clear();
			pp = TokenizeMultiCharSeparator((*itr), "|||");
                                                                                
                        Update(pp[0], pp[1], "1");
                                                                                
                        itr++;
                }

                IFVERBOSE(2) Print();
        }
	
	/*
	 * Traverse through the cache and ages every phrase pair.
	 */
	void PhraseDictionaryCache::Decay()
	{
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif          
		std::map<Phrase, TargetCollectionAgePair>::iterator it;
		for(it = m_cacheTM.begin(); it!=m_cacheTM.end(); it++)
		{
			Decay((*it).first);
		}
	}
	
	void PhraseDictionaryCache::Decay(Phrase p)
	{
		std::map<Phrase, TargetCollectionAgePair>::const_iterator it = m_cacheTM.find(p);
		if (it != m_cacheTM.end())
		{
			//p is found
			
			TargetCollectionAgePair TgtCollAgePair = it->second;
			TargetPhraseCollection* tpc = TgtCollAgePair.first;
			TargetAgeMap* tam = TgtCollAgePair.second;
			
			TargetAgeMap::iterator tam_it;
			for (tam_it=tam->begin(); tam_it!=tam->end();tam_it++)
			{
				int tp_age = ((*tam_it).second).first + 1; //increase the age by 1
				((*tam_it).second).first = tp_age;
				size_t tp_pos = ((*tam_it).second).second;
				
				TargetPhrase* tp_ptr = tpc->GetTargetPhrase(tp_pos);
				tp_ptr->SetScore(m_feature,precomputedScores[tp_age]);
			}
		}
		else
		{
			//do nothing
		}
		
		//put here the removal of entries with age greater than maxAge
	}

        void PhraseDictionaryCache::SetScoreType(size_t type) {
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif

                score_type = type;
                if ( score_type != CBTM_SCORE_TYPE_HYPERBOLA
                        && score_type != CBTM_SCORE_TYPE_POWER
                        && score_type != CBTM_SCORE_TYPE_EXPONENTIAL
                        && score_type != CBTM_SCORE_TYPE_COSINE
                        && score_type != CBTM_SCORE_TYPE_HYPERBOLA_REWARD
                        && score_type != CBTM_SCORE_TYPE_POWER_REWARD
                        && score_type != CBTM_SCORE_TYPE_EXPONENTIAL_REWARD )
                {
                               VERBOSE(2, "This score type " << score_type << " is unknown. Instead used " << CBTM_SCORE_TYPE_HYPERBOLA << "." << std::endl);
                               score_type = CBTM_SCORE_TYPE_HYPERBOLA;
                }
 
                VERBOSE(2, "PhraseDictionaryCache ScoreType:  " << score_type << std::endl);
        };

        void PhraseDictionaryCache::SetMaxAge(unsigned int age) {
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
                maxAge = age;
                VERBOSE(2, "PhraseDictionaryCache MaxAge:  " << maxAge << std::endl);
        };
	
	void PhraseDictionaryCache::Clear()
	{
#ifdef WITH_THREADS
                boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif          
		std::map<Phrase, TargetCollectionAgePair>::iterator it;
		for(it = m_cacheTM.begin(); it!=m_cacheTM.end(); it++)
		{
			delete ((*it).second).first;
			(((*it).second).second)->clear();
			delete ((*it).second).second;
		}
		
		m_cacheTM.clear();
	}
	
        float PhraseDictionaryCache::decaying_score(const int age)
        {
                float sc;
                switch(score_type){
                case CBTM_SCORE_TYPE_HYPERBOLA:
                        sc = (float) 1.0/age - 1.0;
                        break;
                case CBTM_SCORE_TYPE_POWER:
                        sc = (float) pow(age, -0.25) - 1.0;
                        break;
                case CBTM_SCORE_TYPE_EXPONENTIAL:
                        sc = (age == 1) ? 0.0 : (float) exp( 1.0/age ) / exp(1.0) - 1.0;
                        break;
                case CBTM_SCORE_TYPE_COSINE:
                        sc = (float) cos( (age-1) * (PI/2) / maxAge ) - 1.0;
                        break;
                case CBTM_SCORE_TYPE_HYPERBOLA_REWARD:
                        sc = (float) 1.0/age;
                        break;
                case CBTM_SCORE_TYPE_POWER_REWARD:
                        sc = (float) pow(age, -0.25);
                        break;
                case CBTM_SCORE_TYPE_EXPONENTIAL_REWARD:
                        sc = (age == 1) ? 1.0 : (float) exp( 1.0/age ) / exp(1.0);
	                break;
                default:
                        sc = -1.0;
                }
                return sc;
        }

}


