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
PhraseDictionaryDynamicCacheBased *PhraseDictionaryDynamicCacheBased::s_instance = NULL;

//! contructor
PhraseDictionaryDynamicCacheBased::PhraseDictionaryDynamicCacheBased(const std::string &line)
  : PhraseDictionary(line)
{
  std::cerr << "Initializing PhraseDictionaryDynamicCacheBased feature..." << std::endl;

  //disabling internal cache (provided by PhraseDictionary) for translation options (third parameter set to 0)
  m_maxCacheSize = 0;

  m_score_type = CBTM_SCORE_TYPE_HYPERBOLA;
  m_maxAge = 1000;
  m_entries = 0;
  ReadParameters();
  UTIL_THROW_IF2(s_instance, "Can only have 1 PhraseDictionaryDynamicCacheBased feature");
  s_instance = this;
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
  VERBOSE(2, "PhraseDictionaryDynamicCacheBased::SetParameter key:|" << key << "| value:|" << value << "|" << std::endl);

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

void PhraseDictionaryDynamicCacheBased::InitializeForInput(InputType const& source)
{
  ReduceCache();
}

const TargetPhraseCollection *PhraseDictionaryDynamicCacheBased::GetTargetPhraseCollection(const Phrase &source) const
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
  TargetPhraseCollection* tpc = NULL;
  VERBOSE(3,"source:|" << source << "|" << std::endl);
  cacheMap::const_iterator it = m_cacheTM.find(source);
  if(it != m_cacheTM.end()) {
    VERBOSE(3,"source:|" << source << "| FOUND" << std::endl);
    tpc = (it->second).first;

    std::vector<const TargetPhrase*>::const_iterator it2 = tpc->begin();

    while (it2 != tpc->end()) {
      ((TargetPhrase*) *it2)->Evaluate(source, GetFeaturesToApply());
      it2++;
    }
  }
  if (tpc)  {
    tpc->NthElement(m_tableLimit); // sort the phrases for the decoder
  }

  return tpc;
}

const TargetPhraseCollection* PhraseDictionaryDynamicCacheBased::GetTargetPhraseCollectionNonCacheLEGACY(Phrase const &src) const
{
  const TargetPhraseCollection *ret = GetTargetPhraseCollection(src);
  return ret;
}

ChartRuleLookupManager* PhraseDictionaryDynamicCacheBased::CreateRuleLookupManager(const ChartParser &parser, const ChartCellCollectionBase &cellCollection)
{
  UTIL_THROW(util::Exception, "Phrase table used in chart decoder");
}

void PhraseDictionaryDynamicCacheBased::SetScoreType(size_t type)
{
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
       && m_score_type != CBTM_SCORE_TYPE_EXPONENTIAL_REWARD ) {
    VERBOSE(2, "This score type " << m_score_type << " is unknown. Instead used " << CBTM_SCORE_TYPE_HYPERBOLA << "." << std::endl);
    m_score_type = CBTM_SCORE_TYPE_HYPERBOLA;
  }

  VERBOSE(2, "PhraseDictionaryDynamicCacheBased ScoreType:  " << m_score_type << std::endl);
}


void PhraseDictionaryDynamicCacheBased::SetMaxAge(unsigned int age)
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
  m_maxAge = age;
  VERBOSE(2, "PhraseDictionaryCache MaxAge:  " << m_maxAge << std::endl);
}


// friend
ostream& operator<<(ostream& out, const PhraseDictionaryDynamicCacheBased& phraseDict)
{
  return out;
}

