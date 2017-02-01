/*
 * Word.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/functional/hash.hpp>
#include "Word.h"
#include "Hypothesis.h"
#include "ActiveChart.h"
#include "TargetPhraseImpl.h"
#include "Sentence.h"
#include "../legacy/Util2.h"
#include "../System.h"
#include "../AlignmentInfo.h"
#include "../ManagerBase.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{
Word::Word(const SCFG::Word &copy)
  :Moses2::Word(copy)
  ,isNonTerminal(copy.isNonTerminal)
{
}

void Word::CreateFromString(FactorCollection &vocab,
                            const System &system,
                            const std::string &str)
{
  vector<string> toks;

  if (str[0] == '[' && str[str.size() - 1] == ']') {
    isNonTerminal = true;

    size_t startPos = str.find("[", 1);
    bool doubleNT = startPos != string::npos;

    if (doubleNT) {
      assert(startPos != string::npos);
      string str2 = str.substr(startPos + 1, str.size() - startPos - 2);
      toks = Tokenize(str2, "|");
    } else {
      string str2 = str.substr(1, str.size() - 2);
      toks = Tokenize(str2, "|");
    }
  } else {
    isNonTerminal = false;
    toks = Tokenize(str, "|");
  }

  // parse string
  for (size_t i = 0; i < toks.size(); ++i) {
    const string &tok = toks[i];
    //cerr << "tok=" << tok << endl;

    const Factor *factor = vocab.AddFactor(tok, system, isNonTerminal);
    m_factors[i] = factor;
  }
}

size_t Word::hash() const
{
  size_t ret = Moses2::Word::hash();
  boost::hash_combine(ret, isNonTerminal);
  return ret;
}

size_t Word::hash(const std::vector<FactorType> &factors) const
{
  size_t seed = isNonTerminal;
  for (size_t i = 0; i < factors.size(); ++i) {
    FactorType factorType = factors[i];
    const Factor *factor = m_factors[factorType];
    boost::hash_combine(seed, factor);
  }
  return seed;
}

void Word::OutputToStream(const System &system, std::ostream &out) const
{
  if (isNonTerminal) {
    out << "[";
  }
  Moses2::Word::OutputToStream(system, out);
  if (isNonTerminal) {
    out << "]";
  }
}

void Word::OutputToStream(
  const ManagerBase &mgr,
  size_t targetPos,
  const SCFG::Hypothesis &hypo,
  std::ostream &out) const
{
  const SCFG::TargetPhraseImpl &tp = hypo.GetTargetPhrase();
  const SCFG::SymbolBind &symbolBind = hypo.GetSymbolBind();

  bool outputWord = true;
  if (mgr.system.options.input.placeholder_factor != NOT_FOUND) {
    const AlignmentInfo &alignInfo = tp.GetAlignTerm();
    std::set<size_t> sourceAligns = alignInfo.GetAlignmentsForTarget(targetPos);
    if (sourceAligns.size() == 1) {
      size_t sourcePos = *sourceAligns.begin();
      /*
      cerr << "sourcePos=" << sourcePos << endl;
      cerr << "tp=" << tp.Debug(mgr.system) << endl;
      cerr << "m_symbolBind=" << symbolBind.Debug(mgr.system) << endl;
      */
      assert(sourcePos < symbolBind.GetSize());
      const Range &inputRange = symbolBind.coll[sourcePos].GetRange();
      assert(inputRange.GetNumWordsCovered() == 1);
      const SCFG::Sentence &sentence = static_cast<const SCFG::Sentence &>(mgr.GetInput());
      const SCFG::Word &sourceWord = sentence[inputRange.GetStartPos()];
      const Factor *factor = sourceWord[mgr.system.options.input.placeholder_factor];
      if (factor) {
        out << factor->GetString();
        outputWord = false;
      }
    }
  }

  if (outputWord) {
    OutputToStream(mgr.system, out);
  }
}

std::string Word::Debug(const System &system) const
{
  stringstream out;
  if (isNonTerminal) {
    out << "[";
  }
  out << Moses2::Word::Debug(system);
  if (isNonTerminal) {
    out << "]";
  }
  return out.str();
}

}
}

