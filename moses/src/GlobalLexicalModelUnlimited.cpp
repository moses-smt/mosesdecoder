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
}

GlobalLexicalModelUnlimited::~GlobalLexicalModelUnlimited(){}

void GlobalLexicalModelUnlimited::LoadData(const vector< FactorType >& inFactors,
                                  const vector< FactorType >& outFactors)
{
//  m_inputFactors = FactorMask(inFactors);
//  m_outputFactors = FactorMask(outFactors);
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
//	cerr << endl;
	set< const Word*, WordComparer > alreadyScored; // do not score a word twice
	for(size_t inputIndex = 0; inputIndex < m_input->GetSize(); inputIndex++ ) {
	  const Word& inputWord = m_input->GetWord( inputIndex );
	  if ( alreadyScored.find( &inputWord ) == alreadyScored.end() ) {
		stringstream feature("glm_");
		feature << targetWord.GetString(m_outputFactors, false);
		feature << ":";
		feature << inputWord.GetString(m_inputFactors, false);
//		cerr << "feature: " << feature.str() << endl;
		accumulator->PlusEquals(this, feature.str(), 1);
		alreadyScored.insert( &inputWord );
	  }
	}

	// Hal Daume says: 1/( 1 + exp [ - sum_i w_i * f_i ] )
//	VERBOSE(2," p=" << FloorScore( log(1/(1+exp(-sum))) ) << endl);
//	score += FloorScore( log(1/(1+exp(-sum))) );
  }
}

}
