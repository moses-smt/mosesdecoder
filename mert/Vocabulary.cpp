#include "Vocabulary.h"
#include "Singleton.h"

namespace mert {
namespace {
Vocabulary* g_vocab = NULL;
} // namespace

Vocabulary* VocabularyFactory::GetVocabulary() {
  if (g_vocab == NULL) {
    return Singleton<Vocabulary>::GetInstance();
  } else {
    return g_vocab;
  }
}

void VocabularyFactory::SetVocabulary(Vocabulary* vocab) {
  g_vocab = vocab;
}

} // namespace mert
