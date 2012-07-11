#ifndef __SOURCE_CONTEXT_HPP_
#define __SOURCE_CONTEXT_HPP_

#include <set>
#include <vector>
#include <string>
#include <map>
#include <string>
#include "Lexicon.h"
#include "TaggedCorpus.h"
#include "FileFormats.h"

using namespace std;

/**
 * \class Set of Context Features
 *
 */
class ContextFeatureSet{

public:

  /**
   * Default constructor 
   */
  ContextFeatureSet();
  inline ~ContextFeatureSet(){posContext.clear(); bowContext.clear(); authorizedValues.clear(); forbiddenValues.clear();};

 /**
   * construct a feature set from config file
   */
  ContextFeatureSet(const char *config);
  vector<string> extract(int startPos, int endPos, string context, string fdelim);

  void printConfig();

private:
  /**
   * position sensitive features
   */
  map<int, vector<int> > posContext;

  vector<string> extractPosContext(int startPos, int endPos, vector< vector<string> > context);

  /**
   * bag of word features
   */
  set<int> bowContext;
  vector<string> extractBowContext(int startPos, int endPos, vector< vector<string> > context);

  /**
   * Authorized feature value lexicon
   * Only store features that take those predefined values (e.g., test set vocabulary)
   */
  vector<Lexicon> authorizedValues;

  /**
   * Forbidden feature value lexicon
   * Ignore features that match those predefined values (e.g., stopword list)
   */
  vector<Lexicon> forbiddenValues;
};
#endif
