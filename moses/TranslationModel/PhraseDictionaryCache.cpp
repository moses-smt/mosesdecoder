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
#include "moses/TranslationModel/PhraseDictionaryCache.h"
#include "moses/FactorCollection.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "moses/TargetPhrase.h"


using namespace std;

namespace Moses
{
std::map< const std::string, PhraseDictionaryCache * > PhraseDictionaryCache::s_instance_map;
PhraseDictionaryCache *PhraseDictionaryCache::s_instance = NULL;

//! contructor
PhraseDictionaryCache::PhraseDictionaryCache(const std::string &line)
  : PhraseDictionary(line, true)
{
  std::cerr << "Initializing PhraseDictionaryCache feature..." << std::endl;

  //disabling internal cache (provided by PhraseDictionary) for translation options (third parameter set to 0)
  m_maxCacheSize = 0;

  m_entries = 0;
  m_name = "default";
  m_constant = false;

  ReadParameters();

  UTIL_THROW_IF2(s_instance_map.find(m_name) != s_instance_map.end(), "Only 1 PhraseDictionaryCache feature named " + m_name + " is allowed");
  s_instance_map[m_name] = this;
  s_instance = this; //for back compatibility
  vector<float> weight = StaticData::Instance().GetWeights(this);
  m_numscorecomponent = weight.size();
  m_sentences=0;
}

PhraseDictionaryCache::~PhraseDictionaryCache()
{
  Clear();
}

void PhraseDictionaryCache::SetParameter(const std::string& key, const std::string& value)
{
  VERBOSE(2, "PhraseDictionaryCache::SetParameter key:|" << key << "| value:|" << value << "|" << std::endl);

  if (key == "cache-name") {
    m_name = Scan<std::string>(value);
  } else if (key == "input-factor") {
    m_inputFactorsVec = Tokenize<FactorType>(value,",");
  } else if (key == "output-factor") {
    m_outputFactorsVec = Tokenize<FactorType>(value,",");
  } else {
    PhraseDictionary::SetParameter(key, value);
  }
}

void PhraseDictionaryCache::CleanUpAfterSentenceProcessing(const InputType& source)
{
  Clear(source.GetTranslationId());
}

void PhraseDictionaryCache::InitializeForInput(ttasksptr const& ttask)
{
#ifdef WITH_THREADS
  boost::unique_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  long tID = ttask->GetSource()->GetTranslationId();
  TargetPhraseCollection::shared_ptr tpc;
  if (m_cacheTM.find(tID) == m_cacheTM.end()) return;
  for(cacheMap::const_iterator it=m_cacheTM.at(tID).begin();  it != m_cacheTM.at(tID).end(); it++) {
    tpc.reset(new TargetPhraseCollection(*(it->second).first));
    std::vector<const TargetPhrase*>::const_iterator it2 = tpc->begin();

    while (it2 != tpc->end()) {
      ((TargetPhrase*) *it2)->EvaluateInIsolation(it->first, GetFeaturesToApply());
      it2++;
    }
  }
  if (tpc)  {
    tpc->NthElement(m_tableLimit); // sort the phrases for the decoder
  }
}

void PhraseDictionaryCache::GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
  InputPathList::const_iterator iter;
  for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter) {
    InputPath &inputPath = **iter;
    long tID = inputPath.ttask->GetSource()->GetTranslationId();
    if (m_cacheTM.find(tID) == m_cacheTM.end()) continue;
    const Phrase &source = inputPath.GetPhrase();
    TargetPhraseCollection::shared_ptr tpc;
    for(cacheMap::const_iterator it=m_cacheTM.at(tID).begin();  it != m_cacheTM.at(tID).end(); it++) {
      if (source.Compare(it->first)!=0) continue;
      tpc.reset(new TargetPhraseCollection(*(it->second).first));
      inputPath.SetTargetPhrases(*this, tpc, NULL);
    }
  }
}

