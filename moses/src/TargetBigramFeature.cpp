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
  scoreIndexManager.AddScoreProducer(this);
}

bool TargetBigramFeature::Load(const std::string &filePath) 
{
  InputFileStream inFile(filePath);
  if (!inFile)
      return false;

  size_t lineNo = 0;
  std::string line;
  while (getline(inFile, line)) {
  m_wordMap[line] = lineNo++;
  }

  inFile.Close();
}

size_t TargetBigramFeature::GetNumScoreComponents() const
{
	return m_wordMap.size() * m_wordMap.size();
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


const FFState* TargetBigramFeature::EmptyHypothesisState(const InputType &/*input*/) const
{
	return NULL;
}

FFState* TargetBigramFeature::Evaluate(const Hypothesis& cur_hypo,
                                       const FFState* prev_state,
                                       ScoreComponentCollection* accumulator) const
{
	vector<string> words;
	if (cur_hypo.GetPrevHypo() != NULL) {
		size_t prevPhraseSize = cur_hypo.GetPrevHypo()->GetCurrTargetPhrase().GetSize();
		if (prevPhraseSize > 0) {
			words.push_back(cur_hypo.GetPrevHypo()->GetCurrTargetPhrase().GetWord(prevPhraseSize - 1).ToString());
		}
	}
	size_t currPhraseSize = cur_hypo.GetCurrTargetPhrase().GetSize();
	for (size_t i = 0; i < currPhraseSize; ++i) {
		words.push_back(cur_hypo.GetCurrTargetPhrase().GetWord(i).ToString());
	}
	
	for (size_t i = 1; i < words.size(); ++i) {
		map<string,size_t>::const_iterator first, second;
		if ((first = m_wordMap.find(words[i-1])) != m_wordMap.end() &&
			(second = m_wordMap.find(words[i])) != m_wordMap.end()) {
			accumulator->Assign(first->second * second->second, 1);
		}
	}

	return NULL;
}

}
