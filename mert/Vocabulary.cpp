#include "Vocabulary.h"
#include "Singleton.h"

namespace mert
{
namespace
{
Vocabulary* g_vocab = NULL;
} // namespace

int Vocabulary::Encode(const std::string& token)
{
  iterator it = m_vocab.find(token);
  int encoded_token;
  if (it == m_vocab.end()) {
    // Add an new entry to the vocaburary.
    encoded_token = static_cast<int>(m_vocab.size());

    m_vocab[token] = encoded_token;
  } else {
    encoded_token = it->second;
  }
  return encoded_token;
}

bool Vocabulary::Lookup(const std::string&str , int* v) const
{

  const_iterator it = m_vocab.find(str);
  if (it == m_vocab.end()) return false;
  *v = it->second;
  return true;
}

Vocabulary* VocabularyFactory::GetVocabulary()
{
  if (g_vocab == NULL) {
    return MosesTuning::Singleton<Vocabulary>::GetInstance();
  } else {
    return g_vocab;
  }
}

void VocabularyFactory::SetVocabulary(Vocabulary* vocab)
{
  g_vocab = vocab;
}

} // namespace mert
