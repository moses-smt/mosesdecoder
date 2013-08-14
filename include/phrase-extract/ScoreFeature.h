/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2012- University of Edinburgh

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

/**
 * This contains extra features that can be added to the scorer. To add a new feature:
 * 1. Implement a subclass of ScoreFeature
 * 2. Updated ScoreFeatureManager.configure() to configure your feature, and usage() to
 *    display usage info.
 * 3. Write unit tests (see ScoreFeatureTest.cpp) and regression tests
**/

#pragma once

#include <string>
#include <map>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "util/exception.hh"

#include "PhraseAlignment.h"

namespace MosesTraining
{

struct MaybeLog {
  MaybeLog(bool useLog, float negativeLog):
    m_useLog(useLog), m_negativeLog(negativeLog) {}

  inline float operator() (float a) const {
    return m_useLog ? m_negativeLog*log(a) : a;
  }

  float m_useLog;
  float m_negativeLog;
};

class ScoreFeatureArgumentException : public util::Exception
{
public:
  ScoreFeatureArgumentException() throw() {
    *this << "Unable to configure features: ";
  }
  ~ScoreFeatureArgumentException() throw() {}
};

/** Passed to each feature to be used to calculate its values */
struct ScoreFeatureContext {
  ScoreFeatureContext(
    const PhraseAlignmentCollection &thePhrasePair,
    float theCount, /* Total counts of all phrase pairs*/
    const MaybeLog& theMaybeLog
  ) :
    phrasePair(thePhrasePair),
    count(theCount),
    maybeLog(theMaybeLog) {
  }

  const PhraseAlignmentCollection& phrasePair;
  float count;
  MaybeLog maybeLog;
};

/**
  * Abstract base class for extra features that can be added to the phrase table
  * during scoring.
  **/
class ScoreFeature
{
public:
  /** Add the values for this feature function. */
  virtual void add(const ScoreFeatureContext& context,
                   std::vector<float>& denseValues,
                   std::map<std::string,float>& sparseValues) const = 0;

  /** Return true if the two phrase pairs are equal from the point of this feature. Assume
      that they already compare true according to PhraseAlignment.equals()
   **/
  virtual bool equals(const PhraseAlignment& lhs, const PhraseAlignment& rhs) const = 0;

  virtual ~ScoreFeature() {}

};

typedef boost::shared_ptr<ScoreFeature> ScoreFeaturePtr;
class ScoreFeatureManager
{
public:
  ScoreFeatureManager():
    m_includeSentenceId(false) {}

  /** To be appended to the score usage message */
  const std::string& usage() const;

  /** Pass the unused command-line arguments to configure the extra features */
  void configure(const std::vector<std::string> args);

  /** Add all the features */
  void addFeatures(const ScoreFeatureContext& context,
                   std::vector<float>& denseValues,
                   std::map<std::string,float>& sparseValues) const;

  /**
    * Used to tell if the PhraseAlignment should be considered the same by all
    * extended features.
   **/
  bool equals(const PhraseAlignment& lhs, const PhraseAlignment& rhs) const;

  const std::vector<ScoreFeaturePtr>& getFeatures() const {
    return m_features;
  }

  /** Do we need to include sentence ids in phrase pairs? */
  bool includeSentenceId() const {
    return m_includeSentenceId;
  }

private:
  std::vector<ScoreFeaturePtr> m_features;
  bool m_includeSentenceId;
};

}


