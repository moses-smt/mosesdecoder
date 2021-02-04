/*
 * SentenceWithCandidates.cpp
 *
 *  Created on: 14 Dec 2015
 *      Author: hieu
 */
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string.hpp> 

#include "SentenceWithCandidates.h"
#include "../System.h"
#include "../parameters/AllOptions.h"
#include "../legacy/Util2.h"
#include <unordered_map>

using namespace std;
using namespace boost;

namespace Moses2
{

const string SentenceWithCandidates::INPUT_PART_DELIM = "@@@";
const string SentenceWithCandidates::PT_LINE_DELIM = "$$$";

SentenceWithCandidates *SentenceWithCandidates::CreateFromString(MemPool &pool, FactorCollection &vocab,
                                     const System &system, const std::string &str)
{
  SentenceWithCandidates *ret;
  
  // Break input into two parts: the parts are delimited by 
  typedef split_iterator<string::const_iterator> string_split_iterator;
  vector<string> input_parts;
  for(string_split_iterator It= make_split_iterator(str, first_finder(SentenceWithCandidates::INPUT_PART_DELIM, is_iequal()));    
                It!=string_split_iterator();    
                ++It)
  {
      input_parts.push_back(copy_range<std::string>(*It));
  }

  //cerr << "Number of subparts: " << input_parts.size() << endl;

  if (input_parts.size() ==2 ) {
      //cerr << "correct number of parts" << endl ;
  } else {
      // TODO: how to handle wrong input format 
      cerr << "INCORRECT number of parts" << endl ;
      exit(1);
  }

  trim(input_parts[0]);
  trim(input_parts[1]);
  //cerr << "Input String: " << input_parts[0] << endl ;
  //cerr << "Phrase Table: " << input_parts[1] << endl ;

  ///// Process the text part of the input 
  const string partstr = input_parts[0];
 
  // no xml
  //cerr << "PB SentenceWithCandidates" << endl;
  std::vector<std::string> toks = Tokenize(partstr);

  size_t size = toks.size();
  ret = new (pool.Allocate<SentenceWithCandidates>()) SentenceWithCandidates(pool, size);
  ret->PhraseImplTemplate<Word>::CreateFromString(vocab, system, toks, false);

  //cerr << "REORDERING CONSTRAINTS:" << ret->GetReorderingConstraint() << endl;
  //cerr << "ret=" << ret->Debug(system) << endl;


  //// Parse the phrase table of the input 
  ret->m_phraseTableString = replace_all_copy(input_parts[1],PT_LINE_DELIM,"\n");
    // ret->m_phraseTableString="constant phrase table";
//   cerr << "Extracted Phrase Table String: " << ret->m_phraseTableString << endl; 
   //cerr << "Extracted Phrase Table String: " << ret->getPhraseTableString() << endl;

  return ret;
}

SentenceWithCandidates::SentenceWithCandidates(MemPool &pool, size_t size)
:Sentence(pool, size)
{
    //cerr << "SentenceWithCandidates::SentenceWithCandidates" << endl;
}

SentenceWithCandidates::~SentenceWithCandidates()
{
    //cerr << "SentenceWithCandidates::~SentenceWithCandidates" << endl;
}

std::string SentenceWithCandidates::Debug(const System &system) const
{
  return "SentenceWithCandidates::Debug";
}

} /* namespace Moses2 */

