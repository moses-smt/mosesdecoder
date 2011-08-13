#include <sstream>
#include "SourceWordDeletionFeature.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "ScoreComponentCollection.h"

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

void SourceWordDeletionFeature::Evaluate(const TargetPhrase& targetPhrase,
                                          ScoreComponentCollection* accumulator) const
{
  // handle special case: unknown words (they have no word alignment)
  size_t targetLength = targetPhrase.GetSize();
  size_t sourceLength = targetPhrase.GetSourcePhrase().GetSize();
  if (targetLength == 1 && sourceLength == 1)
    return;

  // flag aligned words
  const AlignmentInfo &alignment = targetPhrase.GetAlignmentInfo();
  bool aligned[16];
  assert(sourceLength < 16);
  for(size_t i=0; i<sourceLength; i++) {
    aligned[i] = false;
  }
  for (AlignmentInfo::const_iterator alignmentPoint = alignment.begin(); alignmentPoint != alignment.end(); alignmentPoint++) {
    aligned[ alignmentPoint->first ] = true;
  }
    
  // process unaligned source words
  for(size_t i=0; i<sourceLength; i++) {
    if (!aligned[i]) {
      const string &word = targetPhrase.GetSourcePhrase().GetWord(i).GetFactor(m_factorType)->GetString();
      stringstream featureName;
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
