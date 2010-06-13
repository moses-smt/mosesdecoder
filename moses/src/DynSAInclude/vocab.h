#ifndef moses_DynSAInclude_vocab_h
#define moses_DynSAInclude_vocab_h

#include <map>
#include <string>
#include "types.h"
#include "file.h"
#include "utils.h"
#include "../TypeDef.h"
#include "../Word.h"

namespace Moses {
// Vocab maps between strings and uint32 ids.

class Vocab {
	public:
	
	typedef std::map<Word, wordID_t> Word2Id;
	typedef std::map<wordID_t, Word> Id2Word;
	
	Vocab(bool sntMarkers = true): 
		m_closed(false),
		m_kOOVWordID(0),
		m_kBOSWordID(1)
	{
		InitSpecialWords();
		if(sntMarkers) {
			GetWordID(m_kBOSWord);	// added in case not observed in corpus
			GetWordID(m_kEOSWord);
		}
	}	
	// if no file then must allow new words
	// specify whether more words can be added via 'closed'
	// assume that if a vocab is loaded from file then it should be closed.
	Vocab(const std::string & vocab_path, const FactorDirection& direction, 
		const FactorList& factors, bool closed = true):
		m_kOOVWordID(0),
		m_kBOSWordID(1)
	{
		InitSpecialWords();
		assert( Load(vocab_path, direction, factors, closed));	
	}
	Vocab(FileHandler * fin, const FactorDirection& direction, 
		const FactorList& factors, bool closed = true):
		m_kOOVWordID(0),
		m_kBOSWordID(1)
	{
		InitSpecialWords();
		assert( Load(fin, direction, factors, closed));	
	}
	~Vocab() {}
	// parse 'word' into factored Word and get id
	wordID_t GetWordID(const std::string& word, const FactorDirection& direction, 
		const FactorList& factors, bool isNonTerminal);
	wordID_t GetWordID(const Word& word);
	wordID_t GetWordID(const string& word);
	Word& GetWord(wordID_t id);
	inline const wordID_t GetkOOVWordID() { return m_kOOVWordID; }
	inline const wordID_t GetBOSWordID() { return m_kBOSWordID; }
	inline const Word& GetkOOVWord() { return m_kOOVWord; }
	inline const Word& GetkBOSWord() { return m_kBOSWord; }
	inline const Word& GetkEOSWord() { return m_kEOSWord; }

	bool InVocab(wordID_t id);
	bool InVocab(const Word& word);
	uint32_t Size() { return m_words2ids.size(); }
	void MakeClosed() { m_closed = true; }
	void MakeOpen() { m_closed = false; }
	bool IsClosed() { return m_closed; }
	bool Save(const std::string & vocab_path);
	bool Save(FileHandler* fout);
	bool Load(const std::string & vocab_path, const FactorDirection& direction, 
		const FactorList& factors, bool closed = true);
	bool Load(FileHandler* fin, const FactorDirection& direction, 
		const FactorList& factors, bool closed = true);
	void PrintVocab();
protected:
	bool m_closed;	// can more words be added
	
	const wordID_t m_kOOVWordID;	 // out of vocabulary word id
	const wordID_t m_kBOSWordID;		
	Word m_kBOSWord;	// beginning of sentence marker
	Word m_kEOSWord;	// end of sentence marker
	Word m_kOOVWord;	// <unk>

	const Word InitSpecialWord( const string& type); // initialize special word like kBOS, kEOS
	void InitSpecialWords();	

	Word2Id m_words2ids;	// map from words to word ids
	Id2Word m_ids2words;	// map from ids to words
};

}

#endif
