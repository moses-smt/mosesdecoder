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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include "Loader.h"
#include "LoaderFactory.h"
#include "PhraseDictionaryFuzzyMatch.h"
#include "moses/FactorCollection.h"
#include "moses/Word.h"
#include "moses/Util.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "moses/WordsRange.h"
#include "moses/UserMessage.h"
#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerMemoryPerSentence.h"
#include "moses/TranslationModel/fuzzy-match/FuzzyMatchWrapper.h"
#include "moses/TranslationModel/fuzzy-match/SentenceAlignment.h"
#include "util/file.hh"
#include "util/exception.hh"

using namespace std;

namespace Moses
{

PhraseDictionaryFuzzyMatch::PhraseDictionaryFuzzyMatch(const std::string &line)
  :PhraseDictionary(line)
  ,m_FuzzyMatchWrapper(NULL)
{
  ReadParameters();
}

PhraseDictionaryFuzzyMatch::~PhraseDictionaryFuzzyMatch()
{
  delete m_FuzzyMatchWrapper;
}

void PhraseDictionaryFuzzyMatch::Load()
{
  SetFeaturesToApply();

  assert(m_config.size() == 3);
  m_FuzzyMatchWrapper = new tmmt::FuzzyMatchWrapper(m_config[0], m_config[1], m_config[2]);
}

ChartRuleLookupManager *PhraseDictionaryFuzzyMatch::CreateRuleLookupManager(
  const ChartParser &parser,
  const ChartCellCollectionBase &cellCollection)
{
  return new ChartRuleLookupManagerMemoryPerSentence(parser, cellCollection, *this);
}


int removedirectoryrecursively(const char *dirname)
{
  DIR *dir;
  struct dirent *entry;
  char path[PATH_MAX];

  if (path == NULL) {
    fprintf(stderr, "Out of memory error\n");
    return 0;
  }
  dir = opendir(dirname);
  if (dir == NULL) {
    perror("Error opendir()");
    return 0;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
      snprintf(path, (size_t) PATH_MAX, "%s/%s", dirname, entry->d_name);
      if (entry->d_type == DT_DIR) {
        removedirectoryrecursively(path);
      }

      remove(path);
      /*
       * Here, the actual deletion must be done.  Beacuse this is
       * quite a dangerous thing to do, and this program is not very
       * well tested, we are just printing as if we are deleting.
       */
      //printf("(not really) Deleting: %s\n", path);
      /*
       * When you are finished testing this and feel you are ready to do the real
       * deleting, use this: remove*STUB*(path);
       * (see "man 3 remove")
       * Please note that I DONT TAKE RESPONSIBILITY for data you delete with this!
       */
    }

  }
  closedir(dir);

  rmdir(dirname);
  /*
   * Now the directory is emtpy, finally delete the directory itself. (Just
   * printing here, see above)
   */
  //printf("(not really) Deleting: %s\n", dirname);

  return 1;
}

void PhraseDictionaryFuzzyMatch::InitializeForInput(InputType const& inputSentence)
{
  char dirName[] = "/tmp/moses.XXXXXX";
  char *temp = mkdtemp(dirName);
  UTIL_THROW_IF2(temp == NULL,
		  "Couldn't create temporary directory " << dirName);

  string dirNameStr(dirName);

  string inFileName(dirNameStr + "/in");

  ofstream inFile(inFileName.c_str());

  for (size_t i = 1; i < inputSentence.GetSize() - 1; ++i) {
    inFile << inputSentence.GetWord(i);
  }
  inFile << endl;
  inFile.close();

  long translationId = inputSentence.GetTranslationId();
  string ptFileName = m_FuzzyMatchWrapper->Extract(translationId, dirNameStr);

  // populate with rules for this sentence
  PhraseDictionaryNodeMemory &rootNode = m_collection[translationId];
  FormatType format = MosesFormat;

  // data from file
  InputFileStream inStream(ptFileName);

  // copied from class LoaderStandard
  PrintUserTime("Start loading fuzzy-match phrase model");

  const StaticData &staticData = StaticData::Instance();
  const std::string& factorDelimiter = staticData.GetFactorDelimiter();


  string lineOrig;
  size_t count = 0;

  while(getline(inStream, lineOrig)) {
    const string *line;
    if (format == HieroFormat) { // reformat line
      UTIL_THROW(util::Exception, "Cannot be Hiero format");
      //line = ReformatHieroRule(lineOrig);
    } else {
      // do nothing to format of line
      line = &lineOrig;
    }

    vector<string> tokens;
    vector<float> scoreVector;

    TokenizeMultiCharSeparator(tokens, *line , "|||" );

    if (tokens.size() != 4 && tokens.size() != 5) {
      stringstream strme;
      strme << "Syntax error at " << ptFileName << ":" << count;
      UserMessage::Add(strme.str());
      abort();
    }

    const string &sourcePhraseString = tokens[0]
                                       , &targetPhraseString = tokens[1]
                                           , &scoreString        = tokens[2]
                                               , &alignString        = tokens[3];

    bool isLHSEmpty = (sourcePhraseString.find_first_not_of(" \t", 0) == string::npos);
    if (isLHSEmpty && !staticData.IsWordDeletionEnabled()) {
      TRACE_ERR( ptFileName << ":" << count << ": pt entry contains empty target, skipping\n");
      continue;
    }

    Tokenize<float>(scoreVector, scoreString);
    const size_t numScoreComponents = GetNumScoreComponents();
    if (scoreVector.size() != numScoreComponents) {
      stringstream strme;
      strme << "Size of scoreVector != number (" << scoreVector.size() << "!="
            << numScoreComponents << ") of score components on line " << count;
      UserMessage::Add(strme.str());
      abort();
    }

    UTIL_THROW_IF2(scoreVector.size() != numScoreComponents,
    		"Number of scores incorrectly specified");

    // parse source & find pt node

    // constituent labels
    Word *sourceLHS;
    Word *targetLHS;

    // source
    Phrase sourcePhrase( 0);
    sourcePhrase.CreateFromString(Input, m_input, sourcePhraseString, factorDelimiter, &sourceLHS);

    // create target phrase obj
    TargetPhrase *targetPhrase = new TargetPhrase();
    targetPhrase->CreateFromString(Output, m_output, targetPhraseString, factorDelimiter, &targetLHS);

    // rest of target phrase
    targetPhrase->SetAlignmentInfo(alignString);
    targetPhrase->SetTargetLHS(targetLHS);
    //targetPhrase->SetDebugOutput(string("New Format pt ") + line);

    // component score, for n-best output
    std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),TransformScore);
    std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),FloorScore);

    targetPhrase->GetScoreBreakdown().Assign(this, scoreVector);
    targetPhrase->Evaluate(sourcePhrase, GetFeaturesToApply());

    TargetPhraseCollection &phraseColl = GetOrCreateTargetPhraseCollection(rootNode, sourcePhrase, *targetPhrase, sourceLHS);
    phraseColl.Add(targetPhrase);

    count++;

    if (format == HieroFormat) { // reformat line
      delete line;
    } else {
      // do nothing
    }

  }

  // sort and prune each target phrase collection
  SortAndPrune(rootNode);

  //removedirectoryrecursively(dirName);
}

