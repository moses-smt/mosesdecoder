#include "PhraseDictionaryDynSuffixArray.h"
#include "FactorCollection.h"
#include "StaticData.h"
#include "TargetPhrase.h"
#include <iomanip>

using namespace std;

namespace Moses
{
PhraseDictionaryDynSuffixArray::PhraseDictionaryDynSuffixArray(size_t numScoreComponent,
    PhraseDictionaryFeature* feature): PhraseDictionary(numScoreComponent, feature)
{
  m_biSA = new BilingualDynSuffixArray();
}

PhraseDictionaryDynSuffixArray::~PhraseDictionaryDynSuffixArray()
{
  delete m_biSA;
}

bool PhraseDictionaryDynSuffixArray::Load(const std::vector<FactorType>& input,
    const std::vector<FactorType>& output,
    string source, string target, string alignments,
    const std::vector<float> &weight,
    size_t tableLimit,
    const LMList &languageModels,
    float weightWP)
{

  m_tableLimit = tableLimit;
  m_languageModels = &languageModels;
  m_weight = weight;
  m_weightWP = weightWP;

  m_biSA->Load( input, output, source, target, alignments, weight);

  return true;
}

void PhraseDictionaryDynSuffixArray::InitializeForInput(const InputType& input)
{
  CHECK(&input == &input);
}

void PhraseDictionaryDynSuffixArray::CleanUp(const InputType &source)
{
  m_biSA->CleanUp(source);
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
    targetPhrase->SetScore(m_feature, scoreVector, ScoreComponentCollection(), m_weight, m_weightWP, *m_languageModels);
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
  CHECK(false);
  return 0;
}

}// end namepsace
