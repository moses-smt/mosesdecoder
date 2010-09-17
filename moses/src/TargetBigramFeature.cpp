#include "TargetBigramFeature.h"
#include "InputFileStream.h"

namespace Moses {

using namespace std;

TargetBigramFeature::TargetBigramFeature(ScoreIndexManager &scoreIndexManager)
{
}

bool TargetBigramFeature::Load(const std::string &filePath) 
{
  InputFileStream inFile(filePath);
  if (!inFile)
      return false;

  std::string line;
  while (getline(inFile, line)) {
      m_wordSet.insert(line);
  }

  inFile.Close();
}

//static size_t TargetBigramFeature::GetWordIndex(const std::string &word) const
//{
 // m_wordSet.find(word);
//}

size_t TargetBigramFeature::GetNumScoreComponents() const
{
	// TODO
	return 0;
}

string TargetBigramFeature::GetScoreProducerDescription() const
{
	// TODO
	return "";
}

string TargetBigramFeature::GetScoreProducerWeightShortName() const
{
	// TODO
	return "";
}

size_t TargetBigramFeature::GetNumInputScores() const
{
	// TODO
	return 0;
}


const FFState* TargetBigramFeature::EmptyHypothesisState(const InputType &input) const
{
	// TODO
	return 0;
}

FFState* TargetBigramFeature::Evaluate(const Hypothesis& cur_hypo,
                                       const FFState* prev_state,
                                       ScoreComponentCollection* accumulator) const
{
	// TODO
	return 0;
}

}
