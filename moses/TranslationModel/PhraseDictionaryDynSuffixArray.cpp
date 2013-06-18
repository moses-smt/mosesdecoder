#include "moses/TranslationModel/PhraseDictionaryDynSuffixArray.h"
#include "moses/FactorCollection.h"
#include "moses/StaticData.h"
#include "moses/TargetPhrase.h"
#include <iomanip>

using namespace std;

namespace Moses
{
PhraseDictionaryDynSuffixArray::PhraseDictionaryDynSuffixArray(const std::string &line)
  :PhraseDictionary("PhraseDictionaryDynSuffixArray", line)
  ,m_biSA(new BilingualDynSuffixArray())
{
  size_t ind = 0;
  while (ind < m_args.size()) {
    vector<string> &args = m_args[ind];
    bool consumed = SetParameter(args[0], args[1]);
    if (consumed) {
      m_args.erase(m_args.begin() + ind);
    } else {
      ++ind;
    }
  }
  CHECK(m_args.size() == 0);

}

PhraseDictionaryDynSuffixArray::~PhraseDictionaryDynSuffixArray()
{
  delete m_biSA;
}

void PhraseDictionaryDynSuffixArray::Load()
{
  SetFeaturesToApply();

  const StaticData &staticData = StaticData::Instance();
  vector<float> weight = staticData.GetWeights(this);

  m_biSA->Load( m_input, m_output, m_source, m_target, m_alignments, weight);
}

const TargetPhraseCollection *PhraseDictionaryDynSuffixArray::GetTargetPhraseCollection(const Phrase& src) const
{
  TargetPhraseCollection *ret = new TargetPhraseCollection();
  std::vector< std::pair< Scores, TargetPhrase*> > trg;
  // extract target phrases and their scores from suffix array
  m_biSA->GetTargetPhrasesByLexicalWeight( src, trg);

  std::vector< std::pair< Scores, TargetPhrase*> >::iterator itr;
  for(itr = trg.begin(); itr != trg.end(); ++itr) {
    Scores scoreVector = itr->first;
    TargetPhrase *targetPhrase = itr->second;
    //std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),NegateScore);
    std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),FloorScore);

    targetPhrase->GetScoreBreakdown().Assign(this, scoreVector);
    targetPhrase->Evaluate(src);

    //cout << *targetPhrase << "\t" << std::setprecision(8) << scoreVector[2] << endl;
    ret->Add(targetPhrase);
  }
  ret->NthElement(m_tableLimit); // sort the phrases for the dcoder
  return ret;
}

void PhraseDictionaryDynSuffixArray::insertSnt(string& source, string& target, string& alignment)
{
  m_biSA->addSntPair(source, target, alignment); // insert sentence pair into suffix arrays
  //StaticData::Instance().ClearTransOptionCache(); // clear translation option cache
}
void PhraseDictionaryDynSuffixArray::deleteSnt(unsigned /* idx */, unsigned /* num2Del */)
{
  // need to implement --
}

ChartRuleLookupManager *PhraseDictionaryDynSuffixArray::CreateRuleLookupManager(const InputType&, const ChartCellCollectionBase&)
{
  throw "Chart decoding not supported by PhraseDictionaryDynSuffixArray";
}

bool PhraseDictionaryDynSuffixArray::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "source") {
    m_source = value;
  } else if (key == "target") {
    m_target = value;
  } else if (key == "alignment") {
    m_alignments = value;
  } else {
    return PhraseDictionary::SetParameter(key, value);
  }
  return true;
}

}// end namepsace
