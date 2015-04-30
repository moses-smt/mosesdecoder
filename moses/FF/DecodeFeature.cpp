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

#include <iostream>

#include "DecodeFeature.h"
#include "moses/DecodeStep.h"
#include "moses/StaticData.h"

using namespace std;

namespace Moses
{

DecodeFeature::DecodeFeature(const std::string &line, bool registerNow)
  : StatelessFeatureFunction(line, registerNow)
  , m_container(NULL)
{
  VERBOSE(2,"DecodeFeature:" << std::endl);
}

DecodeFeature::DecodeFeature(size_t numScoreComponents
                             , const std::string &line)
  : StatelessFeatureFunction(numScoreComponents, line)
  , m_container(NULL)
{
  VERBOSE(2,"DecodeFeature: no factors yet" << std::endl);
}

DecodeFeature::DecodeFeature(size_t numScoreComponents
                             , const std::vector<FactorType> &input
                             , const std::vector<FactorType> &output
                             , const std::string &line)
  : StatelessFeatureFunction(numScoreComponents, line)
  , m_input(input), m_output(output)
  , m_container(NULL)
{
  m_inputFactors = FactorMask(input);
  m_outputFactors = FactorMask(output);
  VERBOSE(2,"DecodeFeature: input=" << m_inputFactors << "  output=" << m_outputFactors << std::endl);
}

void DecodeFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "input-factor") {
    m_input =Tokenize<FactorType>(value, ",");
    m_inputFactors = FactorMask(m_input);
  } else if (key == "output-factor") {
    m_output =Tokenize<FactorType>(value, ",");
    m_outputFactors = FactorMask(m_output);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}


const FactorMask& DecodeFeature::GetOutputFactorMask() const
{
  return m_outputFactors;
}


const FactorMask& DecodeFeature::GetInputFactorMask() const
{
  return m_inputFactors;
}

const std::vector<FactorType>& DecodeFeature::GetInput() const
{
  return m_input;
}

const std::vector<FactorType>& DecodeFeature::GetOutput() const
{
  return m_output;
}

bool DecodeFeature::IsUseable(const FactorMask &mask) const
{
  for (size_t i = 0; i < m_output.size(); ++i) {
    const FactorType &factor = m_output[i];
    if (!mask[factor]) {
      return false;
    }
  }
  return true;
}

const DecodeGraph &DecodeFeature::GetDecodeGraph() const
{
  assert(m_container);
  const DecodeGraph *graph = m_container->GetContainer();
  assert(graph);
  return *graph;
}

}

