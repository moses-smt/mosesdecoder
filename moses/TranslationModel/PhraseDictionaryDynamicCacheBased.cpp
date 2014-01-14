// vim:tabstop=2

/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2006 University of Edinburgh

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/
#include "util/exception.hh"

#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/TranslationModel/PhraseDictionaryDynamicCacheBased.h"
#include "moses/FactorCollection.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "moses/TargetPhrase.h"

using namespace std;

namespace Moses
{
//! contructor
PhraseDictionaryDynamicCacheBased::PhraseDictionaryDynamicCacheBased(const std::string &line)
: PhraseDictionary("PhraseDictionaryDynamicCacheBased", line)
{
  std::cerr << "Initializing PhraseDictionaryDynamicCacheBased feature..." << std::endl;

  //disabling internal cache (provided by PhraseDictionary) for translation options (third parameter set to 0)
  m_maxCacheSize = 0;

  m_score_type = CBTM_SCORE_TYPE_HYPERBOLA;
  m_maxAge = 1000;
  m_entries = 0;
  ReadParameters();
}

PhraseDictionaryDynamicCacheBased::~PhraseDictionaryDynamicCacheBased()
{
  Clear();
}

void PhraseDictionaryDynamicCacheBased::Load()
{
  std::cerr << "PhraseDictionaryDynamicCacheBased::Load()" << std::endl;
  VERBOSE(2,"PhraseDictionaryDynamicCacheBased::Load()" << std::endl);
  SetFeaturesToApply();
  vector<float> weight = StaticData::Instance().GetWeights(this);
  SetPreComputedScores(weight.size());
  Load(m_initfiles);
}

void PhraseDictionaryDynamicCacheBased::Load(const std::string file)
{
  VERBOSE(2,"PhraseDictionaryDynamicCacheBased::Load(const std::string file)" << std::endl);
  std::vector<std::string> files = Tokenize(m_initfiles, "||");
  Load_Multiple_Files(files);
}

void PhraseDictionaryDynamicCacheBased::Load_Multiple_Files(std::vector<std::string> files)
{
  VERBOSE(2,"PhraseDictionaryDynamicCacheBased::Load_Multiple_Files(std::vector<std::string> files)" << std::endl);
  for(size_t j = 0; j < files.size(); ++j) {
    Load_Single_File(files[j]);
  }
}

void PhraseDictionaryDynamicCacheBased::Load_Single_File(const std::string file)
{
  VERBOSE(2,"PhraseDictionaryDynamicCacheBased::Load_Single_File(const std::string file)" << std::endl);
}

void PhraseDictionaryDynamicCacheBased::SetParameter(const std::string& key, const std::string& value)
{
  VERBOSE(2,"PhraseDictionaryDynamicCacheBased::SetParameter" << std::endl);

  if(key == "cbtm-score-type") {
    SetScoreType(Scan<size_t>(value));
  } else if (key == "cbtm-max-age") {
    SetMaxAge(Scan<unsigned int>(value));
  } else if (key == "cbtm-file") {
    m_initfiles = Scan<std::string>(value);
    Load(m_initfiles);
  } else {
    PhraseDictionary::SetParameter(key, value);
  }
}

const TargetPhraseCollection *PhraseDictionaryDynamicCacheBased::GetTargetPhraseCollection(const Phrase &source) const
{
  VERBOSE(2,"PhraseDictionaryDynamicCacheBased::GetTargetPhraseCollection(const Phrase &source) const START" << std::endl);
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
  const TargetPhraseCollection* tpc = NULL;
  VERBOSE(2,"source:|" << source << "|" << std::endl);
  std::map<Phrase, TargetCollectionAgePair>::const_iterator it = m_cacheTM.find(source);
  if(it != m_cacheTM.end())
  {
    VERBOSE(2,"source:|" << source << "| FOUND" << std::endl);
    tpc = (it->second).first;
  }
  VERBOSE(2,"PhraseDictionaryDynamicCacheBased::GetTargetPhraseCollection(const Phrase &source) const END" << std::endl);
  return tpc;
}

const TargetPhraseCollection* PhraseDictionaryDynamicCacheBased::GetTargetPhraseCollectionNonCacheLEGACY(Phrase const &src) const
{
  const TargetPhraseCollection *ret = GetTargetPhraseCollection(src);
  return ret;
}

ChartRuleLookupManager* PhraseDictionaryDynamicCacheBased::CreateRuleLookupManager(const ChartParser &, const ChartCellCollectionBase&)
{
  UTIL_THROW(util::Exception, "Phrase table used in chart decoder");
}

void PhraseDictionaryDynamicCacheBased::SetScoreType(size_t type) {
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif

  m_score_type = type;
  if ( m_score_type != CBTM_SCORE_TYPE_HYPERBOLA
       && m_score_type != CBTM_SCORE_TYPE_POWER
       && m_score_type != CBTM_SCORE_TYPE_EXPONENTIAL
       && m_score_type != CBTM_SCORE_TYPE_COSINE
       && m_score_type != CBTM_SCORE_TYPE_HYPERBOLA_REWARD
       && m_score_type != CBTM_SCORE_TYPE_POWER_REWARD
       && m_score_type != CBTM_SCORE_TYPE_EXPONENTIAL_REWARD )
  {
    VERBOSE(2, "This score type " << m_score_type << " is unknown. Instead used " << CBTM_SCORE_TYPE_HYPERBOLA << "." << std::endl);
    m_score_type = CBTM_SCORE_TYPE_HYPERBOLA;
  }
 
  VERBOSE(2, "PhraseDictionaryDynamicCacheBased ScoreType:  " << m_score_type << std::endl);
}


void PhraseDictionaryDynamicCacheBased::SetMaxAge(unsigned int age) {
 VERBOSE(2, "PhraseDictionaryCache MaxAge(unsigned int age) START" << std::endl);
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
  m_maxAge = age;
 VERBOSE(2, "PhraseDictionaryCache MaxAge:  " << m_maxAge << std::endl);
 VERBOSE(2, "PhraseDictionaryCache MaxAge(unsigned int age) END" << std::endl);
}

// friend
ostream& operator<<(ostream& out, const PhraseDictionaryDynamicCacheBased& phraseDict)
{
  return out;
}

float PhraseDictionaryDynamicCacheBased::decaying_score(const int age)
{
  float sc;
  switch(m_score_type){
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
    sc = (float) cos( (age-1) * (PI/2) / m_maxAge ) - 1.0;
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

void PhraseDictionaryDynamicCacheBased::SetPreComputedScores(const unsigned int numScoreComponent)
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::SetPreComputedScores(const unsigned int numScoreComponent) START" << std::endl);
  VERBOSE(1,"m_maxAge:|" << m_maxAge << "|" << std::endl);
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif        
  float sc;
  for (size_t i=0; i<=m_maxAge; i++)
  {
    if (i==m_maxAge){
      if ( m_score_type == CBTM_SCORE_TYPE_HYPERBOLA
         || m_score_type == CBTM_SCORE_TYPE_POWER
         || m_score_type == CBTM_SCORE_TYPE_EXPONENTIAL
         || m_score_type == CBTM_SCORE_TYPE_COSINE )
      {
        sc = decaying_score(m_maxAge)/numScoreComponent;
      }
      else{  // m_score_type = CBTM_SCORE_TYPE_XXXXXXXXX_REWARD
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
  VERBOSE(1,"precomputedScores.size():|" << precomputedScores.size() << "|" << std::endl);
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::SetPreComputedScores(const unsigned int numScoreComponent) END" << std::endl);
}

Scores PhraseDictionaryDynamicCacheBased::GetPreComputedScores(const unsigned int age)
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::GetPreComputedScores(const unsigned int) START" << std::endl);
  VERBOSE(1,"age:|" << age << "|" << std::endl);
  if (age < precomputedScores.size())
  {
    VERBOSE(1,"PhraseDictionaryDynamicCacheBased::GetPreComputedScores(const unsigned int) END" << std::endl);
    return precomputedScores.at(age);
  }
  else
  {
    VERBOSE(1,"PhraseDictionaryDynamicCacheBased::GetPreComputedScores(const unsigned int) END" << std::endl);
    return precomputedScores.at(m_maxAge);
  }
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::GetPreComputedScores(const unsigned int) END" << std::endl);
}

void PhraseDictionaryDynamicCacheBased::Insert(std::string &entries)
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Insert(std::string &entries) START" << std::endl);
  if (entries != "") {
    VERBOSE(1,"entries:|" << entries << "|" << std::endl);
    std::vector<std::string> elements = TokenizeMultiCharSeparator(entries, "||||");
    VERBOSE(1,"elements.size() after:|" << elements.size() << "|" << std::endl);
    Insert(elements);
  }
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Insert(std::string &entries) END" << std::endl);
}

void PhraseDictionaryDynamicCacheBased::Insert(std::vector<std::string> entries)
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Insert(std::vector<std::string> entries) START" << std::endl);
  VERBOSE(1,"entries.size():|" << entries.size() << "|" << std::endl);
  Decay();
  Update(entries, "1");
  IFVERBOSE(2) Print();
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Insert(std::vector<std::string> entries) END" << std::endl);
}


void PhraseDictionaryDynamicCacheBased::Update(std::vector<std::string> entries, std::string ageString)
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Update(std::vector<std::string> entries, std::string ageString)" << std::endl);
  std::vector<std::string> pp;

  std::vector<std::string>::iterator it;
  for(it = entries.begin(); it!=entries.end(); it++)
  {
    pp.clear();
    pp = TokenizeMultiCharSeparator((*it), "|||");
    VERBOSE(1,"pp[0]:|" << pp[0] << "|" << std::endl);
    VERBOSE(1,"pp[1]:|" << pp[1] << "|" << std::endl);
                                                                                
    Update(pp[0], pp[1], ageString);
  }
}

void PhraseDictionaryDynamicCacheBased::Update(std::string sourcePhraseString, std::string targetPhraseString, std::string ageString)
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Update(std::string sourcePhraseString, std::string targetPhraseString, std::string ageString)" << std::endl);
  const StaticData &staticData = StaticData::Instance();
  const std::string& factorDelimiter = staticData.GetFactorDelimiter();
  Phrase sourcePhrase(0);
  Phrase targetPhrase(0);
                
