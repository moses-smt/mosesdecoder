#include "GlobalLexicalModelUnlimited.h"
#include <fstream>
#include "StaticData.h"
#include "InputFileStream.h"
#include "UserMessage.h"

using namespace std;

namespace Moses
{
GlobalLexicalModelUnlimited::GlobalLexicalModelUnlimited(const vector< FactorType >& inFactors,
                                                         const vector< FactorType >& outFactors)
: StatelessFeatureFunction("glm",ScoreProducer::unlimited),
  m_sparseProducerWeight(1)
{
	std::cerr << "Creating global lexical model unlimited...\n";

	// load model
	LoadData( inFactors, outFactors );

	// compile a list of punctuation characters
	char punctuation[] = "\"'!?¿·()#_,:;&-/\\0123456789";
	for (size_t i=0; i < sizeof(punctuation)-1; ++i)
		m_punctuationHash[punctuation[i]] = 1;
}

GlobalLexicalModelUnlimited::~GlobalLexicalModelUnlimited(){}

void GlobalLexicalModelUnlimited::LoadData(const vector< FactorType >& inFactors,
                                  const vector< FactorType >& outFactors)
{
	m_inputFactors = inFactors;
	m_outputFactors = outFactors;
}

void GlobalLexicalModelUnlimited::InitializeForInput( Sentence const& in )
{
  m_input = &in;
}

void GlobalLexicalModelUnlimited::Evaluate(const TargetPhrase& targetPhrase, ScoreComponentCollection* accumulator) const
{
  for(size_t targetIndex = 0; targetIndex < targetPhrase.GetSize(); targetIndex++ ) {
  	const Word& targetWord = targetPhrase.GetWord( targetIndex );

  	// check if first char is punctuation
		char firstChar = (targetWord.GetString(m_inputFactors, false)).at(0);
		CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
		if(charIterator != m_punctuationHash.end())
			continue;

  	set< const Word*, WordComparer > alreadyScored; // do not score a word twice
  	for(size_t inputIndex = 0; inputIndex < m_input->GetSize(); inputIndex++ ) {
  		const Word& inputWord = m_input->GetWord( inputIndex );

  		// check if first char is punctuation
  		firstChar = (inputWord.GetString(m_inputFactors, false)).at(0);
  		CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
  		if(charIterator != m_punctuationHash.end())
  			continue;

  		if ( alreadyScored.find( &inputWord ) == alreadyScored.end() ) {
  			stringstream feature("glm_");
  			feature << targetWord.GetString(m_outputFactors, false);
  			feature << "~";
  			feature << inputWord.GetString(m_inputFactors, false);
  			accumulator->PlusEquals(this, feature.str(), 1);
  			alreadyScored.insert( &inputWord );
  		}
  	}
  }
}

}