float PhraseDictionaryDynamicCacheBased::decaying_score(const int age)
{
  float sc;
  switch(m_score_type) {
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
  VERBOSE(3,"m_maxAge:|" << m_maxAge << "|" << std::endl);
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  float sc;
  for (size_t i=0; i<=m_maxAge; i++) {
    if (i==m_maxAge) {
      if ( m_score_type == CBTM_SCORE_TYPE_HYPERBOLA
           || m_score_type == CBTM_SCORE_TYPE_POWER
           || m_score_type == CBTM_SCORE_TYPE_EXPONENTIAL
           || m_score_type == CBTM_SCORE_TYPE_COSINE ) {
        sc = decaying_score(m_maxAge)/numScoreComponent;
      } else { // m_score_type = CBTM_SCORE_TYPE_XXXXXXXXX_REWARD
        sc = 0.0;
      }
    } else {
      sc = decaying_score(i)/numScoreComponent;
    }
    Scores sc_vec;
    for (size_t j=0; j<numScoreComponent; j++) {
      sc_vec.push_back(sc);   //CHECK THIS SCORE
    }
    precomputedScores.push_back(sc_vec);
  }
  VERBOSE(3,"precomputedScores.size():|" << precomputedScores.size() << "|" << std::endl);
}

Scores PhraseDictionaryDynamicCacheBased::GetPreComputedScores(const unsigned int age)
{
  VERBOSE(3,"age:|" << age << "|" << std::endl);
  if (age < precomputedScores.size()) {
    return precomputedScores.at(age);
  } else {
    return precomputedScores.at(m_maxAge);
  }
}

void PhraseDictionaryDynamicCacheBased::Insert(std::string &entries)
{
  if (entries != "") {
    VERBOSE(3,"entries:|" << entries << "|" << std::endl);
    std::vector<std::string> elements = TokenizeMultiCharSeparator(entries, "||||");
    VERBOSE(3,"elements.size() after:|" << elements.size() << "|" << std::endl);
    Insert(elements);
  }
}

void PhraseDictionaryDynamicCacheBased::Insert(std::vector<std::string> entries)
{
  VERBOSE(3,"entries.size():|" << entries.size() << "|" << std::endl);
  Decay();
  Update(entries, "1");
  IFVERBOSE(2) Print();
}


void PhraseDictionaryDynamicCacheBased::Update(std::vector<std::string> entries, std::string ageString)
{
  VERBOSE(3,"PhraseDictionaryDynamicCacheBased::Update(std::vector<std::string> entries, std::string ageString)" << std::endl);
  std::vector<std::string> pp;

  std::vector<std::string>::iterator it;
  for(it = entries.begin(); it!=entries.end(); it++) {
    pp.clear();
    pp = TokenizeMultiCharSeparator((*it), "|||");
    VERBOSE(3,"pp[0]:|" << pp[0] << "|" << std::endl);
    VERBOSE(3,"pp[1]:|" << pp[1] << "|" << std::endl);

    Update(pp[0], pp[1], ageString);
  }
}

void PhraseDictionaryDynamicCacheBased::Update(std::string sourcePhraseString, std::string targetPhraseString, std::string ageString)
{
  VERBOSE(3,"PhraseDictionaryDynamicCacheBased::Update(std::string sourcePhraseString, std::string targetPhraseString, std::string ageString)" << std::endl);
  const StaticData &staticData = StaticData::Instance();
  const std::string& factorDelimiter = staticData.GetFactorDelimiter();
  Phrase sourcePhrase(0);
  Phrase targetPhrase(0);

  char *err_ind_temp;
  int age = strtod(ageString.c_str(), &err_ind_temp);
  //target
  targetPhrase.Clear();
  VERBOSE(3, "targetPhraseString:|" << targetPhraseString << "|" << std::endl);
  targetPhrase.CreateFromString(Output, staticData.GetOutputFactorOrder(), targetPhraseString, factorDelimiter, NULL);
  VERBOSE(2, "targetPhrase:|" << targetPhrase << "|" << std::endl);

  //TODO: Would be better to reuse source phrases, but ownership has to be
  //consistent across phrase table implementations
  sourcePhrase.Clear();
  VERBOSE(3, "sourcePhraseString:|" << sourcePhraseString << "|" << std::endl);
  sourcePhrase.CreateFromString(Input, staticData.GetInputFactorOrder(), sourcePhraseString, factorDelimiter, NULL);
  VERBOSE(3, "sourcePhrase:|" << sourcePhrase << "|" << std::endl);
  Update(sourcePhrase, targetPhrase, age);
}

void PhraseDictionaryDynamicCacheBased::Update(Phrase sp, Phrase tp, int age)
{
  VERBOSE(3,"PhraseDictionaryDynamicCacheBased::Update(Phrase sp, Phrase tp, int age)" << std::endl);
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  VERBOSE(3, "PhraseDictionaryCache inserting sp:|" << sp << "| tp:|" << tp << "| age:|" << age << "|" << std::endl);

  cacheMap::const_iterator it = m_cacheTM.find(sp);
  VERBOSE(3,"sp:|" << sp << "|" << std::endl);
  if(it!=m_cacheTM.end()) {
    VERBOSE(3,"sp:|" << sp << "| FOUND" << std::endl);
    // p is found
    // here we have to remove the target phrase from targetphrasecollection and from the TargetAgeMap
    // and then add new entry

    TargetCollectionAgePair TgtCollAgePair = it->second;
    TargetPhraseCollection* tpc = TgtCollAgePair.first;
    AgeCollection* ac = TgtCollAgePair.second;
    const Phrase* tp_ptr = NULL;
    bool found = false;
    size_t tp_pos=0;
    while (!found && tp_pos < tpc->GetSize()) {
      tp_ptr = (const Phrase*) tpc->GetTargetPhrase(tp_pos);
      if (tp == *tp_ptr) {
        found = true;
        continue;
      }
      tp_pos++;
    }
    if (!found) {
      VERBOSE(3,"tp:|" << tp << "| NOT FOUND" << std::endl);
      std::auto_ptr<TargetPhrase> targetPhrase(new TargetPhrase(tp));

      targetPhrase->GetScoreBreakdown().Assign(this, GetPreComputedScores(age));
      tpc->Add(targetPhrase.release());
      tp_pos = tpc->GetSize()-1;
      ac->push_back(age);
      m_entries++;
      VERBOSE(3,"tpc size:|" << tpc->GetSize() << "|" << std::endl);
      VERBOSE(3,"ac size:|" << ac->size() << "|" << std::endl);
      VERBOSE(3,"tp:|" << tp << "| INSERTED" << std::endl);
    }
  } else {
    VERBOSE(3,"sp:|" << sp << "| NOT FOUND" << std::endl);
    // p is not found
    // create target collection
    // we have to create new target collection age pair and add new entry to target collection age pair

    TargetPhraseCollection* tpc = new TargetPhraseCollection();
    AgeCollection* ac = new AgeCollection();
    m_cacheTM.insert(make_pair(sp,make_pair(tpc,ac)));

    //tp is not found
    std::auto_ptr<TargetPhrase> targetPhrase(new TargetPhrase(tp));

    targetPhrase->GetScoreBreakdown().Assign(this, GetPreComputedScores(age));
    tpc->Add(targetPhrase.release());
    ac->push_back(age);
    m_entries++;
    VERBOSE(3,"tpc size:|" << tpc->GetSize() << "|" << std::endl);
    VERBOSE(3,"ac size:|" << ac->size() << "|" << std::endl);
    VERBOSE(3,"sp:|" << sp << "| tp:|" << tp << "| INSERTED" << std::endl);
  }
}

void PhraseDictionaryDynamicCacheBased::Decay()
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  cacheMap::iterator it;
  for(it = m_cacheTM.begin(); it!=m_cacheTM.end(); it++) {
    Decay((*it).first);
  }
}

