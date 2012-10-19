#include <sstream>
#include "SourceWordDeletionFeature.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "ChartHypothesis.h"
#include "ScoreComponentCollection.h"
#include "TranslationOption.h"

namespace Moses {

using namespace std;

bool SourceWordDeletionFeature::Load(const std::string &filePath) 
{
  ifstream inFile(filePath.c_str());
  if (!inFile)
  {
      cerr << "could not open file " << filePath << endl;
      return false;
  }

  std::string line;
  while (getline(inFile, line)) {
    m_vocab.insert(line);
  }

  inFile.close();

  m_unrestricted = false;
  return true;
}

void SourceWordDeletionFeature::Evaluate(
            const PhraseBasedFeatureContext& context,
            ScoreComponentCollection* accumulator) const
{
	const TargetPhrase& targetPhrase = context.GetTargetPhrase();
	const AlignmentInfo &alignmentInfo = targetPhrase.GetAlignTerm();
	ComputeFeatures(targetPhrase, accumulator, alignmentInfo);
}

void SourceWordDeletionFeature::EvaluateChart(
            const ChartBasedFeatureContext& context,
		 	 	 	 	ScoreComponentCollection* accumulator) const
{
	const AlignmentInfo &alignmentInfo = context.GetTargetPhrase().GetAlignTerm();
	ComputeFeatures(context.GetTargetPhrase(), accumulator, alignmentInfo);
}

void SourceWordDeletionFeature::ComputeFeatures(const TargetPhrase& targetPhrase,
		                   	 	 	 	 	 	ScoreComponentCollection* accumulator,
		                   	 	 	 	 	 	const AlignmentInfo &alignmentInfo) const
{
  // handle special case: unknown words (they have no word alignment)
	size_t targetLength = targetPhrase.GetSize();
	size_t sourceLength = targetPhrase.GetSourcePhrase().GetSize();
	if (targetLength == 1 && sourceLength == 1) {
		const Factor* f1 = targetPhrase.GetWord(0).GetFactor(1);
		if (f1 && f1->GetString().compare(UNKNOWN_FACTOR) == 0) {
			return;
		}
	}

  // flag aligned words
  bool aligned[16];
  CHECK(sourceLength < 16);
  for(size_t i=0; i<sourceLength; i++)
    aligned[i] = false;
  for (AlignmentInfo::const_iterator alignmentPoint = alignmentInfo.begin(); alignmentPoint != alignmentInfo.end(); alignmentPoint++)
    aligned[ alignmentPoint->first ] = true;
      
  // process unaligned source words
  for(size_t i=0; i<sourceLength; i++) {
    if (!aligned[i]) {
    	Word w = targetPhrase.GetSourcePhrase().GetWord(i);
    	if (!w.IsNonTerminal()) {
    		const string &word = w.GetFactor(m_factorType)->GetString();
    		if (word != "<s>" && word != "</s>") {
    			if (!m_unrestricted && m_vocab.find( word ) == m_vocab.end()) {
    				accumulator->PlusEquals(this,"OTHER",1);	
    			}
    			else {
    				accumulator->PlusEquals(this,word,1);
    			}
    		}
    	}
    }
  }
}

}
