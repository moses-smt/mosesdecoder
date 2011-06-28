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

#pragma once

#include <boost/shared_ptr.hpp>

#include "Decoder.h"

namespace Josiah {

class GainFunction;
typedef boost::shared_ptr<GainFunction> GainFunctionHandle;

void TextToTranslation(const std::string& text, Translation& words);

/**
 * Factory for gain functions.
 **/
class Gain {
  public:
    /** Load the reference files */
    void LoadReferences(const std::vector<std::string>& refFilenames,
           const std::string& sourceFile);
    /** Get the function to calculate the gain on these sentences */
    virtual GainFunctionHandle GetGainFunction(const std::vector<size_t>& sentenceIds) = 0;
    /** Add the set of references for a specific sentence */
    virtual void AddReferences(const std::vector<Translation>& refs, const Translation& source) = 0;
    /** Convenience method fof single sentence */
    GainFunctionHandle GetGainFunction(size_t sentenceId);
    virtual float GetAverageReferenceLength(size_t sentenceId) const = 0;
    virtual ~Gain() {}
};

class GainFunction {
  public:
    /** Calculate Gain for set of hypotheses */
    virtual float Evaluate(const std::vector<Translation>& hypotheses) const = 0;
    /** Add the stats for this hypothesis to the smoothing stats being collected */
    virtual void AddSmoothingStats(size_t sentenceId, const Translation& hypothesis) {}
    /** Inform the GainFunction that we've finished with this sentence, and it can now
     update the parent's stats */
    virtual void UpdateSmoothingStats() {}
    /** Shortcut for evaluating just one sentence */
    float Evaluate(const Translation& hypothesis) const;
    virtual ~GainFunction() {}

};

}
