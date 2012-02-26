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

void WordTranslationFeature::Evaluate(const TargetPhrase& targetPhrase,
                                      ScoreComponentCollection* accumulator) const
{
  const AlignmentInfo &alignment = targetPhrase.GetAlignmentInfo();

  // process aligned words
  for (AlignmentInfo::const_iterator alignmentPoint = alignment.begin(); alignmentPoint != alignment.end(); alignmentPoint++) {
    // look up words
    const string &sourceWord = targetPhrase.GetSourcePhrase().GetWord(alignmentPoint->first).GetFactor(m_factorTypeSource)->GetString();
    const string &targetWord = targetPhrase.GetWord(alignmentPoint->second).GetFactor(m_factorTypeTarget)->GetString();
    bool sourceExists = m_vocabSource.find( sourceWord ) != m_vocabSource.end();
    bool targetExists = m_vocabTarget.find( targetWord ) != m_vocabTarget.end();
    // no feature if both words are not in restricted vocabularies
    if (m_unrestricted || (sourceExists && targetExists)) {
      // construct feature name
      stringstream featureName;
      featureName << ((sourceExists||m_unrestricted) ? sourceWord : "OTHER");
      featureName << "|";
      featureName << ((targetExists||m_unrestricted) ? targetWord : "OTHER");
      accumulator->PlusEquals(this,featureName.str(),1);
    }
  }
}

}
