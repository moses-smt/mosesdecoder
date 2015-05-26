/*
 * HwcmSScorer.h
 *
 *  Created on: May 5, 2015
 *      Author: mnadejde
 */

#ifndef HWCMSSCORER_H_
#define HWCMSSCORER_H_

#include <string>
#include <vector>

#include "StatisticsBasedScorer.h"
#include "moses/FF/CreateJavaVM.h"
#include "moses/Util.h"
#include <fstream>


namespace MosesTuning
{

class ScoreStats;


/**
 * HWCM scoring (Liu and Gildea 2005), but F1 instead of precision.
 * Rico's class but with no internal tree. Dependency tuples are given instead: dep_id head_id depRel -> 1 2 nsubj
 */
class HwcmSScorer: public StatisticsBasedScorer
{
public:
  explicit HwcmSScorer(const std::string& config = "");
  ~HwcmSScorer();

  std::string CallStanfordDep(std::string parsedSentence, jmethodID methodId) const;


  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles);
  virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);

  virtual std::size_t NumberOfScores() const {
    return m_order*3; //HwcmOrder = 4
  }

  virtual float calculateScore(const std::vector<int>& comps) const;

 std::vector <std::map <std::string,int> > MakeTuples(std::string ref, std::string dep, size_t order, std::vector<int> &totals);

  //which means use field 5 or 6 where we extract the tuples from
  bool useAlignment() const {
    return false;
  }

  /*
   * Scorer uses extra data instead of Alignment
   */
  virtual bool useExtraData() const {
      //cout << "Scorer::useAlignment returning false " << endl;
      return true;
    };

  std::vector<std::vector<std::string> > getExtraData() const {
  	return m_currentDepRel;
  }

private:
  mutable Moses::CreateJavaVM *javaWrapper;
  jobject m_workingStanforDepObj;
  std::ofstream inDepNbest;

  //models
  //boost::shared_ptr<lm::ngram::Model> m_WBmodel;

  // data extracted from reference files
  std::vector<std::vector<int> > m_ref_lengths;
  std::vector< std::pair<std::string, std::string> > m_ref;
  int m_currentRefId;
  std::vector <std::map<std::string,int> > m_currentRefTuples;
  bool m_includeRel;
  std::vector<int> m_totalRef;
  size_t m_order; //3 means: child + 3 ancestors
  std::vector< std::vector<std::string> > m_currentDepRel;

  // no copying allowed
  HwcmSScorer(const HwcmSScorer&);
  HwcmSScorer& operator=(const HwcmSScorer&);
};


}

#endif /* HWCMSSCORER_H_ */
