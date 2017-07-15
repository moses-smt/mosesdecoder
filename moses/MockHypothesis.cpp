/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <boost/test/unit_test.hpp>
#include "MockHypothesis.h"
#include "TranslationOption.h"
#include "TranslationTask.h"
#include "Bitmaps.h"

using namespace Moses;
using namespace std;

namespace MosesTest
{

MockHypothesisGuard
::MockHypothesisGuard
( const string& sourceSentence,
  const vector<Alignment>& alignments,
  const vector<string>& targetSegments)
  : m_initialTransOpt(), m_wp("WordPenalty"),
    m_uwp("UnknownWordPenalty"), m_dist("Distortion")
{
  BOOST_CHECK_EQUAL(alignments.size(), targetSegments.size());
  AllOptions::ptr opts(new AllOptions(*StaticData::Instance().options()));
  m_sentence.reset(new Sentence(opts, 0, sourceSentence));
  m_ttask = TranslationTask::create(m_sentence);
  m_manager.reset(new Manager(m_ttask));

  //Initial empty hypothesis
  Bitmaps bitmaps(m_sentence.get()->GetSize(),
                  m_sentence.get()->m_sourceCompleted);
  m_manager->ResetSentenceStats(*m_sentence);

  const Bitmap &initBitmap = bitmaps.GetInitialBitmap();
  m_hypothesis = new Hypothesis(*m_manager, *m_sentence, m_initialTransOpt,
                                initBitmap, m_manager->GetNextHypoId());

  //create the chain
  vector<Alignment>::const_iterator ai = alignments.begin();
  vector<string>::const_iterator ti = targetSegments.begin();
  for (; ti != targetSegments.end() && ai != alignments.end(); ++ti,++ai) {
    Hypothesis* prevHypo = m_hypothesis;
    Range range(ai->first,ai->second);
    const Bitmap &newBitmap = bitmaps.GetBitmap(prevHypo->GetWordsBitmap(), range);
    m_targetPhrases.push_back(TargetPhrase(NULL));
    vector<FactorType> const& factors = opts->output.factor_order;
    m_targetPhrases.back().CreateFromString(Input, factors, *ti, NULL);
    m_toptions.push_back(new TranslationOption
                         (range,m_targetPhrases.back()));
    m_hypothesis = new Hypothesis(*prevHypo, *m_toptions.back(), newBitmap,
                                  m_manager->GetNextHypoId());
  }


}

MockHypothesisGuard::~MockHypothesisGuard()
{
  RemoveAllInColl(m_toptions);
  while (m_hypothesis) {
    Hypothesis* prevHypo = const_cast<Hypothesis*>(m_hypothesis->GetPrevHypo());
    delete m_hypothesis;
    m_hypothesis = prevHypo;
  }
}

HypothesisFixture::HypothesisFixture()
{
  string source = "je ne sais pas . ";
  vector<string> target;
  vector<Alignment> alignments;
  m_empty.reset(new MockHypothesisGuard(source,alignments,target));
  target.push_back("i");
  target.push_back("do not");
  alignments.push_back(Alignment(0,0));
  alignments.push_back(Alignment(3,3));
  m_partial.reset(new MockHypothesisGuard(source,alignments,target));
  target.push_back("know");
  target.push_back(".");
  alignments.push_back(Alignment(1,2));
  alignments.push_back(Alignment(4,4));
  m_full.reset(new MockHypothesisGuard(source,alignments,target));
}

}

