// -*- c++-mode -*-
// In-memory corpus track
// (c) 2006-2012 Ulrich Germann. 

#ifndef __ug_im_ttrack
#define __ug_im_ttrack

#include <string>
#include <iostream>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "tpt_typedefs.h"
#include "tpt_tokenindex.h"
#include "ug_ttrack_base.h"
#include "tpt_tokenindex.h"
// #include "ug_vocab.h"

namespace ugdiss
{
  using namespace std;
  namespace bio=boost::iostreams;

  template<typename Token=id_type>
  class imTtrack : public Ttrack<Token>
  {
  private:
    size_t numToks;
    boost::shared_ptr<vector<vector<Token> > > myData;  // pointer to corpus data
  public:

    imTtrack(boost::shared_ptr<vector<vector<Token> > > const& d);
    imTtrack(istream& in, TokenIndex const& V, ostream* log);
    imTtrack();
    // imTtrack(istream& in, Vocab& V);

    /** return pointer to beginning of sentence */
    Token const* sntStart(size_t sid) const; 

    /** return pointer to beginning of sentence */
    Token const* sntEnd(size_t sid) const;   

    size_t size() const; 
    size_t numTokens() const;

    id_type findSid(Token const* t) const;

  };

  template<typename Token>
  Token const* 
  imTtrack<Token>::
  sntStart(size_t sid) const // return pointer to beginning of sentence
  {
    assert(sid < size());
    if ((*myData)[sid].size() == 0) return NULL;
    return &((*myData)[sid].front());
  }
  
  template<typename Token>
  Token const* 
  imTtrack<Token>::
  sntEnd(size_t sid) const // return pointer to end of sentence
  {
    assert(sid < size());
    if ((*myData)[sid].size() == 0) return NULL;
    return &(*myData)[sid].back();
  }
  
  template<typename Token>
  size_t 
  imTtrack<Token>::
  size() const // return size of corpus (in number of sentences)
  {
    // we assume that myIndex has pointers to both the beginning of the
    // first sentence and the end point of the last, so there's one more
    // offset in the myIndex than there are sentences
    return myData.size();
  }
  
  template<typename Token>
  size_t 
  imTtrack<Token>::
  numTokens() const // return size of corpus (in number of words)
  {
    return numToks;
  }
  
  template<typename Token>
  imTtrack<Token>::
  imTtrack(istream& in, TokenIndex const& V, ostream* log = NULL)
  {
    myData.reset(new vector<vector<Token> >());
    numToks = 0;
    string line,w;
    size_t linectr=0;
    boost::unordered_map<string,id_type> H;
    for (id_type i = 0; i < V.knownVocabSize(); ++i)
      H[V[i]] = i;
    while (getline(in,line)) 
      {
	myData->push_back(vector<Token>());
	if (log && ++linectr%1000000==0) 
	  *log << linectr/1000000 << "M lines of input processed" << endl;
	istringstream buf(line);
	while (buf>>w) 
	  myData->back().push_back(Token(H[w]));
	myData->back().resize(myData.back().size());
	numToks += myData->back().size();
      }
  }
  
  template<typename Token>
  imTtrack<Token>::
  imTtrack()
  {
    myData.reset(new vector<vector<Token> >());
  }

  template<typename Token>
  imTtrack<Token>::
  imTtrack(boost::shared_ptr<vector<vector<Token> > > const& d)
  {
    myData  = d;
  }

  template<typename Token>
  id_type
  imTtrack<Token>::
  findSid(Token const* t) const
  {
    id_type i;
    for (i = 0; i < myData->size(); ++i)
      {
	vector<Token> const& v = (*myData)[i];
	if (v.size() == 0) continue;
	if (&v.front() <= t && &v.back() >= t) 
	  break;
      }
    return i;
  }

}
#endif
