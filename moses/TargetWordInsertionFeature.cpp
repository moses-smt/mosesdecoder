#include <sstream>
#include "TargetWordInsertionFeature.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "ChartHypothesis.h"
#include "ScoreComponentCollection.h"
#include "TranslationOption.h"
#include "UserMessage.h"
#include "util/string_piece_hash.hh"

namespace Moses {

using namespace std;

TargetWordInsertionFeature::TargetWordInsertionFeature(const std::string &line)
:StatelessFeatureFunction("TargetWordInsertionFeature", FeatureFunction::unlimited, line),
m_unrestricted(true)
{
  std::cerr << "Initializing target word insertion feature.." << std::endl;

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
  if (filename != "") {
    cerr << "loading target word insertion word list from " << filename << endl;
    if (!Load(filename)) {
      UserMessage::Add("Unable to load word list for target word insertion feature from file " + filename);
      //return false;
    }
  }

}

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

void TargetWordInsertionFeature::Evaluate(
                      const PhraseBasedFeatureContext& context,
                      ScoreComponentCollection* accumulator) const
{
	const TargetPhrase& targetPhrase = context.GetTargetPhrase();
	const AlignmentInfo &alignmentInfo = targetPhrase.GetAlignTerm();
	ComputeFeatures(targetPhrase, accumulator, alignmentInfo);
}

void TargetWordInsertionFeature::EvaluateChart(
            const ChartBasedFeatureContext& context,
						ScoreComponentCollection* accumulator) const
{
	const TargetPhrase& targetPhrase = context.GetTargetPhrase();
	const AlignmentInfo &alignmentInfo = context.GetTargetPhrase().GetAlignTerm();
	ComputeFeatures(targetPhrase, accumulator, alignmentInfo);
}

void TargetWordInsertionFeature::ComputeFeatures(const TargetPhrase& targetPhrase,
    											 ScoreComponentCollection* accumulator,
    											 const AlignmentInfo &alignmentInfo) const
{
  // handle special case: unknown words (they have no word alignment)
  size_t targetLength = targetPhrase.GetSize();
  size_t sourceLength = targetPhrase.GetSourcePhrase().GetSize();
  if (targetLength == 1 && sourceLength == 1 && !alignmentInfo.GetSize()) return;

  // flag aligned words
  bool aligned[16];
  CHECK(targetLength < 16);
  for(size_t i=0; i<targetLength; i++) {
    aligned[i] = false;
  }
  for (AlignmentInfo::const_iterator alignmentPoint = alignmentInfo.begin(); alignmentPoint != alignmentInfo.end(); alignmentPoint++) {
    aligned[ alignmentPoint->second ] = true;
  }

  // process unaligned target words
  for(size_t i=0; i<targetLength; i++) {
    if (!aligned[i]) {
      Word w = targetPhrase.GetWord(i);
      if (!w.IsNonTerminal()) {
    	const StringPiece word = w.GetFactor(m_factorType)->GetString();
    	if (word != "<s>" && word != "</s>") {
      	  if (!m_unrestricted && FindStringPiece(m_vocab, word ) == m_vocab.end()) {
      		accumulator->PlusEquals(this,StringPiece("OTHER"),1);
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
