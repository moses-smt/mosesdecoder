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
std::map< const std::string, PhraseDictionaryDynamicCacheBased * > PhraseDictionaryDynamicCacheBased::s_instance_map;
PhraseDictionaryDynamicCacheBased *PhraseDictionaryDynamicCacheBased::s_instance = NULL;

//! contructor
PhraseDictionaryDynamicCacheBased::PhraseDictionaryDynamicCacheBased(const std::string &line)
  : PhraseDictionary(line, true)
{
  std::cerr << "Initializing PhraseDictionaryDynamicCacheBased feature..." << std::endl;

  //disabling internal cache (provided by PhraseDictionary) for translation options (third parameter set to 0)
  m_maxCacheSize = 0;

  m_score_type = CBTM_SCORE_TYPE_HYPERBOLA;
  m_maxAge = 1000;
  m_entries = 0;
  m_name = "default";
  m_constant = false;

  ReadParameters();

  UTIL_THROW_IF2(s_instance_map.find(m_name) != s_instance_map.end(), "Only 1 PhraseDictionaryDynamicCacheBased feature named " + m_name + " is allowed");
  s_instance_map[m_name] = this;
  s_instance = this; //for back compatibility
  vector<float> weight = StaticData::Instance().GetWeights(this);
  m_numscorecomponent = weight.size();
}

PhraseDictionaryDynamicCacheBased::~PhraseDictionaryDynamicCacheBased()
{
  Clear();
}

void PhraseDictionaryDynamicCacheBased::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  VERBOSE(2,"PhraseDictionaryDynamicCacheBased::Load()" << std::endl);
  SetFeaturesToApply();

  SetPreComputedScores(1);
  // weights.size() doesn't make sense at all.. why would you have multiple ages for a unique phrase pair??
//  SetPreComputedScores(m_numscorecomponent);

  Load(m_initfiles);
}

void PhraseDictionaryDynamicCacheBased::Load(const std::string filestr)
{
  VERBOSE(2,"PhraseDictionaryDynamicCacheBased::Load(const std::string filestr)" << std::endl);
//  std::vector<std::string> files = Tokenize(m_initfiles, "||");
  std::vector<std::string> files = Tokenize(filestr, "||");
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
  //file format
  //age |||| src_phr ||| trg_phr
  //age |||| src_phr2 ||| trg_phr2 |||| src_phr3 ||| trg_phr3 |||| src_phr4 ||| trg_ph4
  //....
  //or
  //age |||| src_phr ||| trg_phr ||| scores ||| wa_align
  //age |||| src_phr2 ||| trg_phr2 ||| scores2 ||| wa_align2 |||| src_phr3 ||| trg_phr3 ||| scores3 ||| wa_align3 |||| src_phr4 ||| trg_phr4 ||| scores4 ||| wa_align4
  //....
  //each src_phr ad trg_phr are sequences of src and trg words, respectively, of any length
  //if provided, wa_align is the alignment between src_phr and trg_phr
  //scores is the feature scores associated to the source phrase and the target phrase
  //there is no limit on the size of n
  //
  //entries can be repeated, but the last entry overwrites the previous


  VERBOSE(2,"Loading data from the cache file " << file << std::endl);
  InputFileStream cacheFile(file);

  std::string line;
  std::vector<std::string> words;

  while (getline(cacheFile, line)) {
    std::vector<std::string> vecStr = TokenizeMultiCharSeparator( line , "||||" );
    if (vecStr.size() >= 2) {
      std::string ageString = vecStr[0];
      vecStr.erase(vecStr.begin());
      Update(vecStr,ageString);
    } else {
      UTIL_THROW_IF2(false, "The format of the loaded file is wrong: " << line);
    }
  }
  IFVERBOSE(2) Print();
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
  } else if (key == "cbtm-name") {
    m_name = Scan<std::string>(value);
  } else if (key == "cbtm-constant") {
    m_constant = Scan<bool>(value);
  } else if (key == "input-factor") {
    m_inputFactorsVec = Tokenize<FactorType>(value,",");
  } else if (key == "output-factor") {
    m_outputFactorsVec = Tokenize<FactorType>(value,",");
  } else {
    PhraseDictionary::SetParameter(key, value);
  }
}