void PhraseDictionaryDynamicCacheBased::Decay(Phrase p)
{
  VERBOSE(3,"p:|" << p << "|" << std::endl);
  cacheMap::const_iterator it = m_cacheTM.find(p);
  VERBOSE(3,"searching:|" << p << "|" << std::endl);
  if (it != m_cacheTM.end()) {
    VERBOSE(3,"found:|" << p << "|" << std::endl);
    //p is found

    TargetCollectionAgePair TgtCollAgePair = it->second;
    TargetPhraseCollection* tpc = TgtCollAgePair.first;
    AgeCollection* ac = TgtCollAgePair.second;

//loop in inverted order to allow a correct deletion of std::vectors tpc and ac
    for (int tp_pos = tpc->GetSize() - 1 ; tp_pos >= 0; tp_pos--) {
      VERBOSE(3,"p:|" << p << "|" << std::endl);
      unsigned int tp_age = ac->at(tp_pos); //increase the age by 1
      tp_age++; //increase the age by 1
      VERBOSE(3,"p:|" << p << "| " << " new tp_age:|" << tp_age << "|" << std::endl);

      TargetPhrase* tp_ptr = (TargetPhrase*) tpc->GetTargetPhrase(tp_pos);
      VERBOSE(3,"p:|" << p << "| " << "tp_age:|" << tp_age << "| " <<  "*tp_ptr:|" << *tp_ptr << "|" << std::endl);
      VERBOSE(3,"precomputedScores.size():|" << precomputedScores.size() << "|" << std::endl);

      if (tp_age > m_maxAge) {
        VERBOSE(3,"tp_age:|" << tp_age << "| TOO BIG" << std::endl);
        tpc->Remove(tp_pos); //delete entry in the Target Phrase Collection
        ac->erase(ac->begin() + tp_pos); //delete entry in the Age Collection
        m_entries--;
      } else {
        VERBOSE(3,"tp_age:|" << tp_age << "| STILL GOOD" << std::endl);
        tp_ptr->GetScoreBreakdown().Assign(this, GetPreComputedScores(tp_age));
        ac->at(tp_pos) = tp_age;
        VERBOSE(3,"precomputedScores.size():|" << precomputedScores.size() << "|" << std::endl);
      }
    }
    if (tpc->GetSize() == 0) {
      // delete the entry from m_cacheTM in case it points to an empty TargetPhraseCollection and AgeCollection
      (((*it).second).second)->clear();
      delete ((*it).second).second;
      delete ((*it).second).first;
      m_cacheTM.erase(p);
    }
  } else {
    //do nothing
    VERBOSE(3,"p:|" << p << "| NOT FOUND" << std::endl);
  }

  //put here the removal of entries with age greater than m_maxAge
}

