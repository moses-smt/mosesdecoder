#include <sstream>
#include "TargetWordInsertionFeature.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "ChartHypothesis.h"
#include "ScoreComponentCollection.h"

namespace Moses {

using namespace std;

bool TargetWordInsertionFeature::Load(const std::string &filePath) 
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

void TargetWordInsertionFeature::Evaluate(const Hypothesis& cur_hypo,
                                          ScoreComponentCollection* accumulator) const
{
	const TargetPhrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();
	ComputeFeatures(targetPhrase, accumulator);
}

void TargetWordInsertionFeature::EvaluateChart(const ChartHypothesis& cur_hypo,
    																					 int featureID,
    																					 ScoreComponentCollection* accumulator) const
{
	const TargetPhrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();
	ComputeFeatures(targetPhrase, accumulator);
}

void TargetWordInsertionFeature::ComputeFeatures(const TargetPhrase& targetPhrase,
    																					   ScoreComponentCollection* accumulator) const
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
  const AlignmentInfo &alignment = targetPhrase.GetAlignmentInfo();
  bool aligned[16];
  CHECK(targetLength < 16);
  for(size_t i=0; i<targetLength; i++) {
    aligned[i] = false;
  }
  for (AlignmentInfo::const_iterator alignmentPoint = alignment.begin(); alignmentPoint != alignment.end(); alignmentPoint++) {
    aligned[ alignmentPoint->second ] = true;
  }

  // process unaligned target words
  for(size_t i=0; i<targetLength; i++) {
    if (!aligned[i]) {
      const string &word = targetPhrase.GetWord(i).GetFactor(m_factorType)->GetString();
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