void PhraseDictionaryDynamicCacheBased::InitializeForInput(ttasksptr const& ttask)
{
  ReduceCache();
}

TargetPhraseCollection::shared_ptr PhraseDictionaryDynamicCacheBased::GetTargetPhraseCollection(const Phrase &source) const
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
  TargetPhraseCollection::shared_ptr tpc;
  cacheMap::const_iterator it = m_cacheTM.find(source);
  if(it != m_cacheTM.end()) {
    tpc.reset(new TargetPhraseCollection(*(boost::get<0>(it->second))));

    std::vector<const TargetPhrase*>::const_iterator it2 = tpc->begin();

    while (it2 != tpc->end()) {
      ((TargetPhrase*) *it2)->EvaluateInIsolation(source, GetFeaturesToApply());
      it2++;
    }
  }
  if (tpc)  {
    tpc->NthElement(m_tableLimit); // sort the phrases for the decoder
  }

  return tpc;
}

TargetPhraseCollection::shared_ptr  PhraseDictionaryDynamicCacheBased::GetTargetPhraseCollectionLEGACY(Phrase const &src) const
{
  TargetPhraseCollection::shared_ptr ret = GetTargetPhraseCollection(src);
  return ret;
}

TargetPhraseCollection::shared_ptr  PhraseDictionaryDynamicCacheBased::GetTargetPhraseCollectionNonCacheLEGACY(Phrase const &src) const
{
  TargetPhraseCollection::shared_ptr ret = GetTargetPhraseCollection(src);
  return ret;
}

ChartRuleLookupManager* PhraseDictionaryDynamicCacheBased::CreateRuleLookupManager(const ChartParser &parser, const ChartCellCollectionBase &cellCollection, std::size_t /*maxChartSpan*/)
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
  VERBOSE(2, "PhraseDictionaryDynamicCacheBased SetPreComputedScores:  " << m_maxAge << std::endl);
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
  m_lower_score = precomputedScores[m_maxAge].at(0);
  VERBOSE(3, "SetPreComputedScores(const unsigned int): lower_age:|" << m_maxAge << "| lower_score:|" << m_lower_score << "|" << std::endl);
}

Scores PhraseDictionaryDynamicCacheBased::GetPreComputedScores(const unsigned int age)
{
  if (age < m_maxAge) {
    return precomputedScores.at(age);
  } else {
    return precomputedScores.at(m_maxAge);
  }
}

void PhraseDictionaryDynamicCacheBased::ClearEntries(std::string &entries)
{
  if (entries != "") {
    VERBOSE(3,"entries:|" << entries << "|" << std::endl);
    std::vector<std::string> elements = TokenizeMultiCharSeparator(entries, "||||");
    VERBOSE(3,"elements.size() after:|" << elements.size() << "|" << std::endl);
    ClearEntries(elements);
  }
}

void PhraseDictionaryDynamicCacheBased::ClearEntries(std::vector<std::string> entries)
{
  VERBOSE(3,"PhraseDictionaryDynamicCacheBased::ClearEntries(std::vector<std::string> entries)" << std::endl);
  std::vector<std::string> pp;

  std::vector<std::string>::iterator it;
  for(it = entries.begin(); it!=entries.end(); it++) {
    pp.clear();
    pp = TokenizeMultiCharSeparator((*it), "|||");
    VERBOSE(3,"pp[0]:|" << pp[0] << "|" << std::endl);
    VERBOSE(3,"pp[1]:|" << pp[1] << "|" << std::endl);

    ClearEntries(pp[0], pp[1]);
  }
}