void PhraseDictionaryDynamicCacheBased::Execute(std::string command)
{
  VERBOSE(2,"command:|" << command << "|" << std::endl);
  std::vector<std::string> commands = Tokenize(command, "||");
  Execute(commands);
}

void PhraseDictionaryDynamicCacheBased::Execute(std::vector<std::string> commands)
{
  for (size_t j=0; j<commands.size(); j++) {
    Execute_Single_Command(commands[j]);
  }
  IFVERBOSE(2) Print();
}

void PhraseDictionaryDynamicCacheBased::Execute_Single_Command(std::string command)
{
  if (command == "clear") {
    VERBOSE(2,"PhraseDictionaryDynamicCacheBased Execute command:|"<< command << "|. Cache cleared." << std::endl);
    Clear();
  } else {
    VERBOSE(2,"PhraseDictionaryDynamicCacheBased Execute command:|"<< command << "| is unknown. Skipped." << std::endl);
  }
}


void PhraseDictionaryDynamicCacheBased::Clear()
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  cacheMap::const_iterator it;
  for(it = m_cacheTM.begin(); it!=m_cacheTM.end(); it++) {
    (((*it).second).second)->clear();
    delete ((*it).second).second;
    delete ((*it).second).first;
  }
  m_cacheTM.clear();
  m_entries = 0;
}

void PhraseDictionaryDynamicCacheBased::Print() const
{
  VERBOSE(2,"PhraseDictionaryDynamicCacheBased::Print()" << std::endl);
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
  cacheMap::const_iterator it;
  for(it = m_cacheTM.begin(); it!=m_cacheTM.end(); it++) {
    std::string source = (it->first).ToString();
    TargetPhraseCollection* tpc = (it->second).first;
    TargetPhraseCollection::iterator itr;
    for(itr = tpc->begin(); itr != tpc->end(); itr++) {
      std::string target = (*itr)->ToString();
      std::cout << source << " ||| " << target << std::endl;
    }
    source.clear();
  }
}

}// end namespace
