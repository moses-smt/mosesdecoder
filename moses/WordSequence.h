//Fabienne Braune
//Extension of Moses for handling sequences of words (e.g. non-terminals in lhs of target sequence)
//Sequence of words for decoding with l-MBOT rules

#ifndef WordSequence_h
#define WordSequence_h

#include <vector>
#include "Word.h"

using namespace std;

namespace Moses
{

class WordSequence
{
	protected:
	vector<Word> m_wordSequence;

	public:
	typedef vector<Word>::const_iterator const_iterator;
	//TODO : Add constructor
	 WordSequence();
	~WordSequence();

	//! iterators
	const_iterator begin() const {
		return m_wordSequence.begin();
	}
	const_iterator end() const {
		return m_wordSequence.end();
	}

	void Add(Word word);

	void CreateWordFromString(StringPiece word, const FactorDirection &fd, const vector<FactorType> &fo);

	bool operator< (const WordSequence& other) const {
		vector<Word> :: iterator itr_words;
		if(GetSize() != other.GetSize())
		{
			return GetSize() < other.GetSize();
		}
		for(size_t i = 0; i < GetSize(); i++)
		{
			return (GetWord(i)) < (other.GetWord(i));
		}
	}

	vector<Word> *GetSequence();
	Word *GetWord(size_t position) const;
	size_t GetSize() const;
	bool AreNonTerms() const;
	void Clear();



};

}

#endif
