// vim:tabstop=2
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

#include <algorithm>
#include <cassert>
#include <cstdlib>

#include "DummyScoreProducers.h"
#include "Gibbler.h"
#include "Hypothesis.h"
#include "TranslationOptionCollection.h"

namespace Moses {

  class Sample;
  class SampleCollector;

  /** Abstract base class for gibbs operators **/
  class GibbsOperator {
    public:
        /**
          * Run an iteration of the Gibbs sampler, updating the hypothesis.
          * TODO: Probably need some kind of collector object
          **/
        virtual void doIteration(Sample& sample, const TranslationOptionCollection& toc) = 0;
        virtual const string& name() const = 0;
        
     protected:
        /**
          * Pick random sample from given (un-normalised) log probabilities.
          **/
        size_t getSample(const std::vector<double>& scores);
  };
  
  
  class MergeSplitOperator : public virtual GibbsOperator {
    public:
        MergeSplitOperator() : m_name("merge-split") {}
        virtual void doIteration(Sample& sample, const TranslationOptionCollection& toc);
        virtual const string& name() const {return m_name;}
    
    private:
        string m_name;
  
  };
  
 
}

