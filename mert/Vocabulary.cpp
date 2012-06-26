#include "Vocabulary.h"
#include "Singleton.h"

namespace mert {
namespace {
Vocabulary* g_vocab = NULL;
} // namespace

int Vocabulary::Encode(const std::string& token) {
#ifdef WITH_THREADS
    boost::upgrade_lock<boost::shared_mutex> read_lock(m_accessLock);
#endif // WITH_THREADS

	iterator it = m_vocab.find(token);
	int encoded_token;
	if (it == m_vocab.end()) {
		// Add an new entry to the vocaburary.
		encoded_token = static_cast<int>(m_vocab.size());
		
#ifdef WITH_THREADS
		boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(read_lock);
#endif // WITH_THREADS

		m_vocab[token] = encoded_token;
	} else {
#ifdef WITH_THREADS
		boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(read_lock);
#endif // WITH_THREADS
		encoded_token = it->second;
	}
	return encoded_token;
}

bool Vocabulary::Lookup(const std::string&str , int* v) const {
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_accessLock);
#endif // WITH_THREADS

	const_iterator it = m_vocab.find(str);
	if (it == m_vocab.end()) return false;
	*v = it->second;
	return true;
}

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
