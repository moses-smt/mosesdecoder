#include "moses/TranslationModel/PhraseDictionaryDynSuffixArray.h"
#include "moses/FactorCollection.h"
#include "moses/StaticData.h"
#include "moses/TargetPhrase.h"
#include <iomanip>
#include <boost/foreach.hpp>
using namespace std;

namespace Moses
{
PhraseDictionaryDynSuffixArray::
PhraseDictionaryDynSuffixArray(const std::string &line)
  : PhraseDictionary(line)
  ,m_biSA(new BilingualDynSuffixArray())
{
  ReadParameters();
}


void
PhraseDictionaryDynSuffixArray::
Load()
{
  SetFeaturesToApply();

  vector<float> weight = StaticData::Instance().GetWeights(this);
  m_biSA->Load(m_input, m_output, m_source, m_target, m_alignments, weight);
}

PhraseDictionaryDynSuffixArray::
~PhraseDictionaryDynSuffixArray()
{
  delete m_biSA;
}

void
PhraseDictionaryDynSuffixArray::
SetParameter(const std::string& key, const std::string& value)
{
  if (key == "source") {
    m_source = value;
  } else if (key == "target") {
    m_target = value;
  } else if (key == "alignment") {
    m_alignments = value;
  } else {
    PhraseDictionary::SetParameter(key, value);
  }
}

const TargetPhraseCollection*
PhraseDictionaryDynSuffixArray::
GetTargetPhraseCollectionLEGACY(const Phrase& src) const
{
  typedef map<SAPhrase, vector<float> >::value_type pstat_entry;
  map<SAPhrase, vector<float> > pstats; // phrase (pair) statistics
  m_biSA->GatherCands(src,pstats);

  TargetPhraseCollection *ret = new TargetPhraseCollection();
  BOOST_FOREACH(pstat_entry & e, pstats) {
    TargetPhrase* tp = m_biSA->GetMosesFactorIDs(e.first, src);
    tp->GetScoreBreakdown().Assign(this,e.second);
    tp->Evaluate(src);
    ret->Add(tp);
  }
  // return ret;
  // TargetPhraseCollection *ret = new TargetPhraseCollection();
  // std::vector< std::pair< Scores, TargetPhrase*> > trg;
  //
  // // extract target phrases and their scores from suffix array
  // m_biSA->GetTargetPhrasesByLexicalWeight(src, trg);
  //
  // std::vector< std::pair< Scores, TargetPhrase*> >::iterator itr;
  // for(itr = trg.begin(); itr != trg.end(); ++itr) {
  //   Scores scoreVector = itr->first;
  //   TargetPhrase *targetPhrase = itr->second;
  //   std::transform(scoreVector.begin(),scoreVector.end(),
  // 		   scoreVector.begin(),FloorScore);
  //   targetPhrase->GetScoreBreakdown().Assign(this, scoreVector);
  //   targetPhrase->Evaluate();
  //   ret->Add(targetPhrase);
  // }
  ret->NthElement(m_tableLimit); // sort the phrases for the decoder
  return ret;
}

void
PhraseDictionaryDynSuffixArray::
insertSnt(string& source, string& target, string& alignment)
{
  m_biSA->addSntPair(source, target, alignment); // insert sentence pair into suffix arrays
  //StaticData::Instance().ClearTransOptionCache(); // clear translation option cache
}

void
PhraseDictionaryDynSuffixArray::
deleteSnt(unsigned /* idx */, unsigned /* num2Del */)
{
  // need to implement --
}

ChartRuleLookupManager*
PhraseDictionaryDynSuffixArray::
CreateRuleLookupManager(const ChartParser &, const ChartCellCollectionBase&)
{
  UTIL_THROW(util::Exception, "SCFG decoding not supported with dynamic suffix array");
  return 0;
}

}// end namepsace
