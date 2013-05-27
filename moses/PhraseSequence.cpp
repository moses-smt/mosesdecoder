//Fabienne Braune
//Sequence of phrases for decoding with l-MBOT rules
//

#include <vector>
#include "PhraseSequence.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{
	PhraseSequence::PhraseSequence()
	{
		m_phraseSequence = vector<Phrase*>();
	}

	PhraseSequence::~PhraseSequence()
	{
		RemoveAllInColl(m_phraseSequence);
	};

	void PhraseSequence::Add(Phrase * phrase)
	{
		m_phraseSequence.push_back(phrase);
	};

	void PhraseSequence::CreatePhraseFromString(vector<StringPiece> annotatedWordVector, const FactorDirection &direction, const vector<FactorType> &factorOrder)
	{
		 	 	 Phrase * myPhrase = new Phrase(annotatedWordVector.size() -1);

		         for (size_t phrasePos = 0 ; phrasePos < annotatedWordVector.size() -  1 ; phrasePos++) {
		           StringPiece &annotatedWord = annotatedWordVector[phrasePos];
		           bool isNonTerminal;
		           if (annotatedWord.size() >= 2 && *annotatedWord.data() == '[' && annotatedWord.data()[annotatedWord.size() - 1] == ']') {
		             // non-term
		             isNonTerminal = true;

		             size_t nextPos = annotatedWord.find('[', 1);
		             CHECK(nextPos != string::npos);

		             if (direction == Input)
		               annotatedWord = annotatedWord.substr(1, nextPos - 2);
		             else
		               annotatedWord = annotatedWord.substr(nextPos + 1, annotatedWord.size() - nextPos - 2);
		           } else {
		             isNonTerminal = false;
		           }

		           Word wordToAdd = Word();
		           wordToAdd.CreateFromString(direction, factorOrder, annotatedWord, isNonTerminal);
		           myPhrase->AddWord(wordToAdd);
		         }
		  Add(myPhrase);
	};

	vector<Phrase*> *PhraseSequence::GetSequence() const
	{
		return const_cast<vector<Phrase*> *>(&m_phraseSequence);
	};

	Phrase *PhraseSequence::GetPhrase(size_t position) const
	{
		CHECK(position < GetSize());
		return m_phraseSequence.at(position);
	};

	size_t PhraseSequence::GetSize() const
	{
		return m_phraseSequence.size();
	};

	ostream& operator<<(ostream& out, const PhraseSequence& ps)
	{
	  int counter = 1;
	  PhraseSequence::const_iterator itr;
	  for(itr = ps.begin(); itr != ps.end(); itr++)
	  {
		  out << **itr << "(" << counter << ")";
		  counter++;
	  }
	  out << std::endl;
	}
}
