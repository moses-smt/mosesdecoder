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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
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
#include "moses/Range.h"
#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerMemoryPerSentence.h"
#include "moses/TranslationModel/fuzzy-match/FuzzyMatchWrapper.h"
#include "moses/TranslationModel/fuzzy-match/SentenceAlignment.h"
#include "moses/TranslationTask.h"
#include "util/file.hh"
#include "util/exception.hh"
#include "util/random.hh"

using namespace std;

#if defined __MINGW32__ && !defined mkdtemp
#include <windows.h>
#include <cerrno>
char *mkdtemp(char *tempbuf)
{
  int rand_value = 0;
  char* tempbase = NULL;
  char tempbasebuf[MAX_PATH] = "";

  if (strcmp(&tempbuf[strlen(tempbuf)-6], "XXXXXX")) {
    errno = EINVAL;
    return NULL;
  }

  util::rand_init();
  rand_value = util::rand_excl(1e6);
  tempbase = strrchr(tempbuf, '/');
  tempbase = tempbase ? tempbase+1 : tempbuf;
  strcpy(tempbasebuf, tempbase);
  sprintf(&tempbasebuf[strlen(tempbasebuf)-6], "%d", rand_value);
  ::GetTempPath(MAX_PATH, tempbuf);
  strcat(tempbuf, tempbasebuf);
  ::CreateDirectory(tempbuf, NULL);
  return tempbuf;
}
#endif

namespace Moses
{

PhraseDictionaryFuzzyMatch::PhraseDictionaryFuzzyMatch(const std::string &line)
  :PhraseDictionary(line, true)
  ,m_config(3)
  ,m_FuzzyMatchWrapper(NULL)
{
  ReadParameters();
}

PhraseDictionaryFuzzyMatch::~PhraseDictionaryFuzzyMatch()
{
  delete m_FuzzyMatchWrapper;
}

void PhraseDictionaryFuzzyMatch::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  SetFeaturesToApply();

  m_FuzzyMatchWrapper = new tmmt::FuzzyMatchWrapper(m_config[0], m_config[1], m_config[2]);
}

ChartRuleLookupManager *PhraseDictionaryFuzzyMatch::CreateRuleLookupManager(
  const ChartParser &parser,
  const ChartCellCollectionBase &cellCollection,
  std::size_t /*maxChartSpan*/)
{
  return new ChartRuleLookupManagerMemoryPerSentence(parser, cellCollection, *this);
}

void
PhraseDictionaryFuzzyMatch::
SetParameter(const std::string& key, const std::string& value)
{
  if (key == "source") {
    m_config[0] = value;
  } else if (key == "target") {
    m_config[1] = value;
  } else if (key == "alignment") {
    m_config[2] = value;
  } else {
    PhraseDictionary::SetParameter(key, value);
  }
}

int removedirectoryrecursively(const char *dirname)
{
#if defined __MINGW32__
  //TODO(jie): replace this function with boost implementation
#else
  DIR *dir;
  struct dirent *entry;
  char path[PATH_MAX];

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
#endif
  return 1;
}

void PhraseDictionaryFuzzyMatch::InitializeForInput(ttasksptr const& ttask)
{
  InputType const& inputSentence = *ttask->GetSource();
#if defined __MINGW32__
  char dirName[] = "moses.XXXXXX";
#else
  char dirName[] = "/tmp/moses.XXXXXX";
#endif // defined
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
      UTIL_THROW2("Syntax error at " << ptFileName << ":" << count);
    }

    const string &sourcePhraseString = tokens[0]
                                       , &targetPhraseString = tokens[1]
                                           , &scoreString        = tokens[2]
                                               , &alignString        = tokens[3];

    bool isLHSEmpty = (sourcePhraseString.find_first_not_of(" \t", 0) == string::npos);
    if (isLHSEmpty && !ttask->options()->unk.word_deletion_enabled) {
      TRACE_ERR( ptFileName << ":" << count << ": pt entry contains empty target, skipping\n");
      continue;
    }

    Tokenize<float>(scoreVector, scoreString);
    const size_t numScoreComponents = GetNumScoreComponents();
    if (scoreVector.size() != numScoreComponents) {
      UTIL_THROW2("Size of scoreVector != number (" << scoreVector.size() << "!="
                  << numScoreComponents << ") of score components on line " << count);
    }

    UTIL_THROW_IF2(scoreVector.size() != numScoreComponents,
                   "Number of scores incorrectly specified");

    // parse source & find pt node

    // constituent labels
    Word *sourceLHS;
    Word *targetLHS;

    // source
    Phrase sourcePhrase( 0);
    sourcePhrase.CreateFromString(Input, m_input, sourcePhraseString, &sourceLHS);

    // create target phrase obj
    TargetPhrase *targetPhrase = new TargetPhrase(this);
    targetPhrase->CreateFromString(Output, m_output, targetPhraseString, &targetLHS);

    // rest of target phrase
    targetPhrase->SetAlignmentInfo(alignString);
    targetPhrase->SetTargetLHS(targetLHS);
    //targetPhrase->SetDebugOutput(string("New Format pt ") + line);

    // component score, for n-best output
    std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),TransformScore);
    std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),FloorScore);

    targetPhrase->GetScoreBreakdown().Assign(this, scoreVector);
    targetPhrase->EvaluateInIsolation(sourcePhrase, GetFeaturesToApply());

    TargetPhraseCollection::shared_ptr phraseColl
    = GetOrCreateTargetPhraseCollection(rootNode, sourcePhrase,
                                        *targetPhrase, sourceLHS);
    phraseColl->Add(targetPhrase);

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

TargetPhraseCollection::shared_ptr
PhraseDictionaryFuzzyMatch::
GetOrCreateTargetPhraseCollection(PhraseDictionaryNodeMemory &rootNode
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

#if defined(UNLABELLED_SOURCE)
      currNode = currNode->GetOrCreateNonTerminalChild(targetNonTerm);
#else
      currNode = currNode->GetOrCreateChild(sourceNonTerm, targetNonTerm);
#endif
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
  /*
  typedef PhraseDictionaryNodeMemory::TerminalMap TermMap;
  typedef PhraseDictionaryNodeMemory::NonTerminalMap NonTermMap;

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