  char *err_ind_temp;
  int age = strtod(ageString.c_str(), &err_ind_temp);
  //target
  targetPhrase.Clear();
  VERBOSE(2, "targetPhraseString:|" << targetPhraseString << "|" << std::endl);
  targetPhrase.CreateFromString(Output, staticData.GetOutputFactorOrder(), targetPhraseString, factorDelimiter, NULL);
  VERBOSE(2, "targetPhrase:|" << targetPhrase << "|" << std::endl);
               
  //TODO: Would be better to reuse source phrases, but ownership has to be
  //consistent across phrase table implementations
  sourcePhrase.Clear();
  VERBOSE(2, "sourcePhraseString:|" << sourcePhraseString << "|" << std::endl);
  sourcePhrase.CreateFromString(Input, staticData.GetInputFactorOrder(), sourcePhraseString, factorDelimiter, NULL);
  VERBOSE(2, "sourcePhrase:|" << sourcePhrase << "|" << std::endl);
  Update(sourcePhrase, targetPhrase, age);
}

void PhraseDictionaryDynamicCacheBased::Update(Phrase sp, Phrase tp, int age)
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Update(Phrase sp, Phrase tp, int age)" << std::endl);
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  VERBOSE(2, "PhraseDictionaryCache inserting sp:|" << sp << "| tp:|" << tp << "| age:|" << age << "|" << std::endl);

