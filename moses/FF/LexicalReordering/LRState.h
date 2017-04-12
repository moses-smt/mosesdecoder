// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#pragma once
#include <vector>
#include <string>

#include "moses/Hypothesis.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Range.h"
#include "moses/Bitmap.h"
#include "moses/TranslationOption.h"
#include "moses/FF/FFState.h"
#include "LRModel.h"

namespace Moses
{

//! Abstract class for lexical reordering model states
class LRState : public FFState
{
public:

  typedef LRModel::ReorderingType ReorderingType;

  virtual
  LRState*
  Expand(const TranslationOption& hypo, const InputType& input,
         ScoreComponentCollection* scores) const = 0;

  static
  LRState*
  CreateLRState(const std::vector<std::string>& config,
                LRModel::Direction dir,
                const InputType &input);

protected:

  const LRModel& m_configuration;

  // The following is the true direction of the object, which can be
  // Backward or Forward even if the Configuration has Bidirectional.
  LRModel::Direction m_direction;
  size_t m_offset;
  //forward scores are conditioned on prev option, so need to remember it
  const TranslationOption *m_prevOption;

  inline
  LRState(const LRState *prev,
          const TranslationOption &topt)
    : m_configuration(prev->m_configuration)
    , m_direction(prev->m_direction)
    , m_offset(prev->m_offset)
    , m_prevOption(&topt)
  { }

  inline
  LRState(const LRModel &config,
          LRModel::Direction dir,
          size_t offset)
    : m_configuration(config)
    , m_direction(dir)
    , m_offset(offset)
    , m_prevOption(NULL)
  { }

  // copy the right scores in the right places, taking into account
  // forward/backward, offset, collapse
  void
  CopyScores(ScoreComponentCollection* scores,
             const TranslationOption& topt,
             const InputType& input, ReorderingType reoType) const;

  int
  ComparePrevScores(const TranslationOption *other) const;
};





}

