/*
 * PhraseTableMemory.cpp
 *
 *  Created on: 5 Oct 2013
 *      Author: hieu
 */

#include <string>
#include <vector>
#include "PhraseTableMemory.h"
#include "InputFileStream.h"
#include "Util.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "InputPath.h"

using namespace std;


PhraseTableMemory::PhraseTableMemory(const std::string &line)
  :PhraseTable(line)
  ,m_tableLimit(20)
{
  ReadParameters();
}

PhraseTableMemory::~PhraseTableMemory()
{
  // TODO Auto-generated destructor stub
}

void PhraseTableMemory::Load()
{
  Scores *estimatedFutureScore = new Scores();

  Moses::InputFileStream iniStrme(m_path);

  vector<string> toks;
  size_t lineNum = 0;
  string line;
  while (getline(iniStrme, line)) {
    if (lineNum % 10000 == 0) {
      cerr << lineNum << " " << flush;
    }
    toks.clear();
    TokenizeMultiCharSeparator(toks, line, "|||");

    Phrase *source = Phrase::CreateFromString(toks[0]);
    TargetPhrase *target = TargetPhrase::CreateFromString(*this, toks[1], toks[2], true);
    FeatureFunction::Evaluate(*source, *target, *estimatedFutureScore);

    //cerr << target->Debug() << endl;

    Node &node = m_root.GetOrCreate(*source, 0);
    node.AddTarget(target);

    ++lineNum;
  }
}

void PhraseTableMemory::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    m_path = value;
  } else if (key == "table-limit") {
    m_tableLimit = Scan<size_t>(value);
  } else {
    PhraseTable::SetParameter(key, value);
  }
}

void PhraseTableMemory::Lookup(const std::vector<InputPath*> &inputPathQueue)
{
  for (size_t i = 0; i < inputPathQueue.size(); ++i) {
    InputPath &path = *inputPathQueue[i];
    const InputPath *prevPath = path.GetPrevPath();

    //cerr << path.GetPhrase().Debug() << endl;

    // which node to start the lookup
    const Node *node;
    if (prevPath) {
      // get node from previous lookup.
      // May be null --> don't lookup any further
      node = (const Node *) prevPath->GetPtLookup(m_ptId).ptNode;
    } else {
      // 1st lookup. Start from root
      node = &m_root;
    }

    // where to store the info for this lookup
    PhraseTableLookup &ptLookup = path.GetPtLookup(m_ptId);
    if (node) {
      // LOOKUP
      // lookup the LAST word only
      const Phrase &source = path.GetPhrase();
      const Word &lastWord = source.Back();

      node = node->Get(lastWord);
    }

    if (node) {
      // found something
      const TargetPhrases &tpColl = node->GetTargetPhrases();
      ptLookup.Set(&tpColl, node);
    } else {
      ptLookup.Set(NULL, NULL);
    }
  }
}
