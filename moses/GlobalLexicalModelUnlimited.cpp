#include "GlobalLexicalModelUnlimited.h"
#include <fstream>
#include "StaticData.h"
#include "InputFileStream.h"
#include "UserMessage.h"

using namespace std;

namespace Moses
{
GlobalLexicalModelUnlimited::GlobalLexicalModelUnlimited(const std::string &line)
:StatelessFeatureFunction("glm",ScoreProducer::unlimited)
{
  const vector<string> modelSpec = Tokenize(line);

  for (size_t i = 0; i < modelSpec.size(); i++ ) {
    bool ignorePunctuation = true, biasFeature = false, restricted = false;
    size_t context = 0;
    string filenameSource, filenameTarget;
    vector< string > factors;
    vector< string > spec = Tokenize(modelSpec[i]," ");

    // read optional punctuation and bias specifications
    if (spec.size() > 0) {
      if (spec.size() != 2 && spec.size() != 3 && spec.size() != 4 && spec.size() != 6) {
        UserMessage::Add("Format of glm feature is <factor-src>-<factor-tgt> [ignore-punct] [use-bias] "
            "[context-type] [filename-src filename-tgt]");
        //return false;
      }

      factors = Tokenize(spec[0],"-");
      if (spec.size() >= 2)
        ignorePunctuation = Scan<size_t>(spec[1]);
      if (spec.size() >= 3)
        biasFeature = Scan<size_t>(spec[2]);
      if (spec.size() >= 4)
        context = Scan<size_t>(spec[3]);
      if (spec.size() == 6) {
        filenameSource = spec[4];
        filenameTarget = spec[5];
        restricted = true;
      }
    }
    else
      factors = Tokenize(modelSpec[i],"-");

    if ( factors.size() != 2 ) {
      UserMessage::Add("Wrong factor definition for global lexical model unlimited: " + modelSpec[i]);
      //return false;
    }

    const vector<FactorType> inputFactors = Tokenize<FactorType>(factors[0],",");
    const vector<FactorType> outputFactors = Tokenize<FactorType>(factors[1],",");
    throw runtime_error("GlobalLexicalModelUnlimited should be reimplemented as a stateful feature");
    GlobalLexicalModelUnlimited* glmu = NULL; // new GlobalLexicalModelUnlimited(inputFactors, outputFactors, biasFeature, ignorePunctuation, context);

    if (restricted) {
      cerr << "loading word translation word lists from " << filenameSource << " and " << filenameTarget << endl;
      if (!glmu->Load(filenameSource, filenameTarget)) {
        UserMessage::Add("Unable to load word lists for word translation feature from files " + filenameSource + " and " + filenameTarget);
        //return false;
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

void GlobalLexicalModelUnlimited::InitializeForInput( Sentence const& in )
{
  m_local.reset(new ThreadLocalStorage);
  m_local->input = &in;
}

void GlobalLexicalModelUnlimited::Evaluate(const Hypothesis& cur_hypo, ScoreComponentCollection* accumulator) const
{
	const Sentence& input = *(m_local->input);
	const TargetPhrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();

	for(int targetIndex = 0; targetIndex < targetPhrase.GetSize(); targetIndex++ ) {
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

  	StringHash alreadyScored;
  	for(int sourceIndex = 0; sourceIndex < input.GetSize(); sourceIndex++ ) {
  		string sourceString = input.GetWord(sourceIndex).GetString(0); // TODO: change for other factors

  		if (m_ignorePunctuation) {
  			// check if first char is punctuation
  			char firstChar = sourceString.at(0);
  			CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
  			if(charIterator != m_punctuationHash.end()) 
			  continue;			
  		}

  		if ( alreadyScored.find(sourceString) == alreadyScored.end()) {
  			bool sourceExists, targetExists;
  			if (!m_unrestricted) {
  				sourceExists = m_vocabSource.find( sourceString ) != m_vocabSource.end();
  			  targetExists = m_vocabTarget.find( targetString) != m_vocabTarget.end();
  			}

  			// no feature if vocab is in use and both words are not in restricted vocabularies
  			if (m_unrestricted || (sourceExists && targetExists)) {
  				if (m_sourceContext) {
  					if (sourceIndex == 0) {
  						// add <s> trigger feature for source
	  					stringstream feature;
	  					feature << "glm_";
	  					feature << targetString;
	  					feature << "~";
	  					feature << "<s>,";
	  					feature << sourceString;
	  					accumulator->SparsePlusEquals(feature.str(), 1);
	  					alreadyScored[sourceString] = 1;
  					}

  					// add source words to the right of current source word as context
  					for(int contextIndex = sourceIndex+1; contextIndex < input.GetSize(); contextIndex++ ) {
  						string contextString = input.GetWord(contextIndex).GetString(0); // TODO: change for other factors
  						bool contextExists;
  						if (!m_unrestricted)
  							contextExists = m_vocabSource.find( contextString ) != m_vocabSource.end();

  						if (m_unrestricted || contextExists) {
  	  					stringstream feature;
  	  					feature << "glm_";
  	  					feature << targetString;
  	  					feature << "~";
  	  					feature << sourceString;
  	  					feature << ",";
  	  					feature << contextString;
  	  					accumulator->SparsePlusEquals(feature.str(), 1);
  	  					alreadyScored[sourceString] = 1;
  						}
  					}
  				}
  				else if (m_biphrase) {
  					// --> look backwards for constructing context
  					int globalTargetIndex = cur_hypo.GetSize() - targetPhrase.GetSize() + targetIndex;

  					// 1) source-target pair, trigger source word (can be discont.) and adjacent target word (bigram)
						string targetContext;
						if (globalTargetIndex > 0)
							targetContext = cur_hypo.GetWord(globalTargetIndex-1).GetString(0); // TODO: change for other factors
						else
							targetContext = "<s>";

  					if (sourceIndex == 0) {
  						string sourceTrigger = "<s>";
  						AddFeature(accumulator, alreadyScored, sourceTrigger, sourceString,
  						  										targetContext, targetString);
  					}
  					else
  						for(int contextIndex = sourceIndex-1; contextIndex >= 0; contextIndex-- ) {
  							string sourceTrigger = input.GetWord(contextIndex).GetString(0); // TODO: change for other factors
  							bool sourceTriggerExists = false;
  							if (!m_unrestricted)
  								sourceTriggerExists = m_vocabSource.find( sourceTrigger ) != m_vocabSource.end();

  							if (m_unrestricted || sourceTriggerExists)
  								AddFeature(accumulator, alreadyScored, sourceTrigger, sourceString,
  										targetContext, targetString);
  						}

  					// 2) source-target pair, adjacent source word (bigram) and trigger target word (can be discont.)
  					string sourceContext;
  					if (sourceIndex-1 >= 0)
  						sourceContext = input.GetWord(sourceIndex-1).GetString(0); // TODO: change for other factors
  					else
  						sourceContext = "<s>";

  					if (globalTargetIndex == 0) {
	  					string targetTrigger = "<s>";
	  					AddFeature(accumulator, alreadyScored, sourceContext, sourceString,
	  					  										targetTrigger, targetString);
  					}
  					else
  						for(int globalContextIndex = globalTargetIndex-1; globalContextIndex >= 0; globalContextIndex-- ) {
  							string targetTrigger = cur_hypo.GetWord(globalContextIndex).GetString(0); // TODO: change for other factors
  							bool targetTriggerExists = false;
  							if (!m_unrestricted)
  								targetTriggerExists = m_vocabTarget.find( targetTrigger ) != m_vocabTarget.end();

  							if (m_unrestricted || targetTriggerExists)
  								AddFeature(accumulator, alreadyScored, sourceContext, sourceString,
  										targetTrigger, targetString);
  						}
  				}
  				else if (m_bitrigger) {
  					// allow additional discont. triggers on both sides
  					int globalTargetIndex = cur_hypo.GetSize() - targetPhrase.GetSize() + targetIndex;

  					if (sourceIndex == 0) {
  						string sourceTrigger = "<s>";
  						bool sourceTriggerExists = true;

  						if (globalTargetIndex == 0) {
  							string targetTrigger = "<s>";
  							bool targetTriggerExists = true;

  							if (m_unrestricted || (sourceTriggerExists && targetTriggerExists))
  								AddFeature(accumulator, alreadyScored, sourceTrigger, sourceString,
  										targetTrigger, targetString);
  						}
  						else {
  							// iterate backwards over target
  							for(int globalContextIndex = globalTargetIndex-1; globalContextIndex >= 0; globalContextIndex-- ) {
  								string targetTrigger = cur_hypo.GetWord(globalContextIndex).GetString(0); // TODO: change for other factors
  								bool targetTriggerExists = false;
  								if (!m_unrestricted)
  									targetTriggerExists = m_vocabTarget.find( targetTrigger ) != m_vocabTarget.end();

  								if (m_unrestricted || (sourceTriggerExists && targetTriggerExists))
  									AddFeature(accumulator, alreadyScored, sourceTrigger, sourceString,
  											targetTrigger, targetString);
  							}
  						}
  					}
  					// iterate over both source and target
  					else {
  						// iterate backwards over source
  						for(int contextIndex = sourceIndex-1; contextIndex >= 0; contextIndex-- ) {
  							string sourceTrigger = input.GetWord(contextIndex).GetString(0); // TODO: change for other factors
  							bool sourceTriggerExists = false;
  							if (!m_unrestricted)
  								sourceTriggerExists = m_vocabSource.find( sourceTrigger ) != m_vocabSource.end();

    						if (globalTargetIndex == 0) {
    							string targetTrigger = "<s>";
    							bool targetTriggerExists = true;

    							if (m_unrestricted || (sourceTriggerExists && targetTriggerExists))
    								AddFeature(accumulator, alreadyScored, sourceTrigger, sourceString,
    										targetTrigger, targetString);
    						}
    						else {
    							// iterate backwards over target
    							for(int globalContextIndex = globalTargetIndex-1; globalContextIndex >= 0; globalContextIndex-- ) {
    								string targetTrigger = cur_hypo.GetWord(globalContextIndex).GetString(0); // TODO: change for other factors
    								bool targetTriggerExists = false;
    								if (!m_unrestricted)
    									targetTriggerExists = m_vocabTarget.find( targetTrigger ) != m_vocabTarget.end();

    								if (m_unrestricted || (sourceTriggerExists && targetTriggerExists))
    									AddFeature(accumulator, alreadyScored, sourceTrigger, sourceString,
    											targetTrigger, targetString);
    							}
    						}
  						}
						}
  				}
  				else {
  					stringstream feature;
  					feature << "glm_";
  					feature << targetString;
  					feature << "~";
  					feature << sourceString;
  					accumulator->SparsePlusEquals(feature.str(), 1);
  					//alreadyScored.insert( &inputWord );
  					alreadyScored[sourceString] = 1;
  				}
  			}
  		}
  	}
  }
}

void GlobalLexicalModelUnlimited::AddFeature(ScoreComponentCollection* accumulator,
		StringHash alreadyScored, string sourceTrigger, string sourceWord, string targetTrigger,
		string targetWord) const {
	stringstream feature;
	feature << "glm_";
	feature << targetTrigger;
	feature << ",";
	feature << targetWord;
	feature << "~";
	feature << sourceTrigger;
	feature << ",";
	feature << sourceWord;
	accumulator->SparsePlusEquals(feature.str(), 1);
	alreadyScored[sourceWord] = 1;
}

}
