#include <sstream>
#include "WordTranslationFeature.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "ScoreComponentCollection.h"

namespace Moses {

using namespace std;

bool WordTranslationFeature::Load(const std::string &filePathSource, const std::string &filePathTarget) 
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

void WordTranslationFeature::InitializeForInput( Sentence const& in )
{
  m_local.reset(new ThreadLocalStorage);
  m_local->input = &in;
}

//void WordTranslationFeature::Evaluate(const TargetPhrase& targetPhrase, ScoreComponentCollection* accumulator) const
FFState* WordTranslationFeature::Evaluate(const Hypothesis& cur_hypo, const FFState* prev_state, ScoreComponentCollection* accumulator) const
{
	const Sentence& input = *(m_local->input);
	const TargetPhrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();
  const AlignmentInfo &alignment = targetPhrase.GetAlignmentInfo();

  // process aligned words
  for (AlignmentInfo::const_iterator alignmentPoint = alignment.begin(); alignmentPoint != alignment.end(); alignmentPoint++) {
    // look up words
  	const Phrase& sourcePhrase = targetPhrase.GetSourcePhrase();
  	int sourceIndex = alignmentPoint->first;
  	int targetIndex = alignmentPoint->second;
    const string &sourceWord = sourcePhrase.GetWord(sourceIndex).GetFactor(m_factorTypeSource)->GetString();
    const string &targetWord = targetPhrase.GetWord(targetIndex).GetFactor(m_factorTypeTarget)->GetString();
    bool sourceExists = m_vocabSource.find( sourceWord ) != m_vocabSource.end();
    bool targetExists = m_vocabTarget.find( targetWord ) != m_vocabTarget.end();
    // no feature if both words are not in restricted vocabularies
    if (m_unrestricted || (sourceExists && targetExists)) {
    	if (m_sourceContext) {
    		// TODO: need to determine position of the source phrase in the global input!!
			cerr << "not implemented!" << endl;
			exit(0);
    		size_t globalSourceIndex = input.GetSize() - sourcePhrase.GetSize() + sourceIndex;
    		if (globalSourceIndex == 0) {
    			// add <s> trigger feature for source
    			stringstream feature;
    			feature << "wt_";
    			feature << targetWord;
    			feature << "~";
    			feature << "<s>,";
    			feature << sourceWord;
    			accumulator->SparsePlusEquals(feature.str(), 1);
    			cerr << "feature: " << feature.str() << endl;
			}

    		// add source words to the right of current source word as context
			for(size_t contextIndex = globalSourceIndex+1; contextIndex < input.GetSize(); contextIndex++ ) {
				string sourceTrigger = input.GetWord(contextIndex).GetFactor(m_factorTypeSource)->GetString();
				bool sourceTriggerExists = false;
				if (!m_unrestricted)
					sourceTriggerExists = m_vocabSource.find( sourceTrigger ) != m_vocabSource.end();
				if (contextIndex == globalSourceIndex+1)
					sourceTriggerExists = true; // always add adjacent context words

				if (m_unrestricted || sourceTriggerExists) {
	    			stringstream feature;
	    			feature << "wt_";
	    			feature << targetWord;
	    			feature << "~";
	    			feature << sourceWord;
	    			feature << ",";
	    			feature << sourceTrigger;
	    			accumulator->SparsePlusEquals(feature.str(), 1);
	    			cerr << "feature: " << feature.str() << endl;
				}
			}
    	}
    	else if (m_biphrase) {
				// allow additional discont. triggers on one of the sides, bigram on the other side
				int globalTargetIndex = cur_hypo.GetSize() - targetPhrase.GetSize() + targetIndex;
				// TODO: need to determine position of the source phrase in the global input!!
				cerr << "not implemented!" << endl;
				exit(0);
				int globalSourceIndex = input.GetSize() - sourcePhrase.GetSize() + sourceIndex;

				// 1) source-target pair, trigger source word (can be discont.) and adjacent target word (bigram)
				string targetContext;
				if (globalTargetIndex > 0)
					targetContext = cur_hypo.GetWord(globalTargetIndex-1).GetFactor(m_factorTypeTarget)->GetString();
				else
					targetContext = "<s>";

				if (globalSourceIndex == 0) {
					string sourceTrigger = "<s>";
					AddFeature(accumulator, sourceTrigger, sourceWord,
					  										targetContext, targetWord);
				}
				else
					for(int contextIndex = globalSourceIndex-1; contextIndex >= 0; contextIndex-- ) {
						string sourceTrigger = input.GetWord(contextIndex).GetFactor(m_factorTypeSource)->GetString();
						bool sourceTriggerExists = false;
						if (!m_unrestricted)
							sourceTriggerExists = m_vocabSource.find( sourceTrigger ) != m_vocabSource.end();
						if (contextIndex == globalSourceIndex-1)
							sourceTriggerExists = true; // always add adjacent context words

						if (m_unrestricted || sourceTriggerExists)
							AddFeature(accumulator, sourceTrigger, sourceWord,
									targetContext, targetWord);
					}

				// 2) source-target pair, adjacent source word (bigram) and trigger target word (can be discont.)
				string sourceContext;
				if (globalSourceIndex-1 >= 0)
					sourceContext = input.GetWord(globalSourceIndex-1).GetFactor(m_factorTypeSource)->GetString();
				else
					sourceContext = "<s>";

				if (globalTargetIndex == 0) {
					string targetTrigger = "<s>";
					AddFeature(accumulator, sourceContext, sourceWord,
					  										targetTrigger, targetWord);
				}
				else
					for(int globalContextIndex = globalTargetIndex-1; globalContextIndex >= 0; globalContextIndex-- ) {
						string targetTrigger = cur_hypo.GetWord(globalContextIndex).GetFactor(m_factorTypeTarget)->GetString();
						bool targetTriggerExists = false;
						if (!m_unrestricted)
							targetTriggerExists = m_vocabTarget.find( targetTrigger ) != m_vocabTarget.end();
						if (globalContextIndex == targetIndex-1)
							targetTriggerExists = true; // always add adjacent context words

						if (m_unrestricted || targetTriggerExists)
							AddFeature(accumulator, sourceContext, sourceWord,
									targetTrigger, targetWord);
					}
    	}
    	else if (m_bitrigger) {
				// allow additional discont. triggers on both sides
				int globalTargetIndex = cur_hypo.GetSize() - targetPhrase.GetSize() + targetIndex;
				// TODO: need to determine position of the source phrase in the global input!!
				cerr << "not implemented!" << endl;
				exit(0);
				int globalSourceIndex =input.GetSize() - sourcePhrase.GetSize() + sourceIndex;

				if (globalSourceIndex == 0) {
					string sourceTrigger = "<s>";
					bool sourceTriggerExists = true;

					if (globalTargetIndex == 0) {
						string targetTrigger = "<s>";
						bool targetTriggerExists = true;

						if (m_unrestricted || (sourceTriggerExists && targetTriggerExists))
							AddFeature(accumulator, sourceTrigger, sourceWord, targetTrigger, targetWord);
					}
					else {
						// iterate backwards over target
						for(int globalContextIndex = globalTargetIndex-1; globalContextIndex >= 0; globalContextIndex-- ) {
							string targetTrigger = cur_hypo.GetWord(globalContextIndex).GetFactor(m_factorTypeTarget)->GetString();
							bool targetTriggerExists = false;
							if (!m_unrestricted)
								targetTriggerExists = m_vocabTarget.find( targetTrigger ) != m_vocabTarget.end();
							if (globalContextIndex == globalTargetIndex-1)
								targetTriggerExists = true; // always add adjacent context words

							if (m_unrestricted || (sourceTriggerExists && targetTriggerExists))
								AddFeature(accumulator, sourceTrigger, sourceWord, targetTrigger, targetWord);
						}
					}
				}
				// iterate over both source and target
				else {
					// iterate backwards over source
					for(int contextIndex = globalSourceIndex-1; contextIndex >= 0; contextIndex-- ) {
						string sourceTrigger = input.GetWord(contextIndex).GetFactor(m_factorTypeSource)->GetString();
						bool sourceTriggerExists = false;
						if (!m_unrestricted)
							sourceTriggerExists = m_vocabSource.find( sourceTrigger ) != m_vocabSource.end();
						if (contextIndex == globalSourceIndex-1)
							sourceTriggerExists = true; // always add adjacent context words

						if (globalTargetIndex == 0) {
							string targetTrigger = "<s>";
							bool targetTriggerExists = true;

							if (m_unrestricted || (sourceTriggerExists && targetTriggerExists))
								AddFeature(accumulator, sourceTrigger, sourceWord, targetTrigger, targetWord);
						}
						else {
							// iterate backwards over target
							for(int globalContextIndex = globalTargetIndex-1; globalContextIndex >= 0; globalContextIndex-- ) {
								string targetTrigger = cur_hypo.GetWord(globalContextIndex).GetFactor(m_factorTypeTarget)->GetString();
								bool targetTriggerExists = false;
								if (!m_unrestricted)
									targetTriggerExists = m_vocabTarget.find( targetTrigger ) != m_vocabTarget.end();
								if (globalContextIndex == globalTargetIndex-1)
									targetTriggerExists = true; // always add adjacent context words

								if (m_unrestricted || (sourceTriggerExists && targetTriggerExists))
									AddFeature(accumulator, sourceTrigger, sourceWord, targetTrigger, targetWord);
							}
						}
					}
				}
    	}
    	else {
    		// construct feature name
    		stringstream featureName;
    		featureName << ((sourceExists||m_unrestricted) ? sourceWord : "OTHER");
    		featureName << "~";
    		featureName << ((targetExists||m_unrestricted) ? targetWord : "OTHER");
    		accumulator->PlusEquals(this,featureName.str(),1);
    	}
    }
  }

	return new DummyState();
}

void WordTranslationFeature::AddFeature(ScoreComponentCollection* accumulator, string sourceTrigger,
		string sourceWord, string targetTrigger, string targetWord) const {
	stringstream feature;
	feature << "wt_";
	feature << targetTrigger;
	feature << ",";
	feature << targetWord;
	feature << "~";
	feature << sourceTrigger;
	feature << ",";
	feature << sourceWord;
	accumulator->SparsePlusEquals(feature.str(), 1);
}

}
