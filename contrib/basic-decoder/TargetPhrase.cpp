
#include "TargetPhrase.h"
#include "Util.h"
#include <stdlib.h>

using namespace std;

TargetPhrase::TargetPhrase(size_t size)
  :Phrase(size)
  ,m_scores()
{
  // TODO Auto-generated constructor stub

}

TargetPhrase::~TargetPhrase()
{
}

TargetPhrase *TargetPhrase::CreateFromString(
  const FeatureFunction &ff,
  const std::string &targetStr,
  const std::string &scoreStr,
  bool logScores)
{
  vector<string> toks;

  // words
  Tokenize(toks, targetStr);
  TargetPhrase *phrase = new TargetPhrase(toks.size());

  for (size_t i = 0; i < toks.size(); ++i) {
    Word &word = phrase->GetWord(i);
    word.CreateFromString(toks[i]);
  }

  // score
  phrase->GetScores().CreateFromString(ff, scoreStr, logScores);

  return phrase;
}

std::string TargetPhrase::Debug() const
{
  stringstream strme;
  strme << Phrase::Debug() << " ";
  strme << m_scores.Debug();
  return strme.str();
}
