#ifndef __LEXICON_HPP_
#define __LEXICON_HPP_

#include <vector>
#include <string>
#include <fstream>

#include "StringUtils.h"

using namespace std;

/**
 * \class store a word or phrase vocabulary in sorted order.
 *
 * The word indices MUST start at 0.
 */
class Lexicon{
public:
  Lexicon(){lexicon.clear();};
  ~Lexicon(){lexicon.clear();};
  
  void insert_in_order(string word);
  bool exists(string word) const;
  int get_id(string word) const;
  string get_word(int word) const;
  
  void readin(istream &in);
  void printout(ostream &out) const;

  int binary_search(string word,int begin, int end) const;
  
private:
  vector<string> lexicon;
};

#endif // __LEXICON_HPP_
