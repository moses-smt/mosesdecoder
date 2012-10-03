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

#include "FactorTypeSet.h"
#include "FeatureFunction.h"
#include "TypeDef.h"

namespace Moses
{

/**
  * A feature on the decoding path (Generation or Translation)
  * @todo don't quite understand what this is
 **/
class DecodeFeature : public StatelessFeatureFunction {

  public:
    DecodeFeature(  const std::string& description,
                    size_t numScoreComponents,
                    const std::vector<FactorType> &input,
                    const std::vector<FactorType> &output);
    
    //! returns output factor types as specified by the ini file
    const FactorMask& GetOutputFactorMask() const;
    
    //! returns input factor types as specified by the ini file
    const FactorMask& GetInputFactorMask() const;
    
    const std::vector<FactorType>& GetInput() const;
    const std::vector<FactorType>& GetOutput() const;
    
  private:
    std::vector<FactorType> m_input;
    std::vector<FactorType> m_output;
    FactorMask m_inputFactors;
    FactorMask m_outputFactors;
};

}

#endif