//  std::map<Phrase, TargetCollectionAgePair>::const_iterator it = m_cacheTM.find(sp);
  std::map<Phrase, TargetCollectionAgePair>::const_iterator it = m_cacheTM.begin();
  VERBOSE(1,"sp:|" << sp << "|" << std::endl);
  if(it!=m_cacheTM.end())
  {
    VERBOSE(1,"sp:|" << sp << "| FOUND" << std::endl);
    // p is found
    // here we have to remove the target phrase from targetphrasecollection and from the TargetAgeMap
    // and then add new entry

    TargetCollectionAgePair TgtCollAgePair = it->second;
    TargetPhraseCollection* tpc = TgtCollAgePair.first;
    AgeCollection* ac = TgtCollAgePair.second;
    const Phrase* tp_ptr = NULL;
    bool found = false;
    size_t tp_pos=0;
    while (!found && tp_pos < tpc->GetSize())
    {
      tp_ptr = (const Phrase*) tpc->GetTargetPhrase(tp_pos);
      if (tp == *tp_ptr)
//      if (tp == tp_ptr->GetSourcePhrase())
      {
        found = true;
        continue;
      }
      tp_pos++;
    }
    if (!found)
    {
      VERBOSE(1,"tp:|" << tp << "| NOT FOUND" << std::endl);
/*
      std::auto_ptr<TargetPhrase> targetPhrase(new TargetPhrase(tp));
      //Now that the source phrase is ready, we give the target phrase a copy
      targetPhrase->SetSourcePhrase(sp);
*/
      std::auto_ptr<TargetPhrase> targetPhrase(new TargetPhrase(tp));

      targetPhrase->GetScoreBreakdown().Assign(this, GetPreComputedScores(age));
      tpc->Add(targetPhrase.release());
      tp_pos = tpc->GetSize()-1;
      ac->push_back(age);
      m_entries++;
      VERBOSE(1,"tpc size:|" << tpc->GetSize() << "|" << std::endl);
      VERBOSE(1,"ac size:|" << ac->size() << "|" << std::endl);
      VERBOSE(1,"tp:|" << tp << "| INSERTED" << std::endl);
    }
  }
  else
  {
    VERBOSE(1,"sp:|" << sp << "| NOT FOUND" << std::endl);
    // p is not found
    // create target collection
    // we have to create new target collection age pair and add new entry to target collection age pair

    TargetPhraseCollection* tpc = new TargetPhraseCollection();
    AgeCollection* ac = new AgeCollection();
    m_cacheTM.insert(make_pair(sp,make_pair(tpc,ac)));
    VERBOSE(1,"HERE 1" << std::endl);

    //tp is not found
/*      
    std::auto_ptr<TargetPhrase> targetPhrase(new TargetPhrase(tp));
    //Now that the source phrase is ready, we give the target phrase a copy
    targetPhrase->SetSourcePhrase(sp);
*/      
    std::auto_ptr<TargetPhrase> targetPhrase(new TargetPhrase(tp));

    targetPhrase->GetScoreBreakdown().Assign(this, GetPreComputedScores(age));
    tpc->Add(targetPhrase.release());
    ac->push_back(age);
    m_entries++;
    VERBOSE(1,"tpc size:|" << tpc->GetSize() << "|" << std::endl);
    VERBOSE(1,"ac size:|" << ac->size() << "|" << std::endl);
    VERBOSE(1,"sp:|" << sp << "| tp:|" << tp << "| INSERTED" << std::endl);
  }
}


