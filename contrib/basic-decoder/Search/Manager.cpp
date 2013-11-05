
#include <iostream>
#include "Manager.h"
#include "InputPath.h"
#include "Hypothesis.h"
#include "Global.h"
#include "FF/StatefulFeatureFunction.h"
#include "FF/TranslationModel/PhraseTable.h"

using namespace std;

Manager::Manager(Sentence &sentence)
  :m_sentence(sentence)
  ,m_stacks(sentence.GetSize() + 1)
  ,m_emptyPhrase(new TargetPhrase(0))
  ,m_emptyRange(new WordsRange(NOT_FOUND, NOT_FOUND))
  ,m_emptyCoverage(new WordsBitmap(sentence.GetSize()))
{
  FeatureFunction::Initialize(m_sentence);

  Global &global = Global::InstanceNonConst();

  global.timer.check("Begin CreateInputPaths");
  CreateInputPaths();
  global.timer.check("Begin Lookup");
  Lookup();
  global.timer.check("Begin Search");
  Search();
  global.timer.check("Finished Search");
}

Manager::~Manager()
{
  FeatureFunction::CleanUp(m_sentence);
}

void Manager::CreateInputPaths()
{
  for (size_t pos = 0; pos < m_sentence.GetSize(); ++pos) {
    Phrase *phrase = new Phrase(1);
    phrase->Set(0, m_sentence.GetWord(pos));

    InputPath *path = new InputPath(NULL, phrase, pos);
    m_inputPathQueue.push_back(path);

    CreateInputPaths(*path, pos + 1);
  }
}

void Manager::CreateInputPaths(const InputPath &prevPath, size_t pos)
{
  if (pos >= m_sentence.GetSize()) {
    return;
  }

  Phrase *phrase = new Phrase(prevPath.GetPhrase(), 1);
  phrase->SetLastWord(m_sentence.GetWord(pos));

  InputPath *path = new InputPath(&prevPath, phrase, pos);
  m_inputPathQueue.push_back(path);

  CreateInputPaths(*path, pos + 1);
}

void Manager::Lookup()
{
  const std::vector<PhraseTable*> &pts = PhraseTable::GetColl();
  for (size_t i = 0; i < pts.size(); ++i) {
    PhraseTable &pt = *pts[i];
    pt.Lookup(m_inputPathQueue);
  }
}

void Manager::Search()
{
  Hypothesis *emptyHypo = new Hypothesis(*m_emptyPhrase, *m_emptyRange, *m_emptyCoverage);
  StatefulFeatureFunction::EvaluateEmptyHypo(m_sentence, *emptyHypo);

  m_stacks.Add(emptyHypo, 0);

  for (size_t i = 0; i < m_stacks.GetSize() - 1; ++i) {
    cerr << Debug() << endl;

    Stack &stack = m_stacks.Get(i);
    stack.PruneToSize();
    stack.Search(m_inputPathQueue);
  }

}

const Hypothesis *Manager::GetHypothesis() const
{
  const Stack &lastStack = m_stacks.Back();
  const Hypothesis *hypo = lastStack.GetHypothesis();
  return hypo;
}

std::string Manager::Debug() const
{
  stringstream strme;
  for (size_t i = 0; i < m_stacks.GetSize(); ++i) {
    const Stack &stack = m_stacks.Get(i);
    strme << stack.Debug() << " ";
  }
  return strme.str();
}