void PhraseDictionaryDynamicCacheBased::ClearEntries(std::string sourcePhraseString, std::string targetPhraseString)
{
  VERBOSE(3,"PhraseDictionaryDynamicCacheBased::ClearEntries(std::string sourcePhraseString, std::string targetPhraseString)" << std::endl);
  const StaticData &staticData = StaticData::Instance();
  Phrase sourcePhrase(0);
  Phrase targetPhrase(0);

  //target
  targetPhrase.Clear();
  VERBOSE(3, "targetPhraseString:|" << targetPhraseString << "|" << std::endl);
  targetPhrase.CreateFromString(Output, m_outputFactorsVec,
                                targetPhraseString, /*factorDelimiter,*/ NULL);
  VERBOSE(2, "targetPhrase:|" << targetPhrase << "|" << std::endl);

  //TODO: Would be better to reuse source phrases, but ownership has to be
  //consistent across phrase table implementations
  sourcePhrase.Clear();
  VERBOSE(3, "sourcePhraseString:|" << sourcePhraseString << "|" << std::endl);
  sourcePhrase.CreateFromString(Input, m_inputFactorsVec,
                                sourcePhraseString, /*factorDelimiter,*/ NULL);
  VERBOSE(3, "sourcePhrase:|" << sourcePhrase << "|" << std::endl);
  ClearEntries(sourcePhrase, targetPhrase);

}

void PhraseDictionaryDynamicCacheBased::ClearEntries(Phrase sp, Phrase tp)
{
  VERBOSE(3,"PhraseDictionaryDynamicCacheBased::ClearEntries(Phrase sp, Phrase tp)" << std::endl);
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  VERBOSE(3, "PhraseDictionaryCache deleting sp:|" << sp << "| tp:|" << tp << "|" << std::endl);

  cacheMap::const_iterator it = m_cacheTM.find(sp);
  VERBOSE(3,"sp:|" << sp << "|" << std::endl);
  if(it!=m_cacheTM.end()) {
    VERBOSE(3,"sp:|" << sp << "| FOUND" << std::endl);
    // sp is found
    // here we have to remove the target phrase from targetphrasecollection and from the TargetAgeMap
    // and then add new entry

    TargetCollectionPair TgtCollPair = it->second;
    TargetPhraseCollection::shared_ptr  tpc = boost::get<0>(TgtCollPair);
    AgeCollection* ac = boost::get<1>(TgtCollPair);
    Scores* sc = boost::get<2>(TgtCollPair);
    const Phrase* p_ptr = NULL;
    TargetPhrase* tp_ptr = NULL;
    bool found = false;
    size_t tp_pos=0;
    while (!found && tp_pos < tpc->GetSize()) {
      tp_ptr = (TargetPhrase*) tpc->GetTargetPhrase(tp_pos);
      p_ptr = (const Phrase*) tp_ptr;
      if (tp == *p_ptr) {
        found = true;
        continue;
      }
      tp_pos++;
    }
    if (!found) {
      VERBOSE(3,"tp:|" << tp << "| NOT FOUND" << std::endl);
      //do nothing
    } else {
      VERBOSE(3,"tp:|" << tp << "| FOUND" << std::endl);

      tpc->Remove(tp_pos); //delete entry in the Target Phrase Collection
      ac->erase(ac->begin() + tp_pos); //delete entry in the Age Collection
      // no need to delete scores here
      m_entries--;
      VERBOSE(3,"tpc size:|" << tpc->GetSize() << "|" << std::endl);
      VERBOSE(3,"ac size:|" << ac->size() << "|" << std::endl);
      VERBOSE(3,"sc size:|" << sc->size() << "|" << std::endl);
      VERBOSE(3,"tp:|" << tp << "| DELETED" << std::endl);
    }
    if (tpc->GetSize() == 0) {
      // delete the entry from m_cacheTM in case it points to an empty TargetPhraseCollection and AgeCollection
      ac->clear();
      sc->clear();
      tpc.reset();
      delete ac;
      delete sc;
      m_cacheTM.erase(sp);
    }

  } else {
    VERBOSE(3,"sp:|" << sp << "| NOT FOUND" << std::endl);
    //do nothing
  }
}




