#include "ContextFactor.h"
#include "Phrase.h"
#include <vector>

using namespace std;

namespace Moses
{

ContextFactor::ContextFactor(size_t ngramOrder,size_t mbotSize)
{
	//std::cerr << "Constructing : " << mbotSize <<  std::endl;
	m_ngramOrder = ngramOrder;
	m_mbotPosition = 0;
	m_previousPosition = 0;
	m_hypoId = -1;
	m_seenHypo = 0;

	//for(size_t m= 0; m < mbotSize; m++)
	//{
		//m_contextStartPositions.push_back(0);
		//m_contextEndPositions.push_back(0);
	//}
}

size_t ContextFactor::GetMbotPosition()
{
	return m_mbotPosition;
}

size_t ContextFactor::GetPreviousPosition()
{
	return m_previousPosition;
}

const Phrase * ContextFactor::GetPhrase()
{
	return m_mbotPhrase[m_mbotPosition];
}

const size_t ContextFactor::GetPhraseSize()
{
	return m_mbotPhrase[m_mbotPosition]->GetSize();
}

const Phrase * ContextFactor::GetPrefixPhrase()
{
	return m_prefixPhrase;
}

const size_t ContextFactor::GetPrefixSize()
{
	return m_prefixPhrase->GetSize();
}

void ContextFactor::IncrementMbotPosition()
{
	m_mbotPosition++;
}

void ContextFactor::IncrementPreviousPosition()
{
	m_previousPosition++;
}

void ContextFactor::SetHypoId(size_t hypoId)
{
	m_hypoId = hypoId;
}

int ContextFactor::IsSameHypoId(size_t hypoId)
{
	if(hypoId == m_hypoId)
	{m_seenHypo++;}
	else
	{m_seenHypo = 0;}
	return m_seenHypo;
}

void ContextFactor::AddPhrase(const Phrase * current)
{
        //std::cerr << "Adding phrase : " << current << std::endl;
	m_mbotPhrase.push_back(current);
}

void ContextFactor::SetPrefix(const Phrase * prefix)
{
	m_prefixPhrase = prefix;
}


void ContextFactor::AddWordFromCurrent(size_t position)
{
	//std::cerr << "ADDING WORD FROM CURRENT : " << m_mbotPhrase[m_mbotPosition]->GetWord(position) << std::endl;
	m_contextFactors.push_back(&m_mbotPhrase[m_mbotPosition]->GetWord(position));
}

void ContextFactor::AddWordFromPrefix(size_t position)
{
     //std::cerr << "ADDING WORD FROM PREFIX : " << m_prefixPhrase->GetWord(position) << std::endl;

	//with vector of mbot phrases
	//m_contextFactors.push_back(&m_mbotPrefix[m_mbotPosition]->GetWord(position));

	//without vector of mbot phrases : TODO remove : is wrong
	m_contextFactors.push_back(&m_prefixPhrase->GetWord(position));
}

size_t ContextFactor::GetNumberWords()
{
	return m_contextFactors.size();
}

void ContextFactor::ShiftAndPop()
{
	//shift
	for (size_t currNGramOrder = 0 ; currNGramOrder < m_ngramOrder - 1 ; currNGramOrder++) {
		//std::cerr << "SHIFT : " << *(m_contextFactors[currNGramOrder]) << " to : " << *(m_contextFactors[currNGramOrder + 1]) << std::endl;
		m_contextFactors[currNGramOrder] = m_contextFactors[currNGramOrder + 1];
	}
	//remove last word
	m_contextFactors.pop_back();

}

bool ContextFactor::IsContextEmpty()
{
	return (m_mbotPhrase[m_mbotPosition]->GetSize() == 0);
}

void ContextFactor::Clear()
{

	m_contextFactors.clear();

}

void ContextFactor::ResetMbotPosition()
{
	m_mbotPosition = 0;
}

void ContextFactor::ResetPreviousPosition()
{
	m_previousPosition = 0;
}

ContextFactor::~ContextFactor()
{
	Clear();
}

//Fabienne Braune : ignore start of sentence symbol
void ContextFactor::ShiftOrPushFromCurrent(size_t position)
{
  //m_mbotContextWords.push_back(copy);
  if (GetNumberWords() < m_ngramOrder) {
	  //std::cout << "SHIFT OR PUSH : pushed " << *copy << " : " << contextFactor.size() << std::endl;
	  AddWordFromCurrent(position);
  }
  //contextFactor.push_back(&word);
  else {
	  ShiftAndPop();
	  AddWordFromCurrent(position);
  }
}

//Fabienne Braune : ignore start of sentence symbol
void ContextFactor::ShiftOrPushFromPrefix(size_t position)
{
	//m_mbotContextWords.push_back(copy);
	  if (GetNumberWords() < m_ngramOrder) {
		  //std::cout << "SHIFT OR PUSH : pushed " << *copy << " : " << contextFactor.size() << std::endl;
		  AddWordFromPrefix(position);
	  }
	  //contextFactor.push_back(&word);
	  else {
		  ShiftAndPop();
		  AddWordFromPrefix(position);
	  }
}

vector<const Word*> ContextFactor::GetContextFactor()
{
	//maybe need to implement a return vector
	return m_contextFactors;
}
}//end namespace