/*
void PhraseDictionaryDynamicCacheBased::Update(Phrase sp, Phrase tp, int age)
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Update(Phrase sp, Phrase tp, int age)" << std::endl);
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif          
  VERBOSE(2, "PhraseDictionaryCache inserting sp:|" << sp << "| tp:|" << tp << "| age:|" << age << "|" << std::endl);
                
//  std::map<Phrase, TargetCollectionAgePair>::const_iterator it = m_cacheTM.find(sp);
  std::map<Phrase, TargetCollectionAgePair>::const_iterator it = m_cacheTM.begin();
  VERBOSE(1,"sp:|" << sp << "|" << std::endl);
  if(it!=m_cacheTM.end())
  {
    VERBOSE(1,"sp:|" << sp << "| FOUND" << std::endl);
    // p is found
    // here we have to remove the target phrase from targetphrasecollection and from the TargetAgeMap
    // and then add new entry

    TargetCollectionAgePair TgtCollAgePair = it->second;
    TargetPhraseCollection* tpc = TgtCollAgePair.first;
    TargetAgeMap* tam = TgtCollAgePair.second;
    TargetAgeMap::iterator tam_it = tam->find(tp);
    if (tam_it!=tam->end())
    {
      VERBOSE(1,"tp:|" << tp << "| FOUND" << std::endl);
      //tp is found
      size_t tp_pos = ((*tam_it).second).second;
      ((*tam_it).second).first = age;
      TargetPhrase* tp_ptr = tpc->GetTargetPhrase(tp_pos);
//      tp_ptr->SetScore(m_feature,precomputedScores[age]);
//      tp_ptr->GetScoreBreakdown().Assign(m_feature, precomputedScores.at(age));
      tp_ptr->GetScoreBreakdown().Assign(this, GetPreComputedScores(age));
      VERBOSE(1,"tp:|" << tp << "| UPDATED" << std::endl);
    }
    else
    {
      VERBOSE(1,"tp:|" << tp << "| NOT FOUND" << std::endl);
      //tp is not found
      std::auto_ptr<TargetPhrase> targetPhrase(new TargetPhrase(tp));
      //Now that the source phrase is ready, we give the target phrase a copy
      targetPhrase->SetSourcePhrase(sp);
//      targetPhrase->SetScore(m_feature,precomputedScores[age]);
      targetPhrase->GetScoreBreakdown().Assign(this, GetPreComputedScores(age));
      tpc->Add(targetPhrase.release());
      size_t tp_pos = tpc->GetSize()-1;
      AgePosPair app(age,tp_pos);
      TargetAgePosPair taap(tp,app);
      tam->insert(taap);
      m_entries++;
      VERBOSE(1,"tp:|" << tp << "| INSERTED" << std::endl);
    }
  }
  else
  {
    VERBOSE(1,"sp:|" << sp << "| NOT FOUND" << std::endl);
    // p is not found
    // create target collection
    // we have to create new target collection age pair and add new entry to target collection age pair
                        
    TargetPhraseCollection* tpc = new TargetPhraseCollection();
    TargetAgeMap* tam = new TargetAgeMap();
    m_cacheTM.insert(make_pair(sp,make_pair(tpc,tam)));
    VERBOSE(1,"HERE 1" << std::endl);
                       
    //tp is not found
    std::auto_ptr<TargetPhrase> targetPhrase(new TargetPhrase(tp));
    //Now that the source phrase is ready, we give the target phrase a copy
    targetPhrase->SetSourcePhrase(sp);
                        
//    targetPhrase->SetScore(m_feature,precomputedScores.at(age));
    targetPhrase->GetScoreBreakdown().Assign(this, GetPreComputedScores(age));
    tpc->Add(targetPhrase.release());
    size_t tp_pos = tpc->GetSize()-1;
    AgePosPair app(age,tp_pos);
    TargetAgePosPair taap(tp,app);
    tam->insert(taap);
    m_entries++;
    VERBOSE(1,"sp:|" << sp << "| tp:|" << tp << "| INSERTED" << std::endl);
  }
}
*/