TargetPhraseCollection::shared_ptr PhraseDictionaryCache::GetTargetPhraseCollection(const Phrase &source, long tID) const
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
  TargetPhraseCollection::shared_ptr tpc;

  if(m_cacheTM.find(tID) == m_cacheTM.end()) return tpc;

  cacheMap::const_iterator it = m_cacheTM.at(tID).find(source);
  if(it != m_cacheTM.at(tID).end()) {
    tpc.reset(new TargetPhraseCollection(*(it->second).first));

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

ChartRuleLookupManager* PhraseDictionaryCache::CreateRuleLookupManager(const ChartParser &parser, const ChartCellCollectionBase &cellCollection, std::size_t /*maxChartSpan*/)
{
  UTIL_THROW(util::Exception, "Not implemented for Chart Decoder");
}

// friend
ostream& operator<<(ostream& out, const PhraseDictionaryCache& phraseDict)
{
  return out;
}

void PhraseDictionaryCache::Insert(std::string &entries, long tID)
{
  if (entries != "") {
    VERBOSE(3,"entries:|" << entries << "|" << " tID | " << tID << std::endl);
    std::vector<std::string> elements = TokenizeMultiCharSeparator(entries, "||||");
    VERBOSE(3,"elements.size() after:|" << elements.size() << "|" << std::endl);
    Insert(elements, tID);
  }
}

void PhraseDictionaryCache::Insert(std::vector<std::string> entries, long tID)
{
  VERBOSE(3,"entries.size():|" << entries.size() << "|" << std::endl);
  Update(tID, entries);
  IFVERBOSE(3) Print();
}


void PhraseDictionaryCache::Update(long tID, std::vector<std::string> entries)
{
  std::vector<std::string> pp;

  std::vector<std::string>::iterator it;
  for(it = entries.begin(); it!=entries.end(); it++) {
    pp.clear();
    pp = TokenizeMultiCharSeparator((*it), "|||");
    VERBOSE(3,"pp[0]:|" << pp[0] << "|" << std::endl);
    VERBOSE(3,"pp[1]:|" << pp[1] << "|" << std::endl);

    if (pp.size() > 3) {
      VERBOSE(3,"pp[2]:|" << pp[2] << "|" << std::endl);
      VERBOSE(3,"pp[3]:|" << pp[3] << "|" << std::endl);
      Update(tID,pp[0], pp[1], pp[2], pp[3]);
    } else if (pp.size() > 2) {
      VERBOSE(3,"pp[2]:|" << pp[2] << "|" << std::endl);
      Update(tID,pp[0], pp[1], pp[2]);
    } else {
      Update(tID,pp[0], pp[1]);
    }
  }
}

Scores PhraseDictionaryCache::Conv2VecFloats(std::string& s)
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

void PhraseDictionaryCache::Update(long tID, std::string sourcePhraseString, std::string targetPhraseString, std::string scoreString, std::string waString)
{
  const StaticData &staticData = StaticData::Instance();
  Phrase sourcePhrase(0);
  TargetPhrase targetPhrase(0);

  char *err_ind_temp;
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

  Update(tID, sourcePhrase, targetPhrase, scores, waString);
}

void PhraseDictionaryCache::Update(long tID, Phrase sp, TargetPhrase tp, Scores scores, std::string waString)
{
  VERBOSE(3,"PhraseDictionaryCache::Update(Phrase sp, TargetPhrase tp, Scores scores, std::string waString)" << std::endl);
#ifdef WITH_THREADS
  boost::unique_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  VERBOSE(3, "PhraseDictionaryCache inserting sp:|" << sp << "| tp:|" << tp << "| word-alignment |" << waString << "|" << std::endl);
  // if there is no cache for the sentence tID, create one.
  cacheMap::const_iterator it = m_cacheTM[tID].find(sp);
  VERBOSE(3,"sp:|" << sp << "|" << std::endl);
  if(it!=m_cacheTM.at(tID).end()) {
    VERBOSE(3,"sp:|" << sp << "| FOUND" << std::endl);
    // sp is found

    TargetCollectionPair TgtCollPair = it->second;
    TargetPhraseCollection::shared_ptr  tpc = TgtCollPair.first;
    Scores* sc = TgtCollPair.second;
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
      for (unsigned int i=0; i<scores.size(); i++) {
        scoreVec.push_back(scores[i]);
      }
      if(scoreVec.size() != m_numScoreComponents) {
        VERBOSE(1, "Scores does not match number of score components for phrase : "<< sp.ToString() <<" ||| " << tp.ToString() <<endl);
        VERBOSE(1, "I am ignoring this..." <<endl);
//    	  std::cin.ignore();
      }
      targetPhrase->GetScoreBreakdown().Assign(this, scoreVec);
      if (!waString.empty()) targetPhrase->SetAlignmentInfo(waString);

      tpc->Add(targetPhrase.release());

      tp_pos = tpc->GetSize()-1;
      sc = &scores;
      m_entries++;
      VERBOSE(3,"sp:|" << sp << "tp:|" << tp << "| INSERTED" << std::endl);
    } else {
      Scores scoreVec;
      for (unsigned int i=0; i<scores.size(); i++) {
        scoreVec.push_back(scores[i]);
      }
      if(scoreVec.size() != m_numScoreComponents) {
        VERBOSE(1, "Scores does not match number of score components for phrase : "<< sp.ToString() <<" ||| " << tp.ToString() <<endl);
        VERBOSE(1, "I am ignoring this..." <<endl);
//		std::cin.ignore();
      }
      tp_ptr->GetScoreBreakdown().Assign(this, scoreVec);
      if (!waString.empty()) tp_ptr->SetAlignmentInfo(waString);
      VERBOSE(3,"sp:|" << sp << "tp:|" << tp << "| UPDATED" << std::endl);
    }
  } else {
    VERBOSE(3,"sp:|" << sp << "| NOT FOUND" << std::endl);
    // p is not found
    // create target collection

    TargetPhraseCollection::shared_ptr tpc(new TargetPhraseCollection);
    Scores* sc = new Scores();
    m_cacheTM[tID].insert(make_pair(sp,std::make_pair(tpc,sc)));

    //tp is not found
    std::auto_ptr<TargetPhrase> targetPhrase(new TargetPhrase(tp));
    // scoreVec is a composition of decay_score and the feature scores
    Scores scoreVec;
    for (unsigned int i=0; i<scores.size(); i++) {
      scoreVec.push_back(scores[i]);
    }
    if(scoreVec.size() != m_numScoreComponents) {
      VERBOSE(1, "Scores do not match number of score components for phrase : "<< sp <<" ||| " << tp <<endl);
      VERBOSE(1, "I am ignoring this..." <<endl);
//    	std::cin.ignore();
    }
    targetPhrase->GetScoreBreakdown().Assign(this, scoreVec);
    if (!waString.empty()) targetPhrase->SetAlignmentInfo(waString);

    tpc->Add(targetPhrase.release());
    sc = &scores;
    m_entries++;
    VERBOSE(3,"sp:|" << sp << "| tp:|" << tp << "| INSERTED" << std::endl);
  }
}

