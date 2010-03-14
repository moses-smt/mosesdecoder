/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

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
#pragma once
#include <utility>
#include <vector>
#include <string>
#include "Derivation.h"
#include "Decoder.h"

namespace Josiah {

class KLDivergenceCalculator {
  public:
    KLDivergenceCalculator(const std::string& line);
    float calcDDDivergence(const std::vector<std::pair<const Derivation*, float> >& nbest);
    float calcTDDivergence(const std::vector<std::pair<const Translation*, float> >& nbest);
  private:
    auto_ptr<KLDecoder> m_klDecoder;   
    const std::string& m_input;
    double m_Z;
};
}