void PhraseDictionaryDynamicCacheBased::Decay()
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Decay() START" << std::endl);
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif          
  std::map<Phrase, TargetCollectionAgePair>::iterator it;
  for(it = m_cacheTM.begin(); it!=m_cacheTM.end(); it++)
  {
    Decay((*it).first);
  }
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Decay() END" << std::endl);
}
     
void PhraseDictionaryDynamicCacheBased::Decay(Phrase p)
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Decay(Phrase p) START" << std::endl);
  VERBOSE(1,"p:|" << p << "|" << std::endl);
  cacheMap::const_iterator it = m_cacheTM.find(p);
  VERBOSE(1,"searching:|" << p << "|" << std::endl);
  if (it != m_cacheTM.end())
  {
    VERBOSE(1,"found:|" << p << "|" << std::endl);
    //p is found

    TargetCollectionAgePair TgtCollAgePair = it->second;
    TargetPhraseCollection* tpc = TgtCollAgePair.first;
    AgeCollection* ac = TgtCollAgePair.second;

//loop in inverted order to allow a correct deletion of std::vectors tpc and ac
    for (int tp_pos = tpc->GetSize() - 1 ; tp_pos >= 0; tp_pos--)
    {
      VERBOSE(1,"p:|" << p << "|" << std::endl);
      unsigned int tp_age = ac->at(tp_pos); //increase the age by 1
      tp_age++; //increase the age by 1
      VERBOSE(1,"p:|" << p << "| " << " new tp_age:|" << tp_age << "|" << std::endl);

      TargetPhrase* tp_ptr = (TargetPhrase*) tpc->GetTargetPhrase(tp_pos);
      VERBOSE(1,"p:|" << p << "| " << "tp_age:|" << tp_age << "| " <<  "*tp_ptr:|" << *tp_ptr << "|" << std::endl);
      VERBOSE(1,"precomputedScores.size():|" << precomputedScores.size() << "|" << std::endl);

      if (tp_age > m_maxAge){
        VERBOSE(1,"tp_age:|" << tp_age << "| TOO BIG" << std::endl);
        tpc->Remove(tp_pos); //delete entry in the Target Phrase Collection
        ac->erase(ac->begin() + tp_pos); //delete entry in the Age Collection
        m_entries--;
      }
      else{
        VERBOSE(1,"tp_age:|" << tp_age << "| STILL GOOD" << std::endl);
        tp_ptr->GetScoreBreakdown().Assign(this, GetPreComputedScores(tp_age));
        ac->at(tp_pos) = tp_age;
        VERBOSE(1,"precomputedScores.size():|" << precomputedScores.size() << "|" << std::endl);
      }
    }
    if (tpc->GetSize() == 0)
    {// delete the entry from m_cacheTM in case it points to an empty TargetPhraseCollection and AgeCollection
      (((*it).second).second)->clear();
      delete ((*it).second).second;
      delete ((*it).second).first;
      m_cacheTM.erase(p);
    }
  }
  else
  {
    //do nothing
    VERBOSE(1,"p:|" << p << "| NOT FOUND" << std::endl);
  }

  //put here the removal of entries with age greater than m_maxAge
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Decay(Phrase p) END" << std::endl);
}

