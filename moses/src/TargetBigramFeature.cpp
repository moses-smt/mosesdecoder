#include "TargetBigramFeature.h"
#include "InputFileStream.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "ScoreComponentCollection.h"

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
	return m_wordSet.size() * m_wordSet.size();
}

string TargetBigramFeature::GetScoreProducerDescription() const
{
	return "TargetBigramFeature";
}

string TargetBigramFeature::GetScoreProducerWeightShortName() const
{
	return "tbf";
}

size_t TargetBigramFeature::GetNumInputScores() const
{
	return 0;
}


const FFState* TargetBigramFeature::EmptyHypothesisState(const InputType &input) const
{
	return NULL;
}

FFState* TargetBigramFeature::Evaluate(const Hypothesis& cur_hypo,
                                       const FFState* prev_state,
                                       ScoreComponentCollection* accumulator) const
{
	vector<const string &> words;
	if (cur_hypo.m_prevHypo != NULL) {
		size_t prevPhraseSize = cur_hypo.m_prevHypo->GetCurrTargetPhrase().GetSize();
		if (prevPhraseSize > 0) {
			words.push_back(cur_hypo.m_prevHypo->GetCurrTargetPhrase().GetWord(prevPhraseSize - 1));
		}
	}
	size_t currPhraseSize = cur_hypo.GetCurrTargetPhrase().GetSize();
	for (size_t i = 0; i < currPhraseSize; ++i) {
		words.push_back(cur_hypo.GetCurrTargetPhrase().GetWord(i));
	}
	
	for (size_t i = 1; i < words.size(); ++i) {
		if 
	}

	return NULL;
}

}
