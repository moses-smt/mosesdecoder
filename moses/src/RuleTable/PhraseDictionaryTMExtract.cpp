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

#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include "RuleTable/Loader.h"
#include "RuleTable/LoaderFactory.h"
#include "PhraseDictionaryTMExtract.h"
#include "FactorCollection.h"
#include "Word.h"
#include "Util.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "WordsRange.h"
#include "UserMessage.h"
#include "CYKPlusParser/ChartRuleLookupManagerMemoryPerSentence.h"

using namespace std;

namespace Moses
{
  PhraseDictionaryTMExtract::PhraseDictionaryTMExtract(size_t numScoreComponents,
                            PhraseDictionaryFeature* feature)
  : PhraseDictionary(numScoreComponents, feature) 
  {
    const StaticData &staticData = StaticData::Instance();
    CHECK(staticData.ThreadCount() == 1);
  }

  void PhraseDictionaryTMExtract::Initialize(const string &initStr)
  {
    cerr << "initStr=" << initStr << endl;
    m_config = Tokenize(initStr, ";");
    assert(m_config.size() == 4);
    
    
  }
  
  TargetPhraseCollection &PhraseDictionaryTMExtract::GetOrCreateTargetPhraseCollection(const InputType &inputSentence
                                                                                  , const Phrase &source
                                                                                  , const TargetPhrase &target
                                                                                  , const Word &sourceLHS)
  {
    PhraseDictionaryNodeSCFG &currNode = GetOrCreateNode(inputSentence, source, target, sourceLHS);
    return currNode.GetOrCreateTargetPhraseCollection();
  }
  
  PhraseDictionaryNodeSCFG &PhraseDictionaryTMExtract::GetOrCreateNode(const InputType &inputSentence
                                                                  , const Phrase &source
                                                                  , const TargetPhrase &target
                                                                  , const Word &sourceLHS)
  {
    const size_t size = source.GetSize();
    
    const AlignmentInfo &alignmentInfo = target.GetAlignmentInfo();
    AlignmentInfo::const_iterator iterAlign = alignmentInfo.begin();
    
    PhraseDictionaryNodeSCFG *currNode = &GetRootNode(inputSentence);
    for (size_t pos = 0 ; pos < size ; ++pos) {
      const Word& word = source.GetWord(pos);
      
      if (word.IsNonTerminal()) {
        // indexed by source label 1st
        const Word &sourceNonTerm = word;
        
        CHECK(iterAlign != target.GetAlignmentInfo().end());
        CHECK(iterAlign->first == pos);
        size_t targetNonTermInd = iterAlign->second;
        ++iterAlign;
        const Word &targetNonTerm = target.GetWord(targetNonTermInd);
        
        currNode = currNode->GetOrCreateChild(sourceNonTerm, targetNonTerm);
      } else {
        currNode = currNode->GetOrCreateChild(word);
      }
      
      CHECK(currNode != NULL);
    }
    
    // finally, the source LHS
    //currNode = currNode->GetOrCreateChild(sourceLHS);
    //CHECK(currNode != NULL);
    
    
    return *currNode;
  }
  
  ChartRuleLookupManager *PhraseDictionaryTMExtract::CreateRuleLookupManager(
                                                                        const InputType &sentence,
                                                                        const ChartCellCollection &cellCollection)
  {
    return new ChartRuleLookupManagerMemoryPerSentence(sentence, cellCollection, *this);
  }
  
  void PhraseDictionaryTMExtract::SortAndPrune(const InputType &source)
  {
    if (GetTableLimit())
    {
      long transId = source.GetTranslationId();
      std::map<long, PhraseDictionaryNodeSCFG>::iterator iter = m_collection.find(transId);
      CHECK(iter != m_collection.end());

      (iter->second).Sort(GetTableLimit());
    }
  }
  
  void PhraseDictionaryTMExtract::InitializeForInput(InputType const& source)
  {
    string data_root = "/tmp";
    string in_file = data_root + "/in";
    string pt_file = data_root + "/pt.out";
    
    ofstream inFile(in_file.c_str());
    
    for (size_t i = 1; i < source.GetSize() - 1; ++i)
    {
      inFile << source.GetWord(i) << " ";
    }
    inFile << endl;
    inFile.close();
    
    string cmd = "perl ~/workspace/github/hieuhoang/contrib/tm-mt-integration/make-pt-from-tm.perl "
              + in_file + " "
              + m_config[0] + " "
              + m_config[1] + " "
              + m_config[2] + " "
              + m_config[3] + " "
              + pt_file;
    system(cmd.c_str());    
    
    cerr << "done\n";
    
    m_collection[source.GetTranslationId()];
    
  }
  
  void PhraseDictionaryTMExtract::CleanUp(const InputType &source)
  {
    m_collection.erase(source.GetTranslationId());
  }

  const PhraseDictionaryNodeSCFG &PhraseDictionaryTMExtract::GetRootNode(const InputType &source) const 
  {
    long transId = source.GetTranslationId();
    std::map<long, PhraseDictionaryNodeSCFG>::const_iterator iter = m_collection.find(transId);
    CHECK(iter != m_collection.end());
    return iter->second; 
  }
  PhraseDictionaryNodeSCFG &PhraseDictionaryTMExtract::GetRootNode(const InputType &source) 
  {
    long transId = source.GetTranslationId();
    std::map<long, PhraseDictionaryNodeSCFG>::iterator iter = m_collection.find(transId);
    CHECK(iter != m_collection.end());
    return iter->second; 
  }


  TO_STRING_BODY(PhraseDictionaryTMExtract);
  
  // friend
  ostream& operator<<(ostream& out, const PhraseDictionaryTMExtract& phraseDict)
  {
    typedef PhraseDictionaryNodeSCFG::TerminalMap TermMap;
    typedef PhraseDictionaryNodeSCFG::NonTerminalMap NonTermMap;
    
    /*
    const PhraseDictionaryNodeSCFG &coll = phraseDict.m_collection;
    for (NonTermMap::const_iterator p = coll.m_nonTermMap.begin(); p != coll.m_nonTermMap.end(); ++p) {
      const Word &sourceNonTerm = p->first.first;
      out << sourceNonTerm;
    }
    for (TermMap::const_iterator p = coll.m_sourceTermMap.begin(); p != coll.m_sourceTermMap.end(); ++p) {
      const Word &sourceTerm = p->first;
      out << sourceTerm;
    }
     */
    
    return out;
  }
  
}
