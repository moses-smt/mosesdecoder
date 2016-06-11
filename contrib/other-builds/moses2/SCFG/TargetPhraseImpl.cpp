/*
 * TargetPhraseImpl.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include <stdlib.h>
#include "TargetPhraseImpl.h"
#include "../Scores.h"
#include "../System.h"
#include "../MemPool.h"
#include "../PhraseBased/Manager.h"
#include "../AlignmentInfoCollection.h"
#include "../TranslationModel/PhraseTable.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{

TargetPhraseImpl *TargetPhraseImpl::CreateFromString(MemPool &pool,
    const PhraseTable &pt, const System &system, const std::string &str)
{
  //cerr << "str=" << str << endl;
  FactorCollection &vocab = system.GetVocab();

  vector<string> toks = Tokenize(str);
  size_t size = toks.size() - 1;
  TargetPhraseImpl *ret =
      new (pool.Allocate<TargetPhraseImpl>()) TargetPhraseImpl(pool, pt, system,
          size);

  for (size_t i = 0; i < size; ++i) {
    SCFG::Word &word = (*ret)[i];
    word.CreateFromString(vocab, system, toks[i]);
  }

  // lhs
  ret->lhs.CreateFromString(vocab, system, toks.back());
  //cerr << "ret=" << *ret << endl;
  return ret;
}

TargetPhraseImpl::TargetPhraseImpl(MemPool &pool,
    const PhraseTable &pt,
    const System &system,
    size_t size)
:Moses2::TargetPhrase<SCFG::Word>(pool, pt, system, size)
,m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
,m_alignNonTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())

{
  m_scores = new (pool.Allocate<Scores>()) Scores(system, pool,
      system.featureFunctions.GetNumScores());

}

TargetPhraseImpl::~TargetPhraseImpl()
{
  // TODO Auto-generated destructor stub
}

void TargetPhraseImpl::Debug(std::ostream &out, const System &system) const
{
  lhs.Debug(out, system);
  out << " -> ";
  for (size_t i = 0; i < GetSize(); ++i) {
    const SCFG::Word &word = (*this)[i];
    word.Debug(out, system);
    out << " ";
  }
  out << "pt=" << pt.GetName();
  out << " SCORES:";
  GetScores().Debug(out, system);
  out << " ALIGN:";
  GetAlignTerm().Debug(out, system);
  out << " ";
  GetAlignNonTerm().Debug(out, system);

}

void TargetPhraseImpl::SetAlignmentInfo(const std::string &alignString)
{
  AlignmentInfo::CollType alignTerm, alignNonTerm;

  vector<string> toks = Tokenize(alignString);
  for (size_t i = 0; i < toks.size(); ++i) {
    vector<size_t> alignPair = Tokenize<size_t>(toks[i], "-");
    UTIL_THROW_IF2(alignPair.size() != 2, "Wrong alignment format");

    size_t sourcePos = alignPair[0];
    size_t targetPos = alignPair[1];

    if ((*this)[targetPos].isNonTerminal) {
      alignNonTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    } else {
      alignTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    }
  }

  SetAlignTerm(alignTerm);
  SetAlignNonTerm(alignNonTerm);
  //    cerr << "TargetPhrase::SetAlignmentInfo(const StringPiece &alignString) this:|" << *this << "|\n";

  //cerr << "alignTerm=" << alignTerm.size() << endl;
  //cerr << "alignNonTerm=" << alignNonTerm.size() << endl;

}

}
}
