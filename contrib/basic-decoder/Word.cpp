
#include "Word.h"
#include "MyVocab.h"

using namespace std;

Word::Word()
{
  // TODO Auto-generated constructor stub

}

Word::~Word()
{
}

void Word::Set(const Word &word)
{
  m_vocabId = word.m_vocabId;
}

void Word::CreateFromString(const std::string &line)
{
  FastMoses::MyVocab &vocab = FastMoses::MyVocab::Instance();
  m_vocabId = vocab.GetOrCreateId(line);
}

void Word::Output(std::ostream &out) const
{
  FastMoses::MyVocab &vocab = FastMoses::MyVocab::Instance();
  const string &ret = vocab.GetString(m_vocabId);
  out << ret;
}

std::string Word::ToString() const
{
  stringstream strme;
  Output(strme);
  return strme.str();
}

std::string Word::Debug() const
{
  return ToString();
}