void PhraseDictionaryCache::Execute(std::string command, long tID)
{
  VERBOSE(2,"command:|" << command << "|" << std::endl);
  std::vector<std::string> commands = Tokenize(command, "||");
  Execute(commands, tID);
}

void PhraseDictionaryCache::Execute(std::vector<std::string> commands, long tID)
{
  for (size_t j=0; j<commands.size(); j++) {
    Execute_Single_Command(commands[j]);
  }
  IFVERBOSE(2) Print();
}

void PhraseDictionaryCache::Execute_Single_Command(std::string command)
{
  if (command == "clear") {
    VERBOSE(2,"PhraseDictionaryCache Execute command:|"<< command << "|. Cache cleared." << std::endl);
    Clear();
  } else {
    VERBOSE(2,"PhraseDictionaryCache Execute command:|"<< command << "| is unknown. Skipped." << std::endl);
  }
}

void PhraseDictionaryCache::Clear()
{
  for(sentCacheMap::iterator it=m_cacheTM.begin(); it!=m_cacheTM.end(); it++) {
    Clear(it->first);
  }
}

void PhraseDictionaryCache::Clear(long tID)
{
#ifdef WITH_THREADS
  boost::unique_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  if (m_cacheTM.find(tID) == m_cacheTM.end()) return;
  cacheMap::iterator it;
  for(it = m_cacheTM.at(tID).begin(); it!=m_cacheTM.at(tID).end(); it++) {
    (((*it).second).second)->clear();
    delete ((*it).second).second;
    ((*it).second).first.reset();
  }
  m_cacheTM.at(tID).clear();
  m_entries = 0;
}


void PhraseDictionaryCache::ExecuteDlt(std::map<std::string, std::string> dlt_meta, long tID)
{
  if (dlt_meta.find("cbtm") != dlt_meta.end()) {
    Insert(dlt_meta["cbtm"], tID);
  }
  if (dlt_meta.find("cbtm-command") != dlt_meta.end()) {
    Execute(dlt_meta["cbtm-command"], tID);
  }
  if (dlt_meta.find("cbtm-clear-all") != dlt_meta.end()) {
    Clear();
  }
}

void PhraseDictionaryCache::Print() const
{
  VERBOSE(2,"PhraseDictionaryCache::Print()" << std::endl);
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
  for(sentCacheMap::const_iterator itr = m_cacheTM.begin(); itr!=m_cacheTM.end(); itr++) {
    cacheMap::const_iterator it;
    for(it = (itr->second).begin(); it!=(itr->second).end(); it++) {
      std::string source = (it->first).ToString();
      TargetPhraseCollection::shared_ptr  tpc = (it->second).first;
      TargetPhraseCollection::iterator itr;
      for(itr = tpc->begin(); itr != tpc->end(); itr++) {
        std::string target = (*itr)->ToString();
        std::cout << source << " ||| " << target << std::endl;
      }
      source.clear();
    }
  }
}

}// end namespace
