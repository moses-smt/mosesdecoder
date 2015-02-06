// $Id: PhraseDictionaryMemory.cpp 2477 2009-08-07 16:47:54Z bhaddow $
// vim:tabstop=2

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
#ifndef moses_DecodeFeature
#define moses_DecodeFeature

#include <vector>

#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/FactorTypeSet.h"
#include "moses/TypeDef.h"

namespace Moses
{
class DecodeStep;
class DecodeGraph;

/**
  * Baseclass for phrase-table or generation table feature function
 **/
class DecodeFeature : public StatelessFeatureFunction
{

public:
  DecodeFeature(const std::string &line);

  DecodeFeature(size_t numScoreComponents
                , const std::string &line);

  DecodeFeature(size_t numScoreComponents
                , const std::vector<FactorType> &input
                , const std::vector<FactorType> &output
                , const std::string &line);

  //! returns output factor types as specified by the ini file
  const FactorMask& GetOutputFactorMask() const;

  //! returns input factor types as specified by the ini file
  const FactorMask& GetInputFactorMask() const;

  const std::vector<FactorType>& GetInput() const;
  const std::vector<FactorType>& GetOutput() const;

  bool IsUseable(const FactorMask &mask) const;
  void SetParameter(const std::string& key, const std::string& value);

  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const {
  }
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                           ScoreComponentCollection* accumulator) const {
  }
  void EvaluateWhenApplied(const Syntax::SHyperedge &hyperedge,
                           ScoreComponentCollection* accumulator) const {
  }
  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedFutureScore = NULL) const {
  }
  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedFutureScore) const {
  }

  void SetContainer(const DecodeStep *container) {
    m_container = container;
  }

  const DecodeGraph &GetDecodeGraph() const;

protected:
  std::vector<FactorType> m_input;
  std::vector<FactorType> m_output;
  FactorMask m_inputFactors;
  FactorMask m_outputFactors;
  const DecodeStep *m_container;
};

}

#endif