TargetPhraseCollection &PhraseDictionaryFuzzyMatch::GetOrCreateTargetPhraseCollection(PhraseDictionaryNodeMemory &rootNode
    , const Phrase &source
    , const TargetPhrase &target
    , const Word *sourceLHS)
{
  PhraseDictionaryNodeMemory &currNode = GetOrCreateNode(rootNode, source, target, sourceLHS);
  return currNode.GetTargetPhraseCollection();
}

PhraseDictionaryNodeMemory &PhraseDictionaryFuzzyMatch::GetOrCreateNode(PhraseDictionaryNodeMemory &rootNode
    , const Phrase &source
    , const TargetPhrase &target
    , const Word *sourceLHS)
{
  cerr << source << endl << target << endl;
  const size_t size = source.GetSize();

  const AlignmentInfo &alignmentInfo = target.GetAlignNonTerm();
  AlignmentInfo::const_iterator iterAlign = alignmentInfo.begin();

  PhraseDictionaryNodeMemory *currNode = &rootNode;
  for (size_t pos = 0 ; pos < size ; ++pos) {
    const Word& word = source.GetWord(pos);

    if (word.IsNonTerminal()) {
      // indexed by source label 1st
      const Word &sourceNonTerm = word;

      UTIL_THROW_IF2(iterAlign == alignmentInfo.end(),
    		  "No alignment for non-term at position " << pos);
      UTIL_THROW_IF2(iterAlign->first != pos,
    		  "Alignment info incorrect at position " << pos);

      size_t targetNonTermInd = iterAlign->second;
      ++iterAlign;
      const Word &targetNonTerm = target.GetWord(targetNonTermInd);

      currNode = currNode->GetOrCreateChild(sourceNonTerm, targetNonTerm);
    } else {
      currNode = currNode->GetOrCreateChild(word);
    }

    UTIL_THROW_IF2(currNode == NULL,
    		"Node not found at position " << pos);

  }

  // finally, the source LHS
  //currNode = currNode->GetOrCreateChild(sourceLHS);

  return *currNode;
}

void PhraseDictionaryFuzzyMatch::SortAndPrune(PhraseDictionaryNodeMemory &rootNode)
{
  if (GetTableLimit()) {
    rootNode.Sort(GetTableLimit());
  }
}

void PhraseDictionaryFuzzyMatch::CleanUpAfterSentenceProcessing(const InputType &source)
{
  m_collection.erase(source.GetTranslationId());
}

const PhraseDictionaryNodeMemory &PhraseDictionaryFuzzyMatch::GetRootNode(long translationId) const
{
  std::map<long, PhraseDictionaryNodeMemory>::const_iterator iter = m_collection.find(translationId);
  UTIL_THROW_IF2(iter == m_collection.end(),
		  "Couldn't find root node for input: " << translationId);
  return iter->second;
}
PhraseDictionaryNodeMemory &PhraseDictionaryFuzzyMatch::GetRootNode(const InputType &source)
{
  long transId = source.GetTranslationId();
  std::map<long, PhraseDictionaryNodeMemory>::iterator iter = m_collection.find(transId);
  UTIL_THROW_IF2(iter == m_collection.end(),
		  "Couldn't find root node for input: " << transId);
  return iter->second;
}

TO_STRING_BODY(PhraseDictionaryFuzzyMatch);

// friend
ostream& operator<<(ostream& out, const PhraseDictionaryFuzzyMatch& phraseDict)
{
  typedef PhraseDictionaryNodeMemory::TerminalMap TermMap;
  typedef PhraseDictionaryNodeMemory::NonTerminalMap NonTermMap;

  /*
  const PhraseDictionaryNodeMemory &coll = phraseDict.m_collection;
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
