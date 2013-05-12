//Fabienne Braune
//Extension of Moses for handling sequences of words (e.g. non-terminals in lhs of target sequence)
//Sequence of words for decoding with l-MBOT rules

#include <vector>
#include "Word.h"

using namespace std;

namespace Moses
{

class WordSequence
{
	protected:
	vector<Word*> *m_wordSequence;

	public:
	//TODO : Add constructor
	 WordSequence(){};
	~WordSequence();
	void Add(Word *word);
	vector<Word*> *GetSequence();
	Word *GetWord(size_t position);
	size_t GetSize();

};

}