void PhraseDictionaryDynamicCacheBased::ClearSource(std::string &entries)
{
  if (entries != "") {
    VERBOSE(3,"entries:|" << entries << "|" << std::endl);
    std::vector<std::string> elements = TokenizeMultiCharSeparator(entries, "||||");
    VERBOSE(3,"elements.size() after:|" << elements.size() << "|" << std::endl);
    ClearEntries(elements);
  }
}

void PhraseDictionaryDynamicCacheBased::ClearSource(std::vector<std::string> entries)
{
  VERBOSE(3,"entries.size():|" << entries.size() << "|" << std::endl);
  const StaticData &staticData = StaticData::Instance();
  Phrase sourcePhrase(0);

  std::vector<std::string>::iterator it;
  for(it = entries.begin(); it!=entries.end(); it++) {

    sourcePhrase.Clear();
    VERBOSE(3, "sourcePhraseString:|" << (*it) << "|" << std::endl);
    sourcePhrase.CreateFromString(Input, m_inputFactorsVec,
                                  *it, /*factorDelimiter,*/ NULL);
    VERBOSE(3, "sourcePhrase:|" << sourcePhrase << "|" << std::endl);

    ClearSource(sourcePhrase);
  }

  IFVERBOSE(2) Print();
}

void PhraseDictionaryDynamicCacheBased::ClearSource(Phrase sp)
{
  VERBOSE(3,"void PhraseDictionaryDynamicCacheBased::ClearSource(Phrase sp) sp:|" << sp << "|" << std::endl);
  cacheMap::const_iterator it = m_cacheTM.find(sp);
  if (it != m_cacheTM.end()) {
    VERBOSE(3,"found:|" << sp << "|" << std::endl);
    //sp is found

    TargetCollectionPair TgtCollPair = it->second;
    TargetPhraseCollection::shared_ptr  tpc = boost::get<0>(TgtCollPair);
    AgeCollection* ac = boost::get<1>(TgtCollPair);
    Scores* sc = boost::get<2>(TgtCollPair);

    m_entries-=tpc->GetSize(); //reduce the total amount of entries of the cache

    // delete the entry from m_cacheTM in case it points to an empty TargetPhraseCollection and AgeCollection
    ac->clear();
    sc->clear();
    tpc.reset();
    delete ac;
    delete sc;
    m_cacheTM.erase(sp);
  } else {
    //do nothing
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
  if (m_constant == false) {
    Decay();
  }
  Update(entries, "1");
  IFVERBOSE(3) Print();
}


void PhraseDictionaryDynamicCacheBased::Update(std::vector<std::string> entries, std::string ageString)
{
  VERBOSE(3,"PhraseDictionaryDynamicCacheBased::Update(std::vector<std::string> entries, std::string ageString)" << std::endl);
  std::vector<std::string> pp;

  VERBOSE(3,"ageString:|" << ageString << "|" << std::endl);
  std::vector<std::string>::iterator it;
  for(it = entries.begin(); it!=entries.end(); it++) {
    pp.clear();
    pp = TokenizeMultiCharSeparator((*it), "|||");
    VERBOSE(3,"pp[0]:|" << pp[0] << "|" << std::endl);
    VERBOSE(3,"pp[1]:|" << pp[1] << "|" << std::endl);

    if (pp.size() > 3) {
      VERBOSE(3,"pp[2]:|" << pp[2] << "|" << std::endl);
      VERBOSE(3,"pp[3]:|" << pp[3] << "|" << std::endl);
      Update(pp[0], pp[1], ageString, pp[2], pp[3]);
    } else if (pp.size() > 2) {
      VERBOSE(3,"pp[2]:|" << pp[2] << "|" << std::endl);
      Update(pp[0], pp[1], ageString, pp[2]);
    } else {
      Update(pp[0], pp[1], ageString);
    }
  }
}

Scores PhraseDictionaryDynamicCacheBased::Conv2VecFloats(std::string& s)
{
  std::vector<float> n;
  if (s.empty())
    return n;
  std::istringstream iss(s);
  std::copy(std::istream_iterator<float>(iss),
            std::istream_iterator<float>(),
            std::back_inserter(n));
  return n;
}

void PhraseDictionaryDynamicCacheBased::Update(std::string sourcePhraseString, std::string targetPhraseString, std::string ageString, std::string scoreString, std::string waString)
{
  VERBOSE(3,"PhraseDictionaryDynamicCacheBased::Update(std::string sourcePhraseString, std::string targetPhraseString, std::string ageString, std::string waString)" << std::endl);
  const StaticData &staticData = StaticData::Instance();
  Phrase sourcePhrase(0);
  TargetPhrase targetPhrase(0);

  VERBOSE(3, "ageString:|" << ageString << "|" << std::endl);
  char *err_ind_temp;
  ageString = Trim(ageString);
  int age = strtod(ageString.c_str(), &err_ind_temp);
  VERBOSE(3, "age:|" << age << "|" << std::endl);
  Scores scores = Conv2VecFloats(scoreString);
  //target
  targetPhrase.Clear();
  // change here for factored based CBTM
  VERBOSE(3, "targetPhraseString:|" << targetPhraseString << "|" << std::endl);
  targetPhrase.CreateFromString(Output, m_outputFactorsVec,
                                targetPhraseString, /*factorDelimiter,*/ NULL);
  VERBOSE(3, "targetPhrase:|" << targetPhrase << "|" << std::endl);

  //TODO: Would be better to reuse source phrases, but ownership has to be
  //consistent across phrase table implementations
  sourcePhrase.Clear();
  VERBOSE(3, "sourcePhraseString:|" << sourcePhraseString << "|" << std::endl);
  sourcePhrase.CreateFromString(Input, m_inputFactorsVec, sourcePhraseString, /*factorDelimiter,*/ NULL);
  VERBOSE(3, "sourcePhrase:|" << sourcePhrase << "|" << std::endl);

  if (!waString.empty()) VERBOSE(3, "waString:|" << waString << "|" << std::endl);

  Update(sourcePhrase, targetPhrase, age, scores, waString);
}

void PhraseDictionaryDynamicCacheBased::Update(Phrase sp, TargetPhrase tp, int age, Scores scores, std::string waString)
{
  VERBOSE(3,"PhraseDictionaryDynamicCacheBased::Update(Phrase sp, TargetPhrase tp, int age, std::string waString)" << std::endl);
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  VERBOSE(3, "PhraseDictionaryCache inserting sp:|" << sp << "| tp:|" << tp << "| age:|" << age << "| word-alignment |" << waString << "|" << std::endl);

  cacheMap::const_iterator it = m_cacheTM.find(sp);
  VERBOSE(3,"sp:|" << sp << "|" << std::endl);
  if(it!=m_cacheTM.end()) {
    VERBOSE(3,"sp:|" << sp << "| FOUND" << std::endl);
    // sp is found
    // here we have to remove the target phrase from targetphrasecollection and from the TargetAgeMap
    // and then add new entry

    TargetCollectionPair TgtCollPair = it->second;
    TargetPhraseCollection::shared_ptr  tpc = boost::get<0>(TgtCollPair);
    AgeCollection* ac = boost::get<1>(TgtCollPair);
    Scores* sc = boost::get<2>(TgtCollPair);
//    const TargetPhrase* p_ptr = NULL;
    const Phrase* p_ptr = NULL;
    TargetPhrase* tp_ptr = NULL;
    bool found = false;
    size_t tp_pos=0;
    while (!found && tp_pos < tpc->GetSize()) {
      tp_ptr = (TargetPhrase*) tpc->GetTargetPhrase(tp_pos);
      p_ptr = (const TargetPhrase*) tp_ptr;
      if ((Phrase) tp == *p_ptr) {
        found = true;
        continue;
      }
      tp_pos++;
    }
    if (!found) {
      VERBOSE(3,"tp:|" << tp << "| NOT FOUND" << std::endl);
      std::auto_ptr<TargetPhrase> targetPhrase(new TargetPhrase(tp));
      Scores scoreVec;
      scoreVec.push_back(GetPreComputedScores(age)[0]);
      for (unsigned int i=0; i<scores.size(); i++) {
        scoreVec.push_back(scores[i]);
      }
      if(scoreVec.size() != m_numScoreComponents) {
        VERBOSE(1, "Scores does not match number of score components for phrase : "<< sp.ToString() <<" ||| " << tp.ToString() <<endl);
        VERBOSE(1, "Debugging: Press Enter to continue..." <<endl);
        std::cin.ignore();
      }
      targetPhrase->GetScoreBreakdown().Assign(this, scoreVec);
//      targetPhrase->GetScoreBreakdown().Assign(this, GetPreComputedScores(age));
      if (!waString.empty()) targetPhrase->SetAlignmentInfo(waString);

      tpc->Add(targetPhrase.release());

      tp_pos = tpc->GetSize()-1;
      ac->push_back(age);
      sc = &scores;
      m_entries++;
      VERBOSE(3,"sp:|" << sp << "tp:|" << tp << "| INSERTED" << std::endl);
    } else {
      Scores scoreVec;
      scoreVec.push_back(GetPreComputedScores(age)[0]);
      for (unsigned int i=0; i<scores.size(); i++) {
        scoreVec.push_back(scores[i]);
      }
      if(scoreVec.size() != m_numScoreComponents) {
        VERBOSE(1, "Scores does not match number of score components for phrase : "<< sp.ToString() <<" ||| " << tp.ToString() <<endl);
        VERBOSE(1, "Debugging: Press Enter to continue..." <<endl);
        std::cin.ignore();
      }
      tp_ptr->GetScoreBreakdown().Assign(this, scoreVec);
//      tp_ptr->GetScoreBreakdown().Assign(this, GetPreComputedScores(age));
      if (!waString.empty()) tp_ptr->SetAlignmentInfo(waString);
      ac->at(tp_pos) = age;
      VERBOSE(3,"sp:|" << sp << "tp:|" << tp << "| UPDATED" << std::endl);
    }
  } else {
    VERBOSE(3,"sp:|" << sp << "| NOT FOUND" << std::endl);
    // p is not found
    // create target collection
    // we have to create new target collection age pair and add new entry to target collection age pair

    TargetPhraseCollection::shared_ptr tpc(new TargetPhraseCollection);
    AgeCollection* ac = new AgeCollection();
    Scores* sc = new Scores();
    m_cacheTM.insert(make_pair(sp,boost::make_tuple(tpc,ac,sc)));

    //tp is not found
    std::auto_ptr<TargetPhrase> targetPhrase(new TargetPhrase(tp));
    // scoreVec is a composition of decay_score and the feature scores
    Scores scoreVec;
    scoreVec.push_back(GetPreComputedScores(age)[0]);
    for (unsigned int i=0; i<scores.size(); i++) {
      scoreVec.push_back(scores[i]);
    }
    if(scoreVec.size() != m_numScoreComponents) {
      VERBOSE(1, "Scores do not match number of score components for phrase : "<< sp <<" ||| " << tp <<endl);
      VERBOSE(1, "Debugging: Press Enter to continue..." <<endl);
      std::cin.ignore();
    }
    targetPhrase->GetScoreBreakdown().Assign(this, scoreVec);
    if (!waString.empty()) targetPhrase->SetAlignmentInfo(waString);

    tpc->Add(targetPhrase.release());
    ac->push_back(age);
    sc = &scores;
    m_entries++;
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

void PhraseDictionaryDynamicCacheBased::Decay(Phrase sp)
{
  VERBOSE(3,"void PhraseDictionaryDynamicCacheBased::Decay(Phrase sp) sp:|" << sp << "|" << std::endl);
  cacheMap::iterator it = m_cacheTM.find(sp);
  if (it != m_cacheTM.end()) {
    VERBOSE(3,"found:|" << sp << "|" << std::endl);
    //sp is found

    TargetCollectionPair TgtCollPair = it->second;
    TargetPhraseCollection::shared_ptr  tpc = boost::get<0>(TgtCollPair);
    AgeCollection* ac = boost::get<1>(TgtCollPair);
    Scores* sc = boost::get<2>(TgtCollPair);

    //loop in inverted order to allow a correct deletion of std::vectors tpc and ac
    for (int tp_pos = tpc->GetSize() - 1 ; tp_pos >= 0; tp_pos--) {
      unsigned int tp_age = ac->at(tp_pos); //increase the age by 1
      tp_age++; //increase the age by 1
      VERBOSE(3,"sp:|" << sp << "| " << " new tp_age:|" << tp_age << "|" << std::endl);

      TargetPhrase* tp_ptr = (TargetPhrase*) tpc->GetTargetPhrase(tp_pos);

      if (tp_age > m_maxAge) {
        VERBOSE(3,"tp_age:|" << tp_age << "| TOO BIG" << std::endl);
        tpc->Remove(tp_pos); //delete entry in the Target Phrase Collection
        ac->erase(ac->begin() + tp_pos); //delete entry in the Age Collection
        // no need to change scores here
        m_entries--;
      } else {
        VERBOSE(3,"tp_age:|" << tp_age << "| STILL GOOD" << std::endl);
        // scoreVec is a composition of decay_score and the feature scores
        size_t idx=0;
        tp_ptr->GetScoreBreakdown().Assign(this, idx, GetPreComputedScores(tp_age)[0]);
//        tp_ptr->GetScoreBreakdown().Assign(this, GetPreComputedScores(tp_age));
        ac->at(tp_pos) = tp_age;
      }
    }
    if (tpc->GetSize() == 0) {
      // delete the entry from m_cacheTM in case it points to an empty TargetPhraseCollection and AgeCollection
      // clear age collection
      ac->clear();
      // clear score collection
      sc->clear();
      // delete age collection
      delete ac;
      // delete score collection
      delete sc;
      // reset the target phrase collectio
      tpc.reset();
      m_cacheTM.erase(sp);
    }
  } else {
    //do nothing
    VERBOSE(3,"sp:|" << sp << "| NOT FOUND" << std::endl);
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
  cacheMap::iterator it;
  for(it = m_cacheTM.begin(); it!=m_cacheTM.end(); it++) {
    // clear age collection
    (boost::get<1>((*it).second))->clear();
    // clear score collection
    (boost::get<2>((*it).second))->clear();
    // delete age collection
    delete boost::get<1>((*it).second);
    // delete score collection
    delete boost::get<2>((*it).second);
    // reset the target phrase collection
    (boost::get<0>(it->second)).reset();
  }
  m_cacheTM.clear();
  m_entries = 0;
}


void PhraseDictionaryDynamicCacheBased::ExecuteDlt(std::map<std::string, std::string> dlt_meta)
{
  if (dlt_meta.find("cbtm") != dlt_meta.end()) {
    Insert(dlt_meta["cbtm"]);
  }
  if (dlt_meta.find("cbtm-command") != dlt_meta.end()) {
    Execute(dlt_meta["cbtm-command"]);
  }
  if (dlt_meta.find("cbtm-file") != dlt_meta.end()) {
    Load(dlt_meta["cbtm-file"]);
  }
  if (dlt_meta.find("cbtm-clear-source") != dlt_meta.end()) {
    ClearSource(dlt_meta["cbtm-clear-source"]);
  }
  if (dlt_meta.find("cbtm-clear-entries") != dlt_meta.end()) {
    ClearEntries(dlt_meta["cbtm-clear-entries"]);
  }
  if (dlt_meta.find("cbtm-clear-all") != dlt_meta.end()) {
    Clear();
  }

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
    TargetPhraseCollection::shared_ptr  tpc = boost::get<0>(it->second);
    TargetPhraseCollection::iterator itr;
    for(itr = tpc->begin(); itr != tpc->end(); itr++) {
      std::string target = (*itr)->ToString();
      std::cout << source << " ||| " << target << std::endl;
    }
    source.clear();
  }
}

}// end namespace
