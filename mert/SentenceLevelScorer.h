//
//  SentenceLevelScorer.h
//  mert_lib
//
//  Created by Hieu Hoang on 22/06/2012.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#ifndef mert_lib_SentenceLevelScorer_h
#define mert_lib_SentenceLevelScorer_h

#include "Scorer.h"
#include <string>
#include <vector>
#include <vector>
#include <boost/spirit/home/support/detail/lexer/runtime_error.hpp>

/**
 * Abstract base class for scorers that work by using sentence level
 * statistics eg. permutation distance metrics **/
class SentenceLevelScorer : public Scorer
{
  
public:
  SentenceLevelScorer(const std::string& name, const std::string& config): Scorer(name,config) {
    //configure regularisation
    static std::string KEY_TYPE = "regtype";
    static std::string KEY_WINDOW = "regwin";
    static std::string KEY_CASE = "case";
    static std::string TYPE_NONE = "none";
    static std::string TYPE_AVERAGE = "average";
    static std::string TYPE_MINIMUM = "min";
    static std::string TRUE = "true";
    static std::string FALSE = "false";
    
    std::string type = getConfig(KEY_TYPE,TYPE_NONE);
    if (type == TYPE_NONE) {
      _regularisationStrategy = REG_NONE;
    } else if (type == TYPE_AVERAGE) {
      _regularisationStrategy = REG_AVERAGE;
    } else if (type == TYPE_MINIMUM) {
      _regularisationStrategy = REG_MINIMUM;
    } else {
      throw boost::lexer::runtime_error("Unknown scorer regularisation strategy: " + type);
    }
    std::cerr << "Using scorer regularisation strategy: " << type << std::endl;
    
    std::string window = getConfig(KEY_WINDOW,"0");
    _regularisationWindow = atoi(window.c_str());
    std::cerr << "Using scorer regularisation window: " << _regularisationWindow << std::endl;
    
    std::string preservecase = getConfig(KEY_CASE,TRUE);
    if (preservecase == TRUE) {
      m_enable_preserve_case = true;
    } else if (preservecase == FALSE) {
      m_enable_preserve_case = false;
    }
    std::cerr << "Using case preservation: " << m_enable_preserve_case << std::endl;
    
    
  }
  ~SentenceLevelScorer() {};
  virtual void score(const candidates_t& candidates, const diffs_t& diffs,
                     statscores_t& scores);
  
  //calculate the actual score
  virtual statscore_t calculateScore(const std::vector<statscore_t>& totals) {
    return 0;
  };
  
  
  
protected:
  
  //regularisation
  ScorerRegularisationStrategy _regularisationStrategy;
  size_t  _regularisationWindow;
  
};



#endif
