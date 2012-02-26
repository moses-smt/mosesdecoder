#include "GlobalLexicalModelUnlimited.h"
#include <fstream>
#include "StaticData.h"
#include "InputFileStream.h"
#include "UserMessage.h"

using namespace std;

namespace Moses
{
GlobalLexicalModelUnlimited::GlobalLexicalModelUnlimited(const vector< FactorType >& inFactors,
                                                         const vector< FactorType >& outFactors,
                                                         bool biasFeature,
                                                         bool ignorePunctuation,
                                                         bool sourceContext)
: StatelessFeatureFunction("glm",ScoreProducer::unlimited),
  m_sparseProducerWeight(1),
  m_inputFactors(inFactors),
  m_outputFactors(outFactors),
  m_biasFeature(biasFeature),
  m_ignorePunctuation(ignorePunctuation),
  m_sourceContext(sourceContext),
  m_unrestricted(true)
{
	std::cerr << "Creating global lexical model unlimited.. ";

	// compile a list of punctuation characters
	if (m_ignorePunctuation) {
		cerr << "ignoring punctuation.. ";
		char punctuation[] = "\"'!?¿·()#_,.:;•&@‑/\\0123456789~=";
		for (size_t i=0; i < sizeof(punctuation)-1; ++i)
			m_punctuationHash[punctuation[i]] = 1;
	}
	cerr << "done." << endl;
}

GlobalLexicalModelUnlimited::~GlobalLexicalModelUnlimited(){}

void GlobalLexicalModelUnlimited::InitializeForInput( Sentence const& in )
{
//  m_input = &in;
  m_local.reset(new ThreadLocalStorage);
  m_local->input = &in;
}

void GlobalLexicalModelUnlimited::Evaluate(const TargetPhrase& targetPhrase, ScoreComponentCollection* accumulator) const
{
	const Sentence& input = *(m_local->input);
  for(size_t targetIndex = 0; targetIndex < targetPhrase.GetSize(); targetIndex++ ) {
  	string targetString = targetPhrase.GetWord(targetIndex).GetString(0); // TODO: change for other factors

  	if (m_ignorePunctuation) {
	  // check if first char is punctuation
	  char firstChar = targetString.at(0);
	  CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
	  if(charIterator != m_punctuationHash.end())
	    continue;	 
  	}

	if (m_biasFeature) {
	  stringstream feature;
	  feature << "glm_";
	  feature << targetString;
	  feature << "~";
	  feature << "**BIAS**";
	  accumulator->SparsePlusEquals(feature.str(), 1);
	}

//  	set< const Word*, WordComparer > alreadyScored; // do not score a word twice
  	StringHash alreadyScored;
  	for(size_t inputIndex = 0; inputIndex < input.GetSize(); inputIndex++ ) {
  		string inputString = input.GetWord(inputIndex).GetString(0); // TODO: change for other factors

  		if (m_ignorePunctuation) {
  			// check if first char is punctuation
  			char firstChar = inputString.at(0);
  			CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
  			if(charIterator != m_punctuationHash.end()) 
			  continue;			
  		}

  		//if ( alreadyScored.find( &inputWord ) == alreadyScored.end() ) {
  		if ( alreadyScored.find(inputString) == alreadyScored.end()) {
  			bool sourceExists, targetExists;
  			if (!m_unrestricted) {
  				sourceExists = m_vocabSource.find( inputString ) != m_vocabSource.end();
  			  targetExists = m_vocabTarget.find( targetString) != m_vocabTarget.end();
  			}

  			// no feature if vocab is in use and both words are not in restricted vocabularies
  			if (m_unrestricted || (sourceExists && targetExists)) {
  				if (m_sourceContext) {
  					// add source words right to current source word as context
  					for(size_t contextIndex = inputIndex+1; contextIndex < input.GetSize(); contextIndex++ ) {
  						string contextString = input.GetWord(contextIndex).GetString(0); // TODO: change for other factors
  						bool contextExists;
  						if (!m_unrestricted)
  							contextExists = m_vocabSource.find( contextString ) != m_vocabSource.end();
  						if (m_unrestricted || contextIndex == inputIndex+1 || contextExists) { // always add adjacent context words
  	  					stringstream feature;
  	  					feature << "glm_";
  	  					feature << targetString;
  	  					feature << "~";
  	  					feature << inputString;
  	  					feature << ",";
  	  					feature << contextString;
  	  					accumulator->SparsePlusEquals(feature.str(), 1);
  	  					alreadyScored[inputString] = 1;
  						}
  					}
  				}
  				else {
  					stringstream feature;
  					feature << "glm_";
  					feature << targetString;
  					feature << "~";
  					feature << inputString;
  					accumulator->SparsePlusEquals(feature.str(), 1);
  					//alreadyScored.insert( &inputWord );
  					alreadyScored[inputString] = 1;
  				}
  			}
  		}
  	}
  }
}

bool GlobalLexicalModelUnlimited::Load(const std::string &filePathSource,
																			 const std::string &filePathTarget)
{
  // restricted source word vocabulary
  ifstream inFileSource(filePathSource.c_str());
  if (!inFileSource)
  {
      cerr << "could not open file " << filePathSource << endl;
      return false;
  }

  std::string line;
  while (getline(inFileSource, line)) {
    m_vocabSource.insert(line);
  }

  inFileSource.close();

  // restricted target word vocabulary
  ifstream inFileTarget(filePathTarget.c_str());
  if (!inFileTarget)
  {
      cerr << "could not open file " << filePathTarget << endl;
      return false;
  }

  while (getline(inFileTarget, line)) {
    m_vocabTarget.insert(line);
  }

  inFileTarget.close();

  m_unrestricted = false;
  return true;
}

}
