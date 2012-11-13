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
#include "StaticData.h"

using namespace std;

namespace Moses
{

DecodeFeature::DecodeFeature(const std::string& description, size_t numScoreComponents, const std::vector<FactorType> &input, const std::vector<FactorType> &output) :
  StatelessFeatureFunction(description,numScoreComponents),
  m_input(input), m_output(output)
{
  m_inputFactors = FactorMask(input);
  m_outputFactors = FactorMask(output);
  VERBOSE(2,"DecodeFeature: input=" << m_inputFactors << "  output=" << m_outputFactors << std::endl);
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

}