/*   
void PhraseDictionaryDynamicCacheBased::Decay(Phrase p)
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Decay(Phrase p) START" << std::endl);
  VERBOSE(1,"p:|" << p << "|" << std::endl);
  std::map<Phrase, TargetCollectionAgePair>::const_iterator it = m_cacheTM.find(p);
  VERBOSE(1,"searching:|" << p << "|" << std::endl);
  if (it != m_cacheTM.end())
  {
    VERBOSE(1,"found:|" << p << "|" << std::endl);
    //p is found
                        	
    TargetCollectionAgePair TgtCollAgePair = it->second;
    TargetPhraseCollection* tpc = TgtCollAgePair.first;
    TargetAgeMap* tam = TgtCollAgePair.second;
                        
    TargetAgeMap::iterator tam_it;
    for (tam_it=tam->begin(); tam_it!=tam->end();tam_it++)
    {
      VERBOSE(1,"p:|" << p << "|" << std::endl);
      int tp_age = ((*tam_it).second).first + 1; //increase the age by 1

      VERBOSE(1,"p:|" << p << "| " << " new tp_age:|" << tp_age << "|" << std::endl);
      ((*tam_it).second).first = tp_age;
      size_t tp_pos = ((*tam_it).second).second;

      TargetPhrase* tp_ptr = tpc->GetTargetPhrase(tp_pos);
      VERBOSE(1,"p:|" << p << "| " << "tp_age:|" << tp_age << "| " <<  "*tp_ptr:|" << *tp_ptr << "|" << std::endl);
      VERBOSE(1,"precomputedScores.size():|" << precomputedScores.size() << "|" << std::endl);

      tp_ptr->GetScoreBreakdown().Assign(this, GetPreComputedScores(tp_age));
      VERBOSE(1,"precomputedScores.size():|" << precomputedScores.size() << "|" << std::endl);
    }
  }
  else
  {
    //do nothing
    VERBOSE(1,"p:|" << p << "| NOT FOUND" << std::endl);
  }
                
  //put here the removal of entries with age greater than m_maxAge
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Decay(Phrase p) END" << std::endl);
}
*/

