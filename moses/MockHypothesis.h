// -*- c++ -*-
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

#ifndef _MOCK_HYPOTHESIS_
#define _MOCK_HYPOTHESIS_

#include <memory>
#include <vector>

#include "moses/FF/UnknownWordPenaltyProducer.h"
#include "moses/FF/DistortionScoreProducer.h"
#include "moses/FF/WordPenaltyProducer.h"
#include "Hypothesis.h"
#include "Manager.h"
#include "TranslationOption.h"

namespace MosesTest
{

//
// Construct a hypothesis with arbitrary source and target phrase
// sequences. Useful for testing feature functions.
//

typedef std::pair<size_t,size_t> Alignment; //(first,last) in source

class MockHypothesisGuard
{
public:
  /** Creates a phrase-based hypothesis.
  */
  MockHypothesisGuard
  ( const std::string& sourceSentence,
    const std::vector<Alignment>& alignments,
    const std::vector<std::string>& targetSegments);

  Moses::Hypothesis* operator*() const {
    return m_hypothesis;
  }

  /** Destroy the hypothesis chain */
  ~MockHypothesisGuard();

private:
  Moses::TranslationOption m_initialTransOpt;
  boost::shared_ptr<Moses::Sentence> m_sentence;
  Moses::WordPenaltyProducer m_wp;
  Moses::UnknownWordPenaltyProducer m_uwp;
  Moses::DistortionScoreProducer m_dist;
  boost::shared_ptr<Moses::Manager> m_manager;
  boost::shared_ptr<Moses::TranslationTask> m_ttask;
  Moses::Hypothesis* m_hypothesis;
  std::vector<Moses::TargetPhrase> m_targetPhrases;
  std::vector<Moses::TranslationOption*> m_toptions;
};

class HypothesisFixture
{
public:
  HypothesisFixture();
  const Moses::Hypothesis* empty() {
    return **m_empty;
  }
  const Moses::Hypothesis* partial() {
    return **m_partial;
  }
  const Moses::Hypothesis* full() {
    return **m_full;
  }

private:
  std::auto_ptr<MockHypothesisGuard> m_empty;
  std::auto_ptr<MockHypothesisGuard> m_partial;
  std::auto_ptr<MockHypothesisGuard> m_full;
};


}

#endif
