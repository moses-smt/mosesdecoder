//Fabienne Braune
//Sequence of Words for decoding with l-MBOT rules
//

#include <vector>
#include "WordSequence.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{

	WordSequence::WordSequence()
	{
		m_wordSequence = vector<Word>();
	};

	WordSequence::WordSequence(const Word &firstWord)
	{
		m_wordSequence = vector<Word>();
		m_wordSequence.push_back(firstWord);
	};

	void WordSequence::Add(Word Word)
	{
		m_wordSequence.push_back(Word);
	};


	void WordSequence::CreateWordFromString(StringPiece annotatedWord, const FactorDirection &direction, const vector<FactorType> &factorOrder)
	{
	     CHECK(annotatedWord.size() >= 2 && *annotatedWord.data() == '[' && annotatedWord.data()[annotatedWord.size() - 1] == ']');

	    Word myLhs = Word();
	    myLhs.CreateFromString(direction, factorOrder, annotatedWord.substr(1, annotatedWord.size() - 2), true);
	    CHECK(myLhs.IsNonTerminal());
	    m_wordSequence.push_back(myLhs);
	}

	vector<Word> *WordSequence::GetSequence()
	{
		vector<Word> * ret;
		ret = &m_wordSequence;
		return ret;
	};


	Word *WordSequence::GetWord(size_t position) const
	{
		CHECK(position < GetSize());
		return const_cast<Word*> (&(m_wordSequence.at(position)));
	};

	size_t WordSequence::GetSize() const
	{
		return m_wordSequence.size();
	};

	bool WordSequence::AreNonTerms() const
	{
		WordSequence::const_iterator itr;
		for(itr = m_wordSequence.begin(); itr != m_wordSequence.end(); itr++)
		{
		  if(!(itr->IsNonTerminal()))
			{
				return false;
			}
		}
		return true;
	}

	void WordSequence::Clear()
	{
		m_wordSequence.clear();
	}

	ostream& operator<<(ostream& out, const WordSequence& ws)
		{
		  int counter = 1;
		  WordSequence::const_iterator itr;
		  for(itr = ws.begin(); itr != ws.end(); itr++)
		  {
			  out << *itr << "(" << counter << ")";
			  counter++;
		  }
		  out << std::endl;
		}

}
