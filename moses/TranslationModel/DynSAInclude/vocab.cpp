#include <sstream>
#include "vocab.h"

namespace Moses
{

// Vocab class
void Vocab::InitSpecialWords()
{
  m_kBOSWord = InitSpecialWord(BOS_);	// BOS_ is a string <s> (defined in ../typedef.h)
  m_kEOSWord = InitSpecialWord(EOS_);	// EOS_ is a string </s> (defined in ../typedef.h)
  m_kOOVWord = InitSpecialWord(UNKNOWN_FACTOR);	// UNKNOWN_FACTOR also defined in ../typedef.h
}

const Word Vocab::InitSpecialWord( const string& word_str)
{
  FactorList factors;
  factors.push_back(0); // store the special word string as the first factor
  Word word;
  // define special word as Input word with one factor and isNonTerminal=false
  word.CreateFromString( Input, factors, word_str, false ); // Input is enum defined in ../typedef.h
  // TODO not sure if this will work properly:
  // 	- word comparison can fail because the last parameter (isNonTerminal)
  // 		in function CreateFromString may not match properly created words
  // 	- special word is Input word but what about Output words?
  // 		- currently Input/Output variable is not stored in class Word, but in the future???
  return word;
}
wordID_t Vocab::GetWordID(const std::string& word_str)
{
  FactorList factors;
  factors.push_back(0);
  Word word;
  word.CreateFromString(Input, factors, word_str, false);
  return GetWordID(word);
}

// get wordID_t index for word represented as string
wordID_t Vocab::GetWordID(const std::string& word_str,
                          const FactorDirection& direction, const FactorList& factors, bool isNonTerminal)
{
  // get id for factored string
  Word word;
  word.CreateFromString( direction, factors, word_str, isNonTerminal);
  return GetWordID( word);
}

wordID_t Vocab::GetWordID(const Word& word)
{
  // get id and possibly add to vocab
  if(m_words2ids.find(word) == m_words2ids.end()) {
    if (!m_closed) {
      wordID_t id = m_words2ids.size() + 1;
      m_ids2words[id] = word;
      // update lookup tables
      m_words2ids[word] = id;
    } else {
      return m_kOOVWordID;
    }
  }
  wordID_t id = m_words2ids[word];
  return id;
}

Word& Vocab::GetWord(wordID_t id)
{
  // get word string given id
  return (m_ids2words.find(id) == m_ids2words.end()) ? m_kOOVWord : m_ids2words[id];
}

bool Vocab::InVocab(wordID_t id)
{
  return m_ids2words.find(id) != m_ids2words.end();
}

bool Vocab::InVocab(const Word& word)
{
  return m_words2ids.find(word) != m_words2ids.end();
}

bool Vocab::Save(const std::string & vocab_path)
{
  // save vocab as id -> word
  FileHandler vcbout(vocab_path, std::ios::out);
  return Save(&vcbout);
}

bool Vocab::Save(FileHandler* vcbout)
{
  // then each vcb entry
  *vcbout << m_ids2words.size() << "\n";
  for (Id2Word::const_iterator iter = m_ids2words.begin();
       iter != m_ids2words.end(); ++iter) {
    *vcbout << iter->second << "\t" << iter->first << "\n";
  }
  return true;
}

bool Vocab::Load(const std::string & vocab_path, const FactorDirection& direction,
                 const FactorList& factors, bool closed)
{
  FileHandler vcbin(vocab_path, std::ios::in);
  std::cerr << "Loading vocab from " << vocab_path << std::endl;
  return Load(&vcbin, direction, factors, closed);
}
bool Vocab::Load(FileHandler* vcbin)
{
  FactorList factors;
  factors.push_back(0);
  return Load(vcbin, Input, factors);
}
bool Vocab::Load(FileHandler* vcbin, const FactorDirection& direction,
                 const FactorList& factors, bool closed)
{
  // load vocab id -> word mapping
  m_words2ids.clear();	// reset mapping
  m_ids2words.clear();
  std::string line, word_str;
  wordID_t id;

  std::istream &ret = getline(*vcbin, line);
  UTIL_THROW_IF2(!ret, "Couldn't read file");
  std::istringstream first(line.c_str());
  uint32_t vcbsize(0);
  first >> vcbsize;
  uint32_t loadedsize = 0;
  while (loadedsize++ < vcbsize && getline(*vcbin, line)) {
    std::istringstream entry(line.c_str());
    entry >> word_str;
    Word word;
    word.CreateFromString( direction, factors, word_str, false); // TODO set correctly isNonTerminal
    entry >> id;
    // may be no id (i.e. file may just be a word list)
    if (id == 0 && word != GetkOOVWord())
      id = m_ids2words.size() + 1;	// assign ids sequentially starting from 1
    UTIL_THROW_IF2(m_ids2words.count(id) != 0 || m_words2ids.count(word) != 0,
    		"Error");

    m_ids2words[id] = word;
    m_words2ids[word] = id;
  }
  m_closed = closed;	// once loaded fix vocab ?
  std::cerr << "Loaded vocab with " << m_ids2words.size() << " words." << std::endl;
  return true;
}
void Vocab::PrintVocab()
{
  for (Id2Word::const_iterator iter = m_ids2words.begin();
       iter != m_ids2words.end(); ++iter ) {
    std::cerr << iter->second << "\t" << iter->first << "\n";
  }
  for (Word2Id::const_iterator iter = m_words2ids.begin();
       iter != m_words2ids.end(); ++iter ) {
    std::cerr << iter->second << "\t" << iter->first << "\n";
  }
}

} //end namespace