void PhraseDictionaryDynamicCacheBased::Execute(std::string command)
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Execute(std::string command) START" << std::endl);
  VERBOSE(1,"command:|" << command << "|" << std::endl);
  std::vector<std::string> commands = Tokenize(command, "||");
  Execute(commands);
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Execute(std::string command) END" << std::endl);
}

void PhraseDictionaryDynamicCacheBased::Execute(std::vector<std::string> commands)
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Execute(std::vector<std::string> commands) START" << std::endl);
  for (size_t j=0; j<commands.size(); j++) {
    Execute_Single_Command(commands[j]);
  }
  IFVERBOSE(2) Print();
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Execute(std::vector<std::string> commands) END" << std::endl);
}

void PhraseDictionaryDynamicCacheBased::Execute_Single_Command(std::string command)
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Execute_Single_Command(std::string command) START" << std::endl);
  VERBOSE(1,"command:|" << command << "|" << std::endl);
  if (command == "clear") {
    VERBOSE(1,"PhraseDictionaryDynamicCacheBased Execute command:|"<< command << "|. Cache cleared." << std::endl);
    Clear();
  } else {
    VERBOSE(1,"PhraseDictionaryDynamicCacheBased Execute command:|"<< command << "| is unknown. Skipped." << std::endl);
  }
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Execute_Single_Command(std::string command) END" << std::endl);
}


void PhraseDictionaryDynamicCacheBased::Clear()
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Clear() START" << std::endl);
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  std::map<Phrase, TargetCollectionAgePair>::iterator it;
  for(it = m_cacheTM.begin(); it!=m_cacheTM.end(); it++)
  {
    (((*it).second).second)->clear();
    delete ((*it).second).second;
    delete ((*it).second).first;
  }
  m_cacheTM.clear();
  m_entries = 0;
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Clear() END" << std::endl);
}

void PhraseDictionaryDynamicCacheBased::Print() const
{
  VERBOSE(1,"PhraseDictionaryDynamicCacheBased::Print()" << std::endl);
  VERBOSE(1,"entries:|" << m_entries << "|" << std::endl);
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif          
  std::map<Phrase, TargetCollectionAgePair>::const_iterator it;
  for(it = m_cacheTM.begin(); it!=m_cacheTM.end(); it++)
  {
    std::string source = (it->first).ToString();
    TargetPhraseCollection* tpc = (it->second).first;
//    std::vector<const TargetPhrase*> TPCvector;// = tpc->GetCollection();
//    TPCvector.reserve(tpc->GetSize());
//    TPCvector = tpc->GetCollection();
//    std::vector<TargetPhrase*>::iterator itr;
//    for(itr = TPCvector.begin(); itr != TPCvector.end(); itr++)
    TargetPhraseCollection::iterator itr;
//    std::vector<TargetPhrase*>::iterator itr;
    for(itr = tpc->begin(); itr != tpc->end(); itr++)
    {
      std::string target = (*itr)->ToString();
      VERBOSE(1, source << " ||| " << target << std::endl);
    }
//    TPCvector.clear();
    source.clear();
  }
}
        
}// end namespace
