#include <sstream>
#include "SourceWordDeletionFeature.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "ChartHypothesis.h"
#include "ScoreComponentCollection.h"
#include "TranslationOption.h"
#include "UserMessage.h"
#include "Util.h"
#include "util/string_piece_hash.hh"

namespace Moses {

using namespace std;

SourceWordDeletionFeature::SourceWordDeletionFeature(const std::string &line)
:StatelessFeatureFunction("SourceWordDeletionFeature", 0, line),
m_unrestricted(true)
{
  std::cerr << "Initializing source word deletion feature.." << std::endl;

  string filename;
  for (size_t i = 0; i < m_args.size(); ++i) {
    const vector<string> &args = m_args[i];

    if (args[0] == "factor") {
      m_factorType = Scan<FactorType>(args[1]);
    }
    else if (args[0] == "path") {
      filename = args[1];
    }
    else {
      throw "Unknown argument " + args[0];
    }
  }

  // load word list for restricted feature set
  cerr << "loading source word deletion word list from " << filename << endl;
  if (!Load(filename)) {
    UserMessage::Add("Unable to load word list for source word deletion feature from file " + filename);
    //return false;
  }
}

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

void SourceWordDeletionFeature::Evaluate(const TargetPhrase &targetPhrase
                      , ScoreComponentCollection &scoreBreakdown
                      , ScoreComponentCollection &estimatedFutureScore) const
{

}

void SourceWordDeletionFeature::ComputeFeatures(const TargetPhrase& targetPhrase,
		                   	 	 	 	 	 	ScoreComponentCollection* accumulator,
		                   	 	 	 	 	 	const AlignmentInfo &alignmentInfo) const
{
  // handle special case: unknown words (they have no word alignment)
	size_t targetLength = targetPhrase.GetSize();
	size_t sourceLength = targetPhrase.GetSourcePhrase().GetSize();
	if (targetLength == 1 && sourceLength == 1 && !alignmentInfo.GetSize()) return;

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
    		const StringPiece word = w.GetFactor(m_factorType)->GetString();
    		if (word != "<s>" && word != "</s>") {
    			if (!m_unrestricted && FindStringPiece(m_vocab, word ) == m_vocab.end()) {
    				accumulator->PlusEquals(this, StringPiece("OTHER"),1);
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
