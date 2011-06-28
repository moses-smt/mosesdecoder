/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2011 University of Edinburgh

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

#include <string>
#include <boost/shared_ptr.hpp>
#include "../moses/src/FeatureFunction.h"
#include "FeatureFunction.h"

namespace Josiah {

typedef boost::shared_ptr<Moses::StatelessFeatureFunction> MosesFeatureHandle;

/** 
 * Stateless Gibbler feature
 **/
class StatelessFeature : public Feature {
  public:
    virtual FeatureFunctionHandle getFunction(const Sample& sample) const;
    /** Scores due to this translation option */
    virtual void assign
      (const Moses::TranslationOption* option, FVector& scores) const = 0;

};

/** 
* Wraps a Moses stateless feature to give a gibbler feature.
**/
class StatelessFeatureAdaptor : public StatelessFeature {
  public:
    StatelessFeatureAdaptor(
      const MosesFeatureHandle& mosesFeature);
    virtual void assign(const Moses::TranslationOption* toption, FVector& scores) const;
  
  private:
    MosesFeatureHandle m_mosesFeature;
    std::vector<FName> m_featureNames;
    
};


class StatelessFeatureFunction : public FeatureFunction {
  public:

  StatelessFeatureFunction
    (const Sample& sample, const StatelessFeature* parent);

  /** Assign the total score of this feature on the current hypo */
  virtual void assignScore(FVector& scores);
  
  /** Score due to one segment */
  virtual void doSingleUpdate
    (const TranslationOption* option, const TargetGap& gap, FVector& scores);

  /** Score due to two segments. The left and right refer to the target positions.**/
  virtual void doContiguousPairedUpdate
    (const TranslationOption* leftOption,const TranslationOption* rightOption, 
      const TargetGap& gap, FVector& scores);

  virtual void doDiscontiguousPairedUpdate
    (const TranslationOption* leftOption,const TranslationOption* rightOption, 
      const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores);
  
  /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
  virtual void doFlipUpdate
    (const TranslationOption* leftOption,const TranslationOption* rightOption, 
       const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores);

public:
  const StatelessFeature* m_parent;
};

}


