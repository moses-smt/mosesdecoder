// $Id$

#ifndef _TIMEFEATURE_H
#define _TIMEFEATURE_H

#include <iostream>
#include <fstream>
#include <assert.h>
#include <stdlib.h>
#include <string> 
#include <queue>
#include <map>
#include <cmath>

#include "ScoreFeature.h"

extern std::vector<std::string> tokenize( const char*);

namespace MosesTraining
{
    
class Time
{
public:
  void load( const std::string &fileName );
  std::string getTimeOfSentence( int sentenceId ) const;
  long getEpoch(std::string date) const;
  
  std::map<int, std::string> m_times;
  std::map<std::string, long> m_epochs;
  
  long m_oldest;
  long m_newest;
};

class TimeFeature : public ScoreFeature
{
public:

  TimeFeature(const std::string& timeFile);

  void addPropertiesToPhrasePair(ExtractionPhrasePair &phrasePair, 
                                 float count, 
                                 int sentenceId) const;

  void add(const ScoreFeatureContext& context,
           std::vector<float>& denseValues,
           std::map<std::string,float>& sparseValues) const;

protected:
    virtual void add(const std::map<std::string,float>& timeCounts, float count,
                   const MaybeLog& maybeLog,
                   std::vector<float>& denseValues,
                   std::map<std::string,float>& sparseValues) const;
  
    Time m_time;
    const std::string m_propertyKey;
    
private:
    typedef float (TimeFeature::*Mapper)(const std::string&) const;
    typedef float (TimeFeature::*Reducer)(const std::map<std::string, float>&, Mapper) const;
    
    float calculate(const std::map<std::string, float>&, Mapper, Reducer) const;
    
    /* Reducers */
    float max(const std::map<std::string, float>&, Mapper) const;
    float min(const std::map<std::string, float>&, Mapper) const;
    float avg(const std::map<std::string, float>&, Mapper) const;
    
    /* Mappers */
    float hyperbola(const std::string& date) const;
    float linear(const std::string& date) const;

};

}

#endif
