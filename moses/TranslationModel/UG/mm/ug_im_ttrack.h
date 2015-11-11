// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// In-memory corpus track
// (c) 2006-2012 Ulrich Germann.

#ifndef __ug_im_ttrack
#define __ug_im_ttrack

#include <string>
#include <iostream>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>

#include "tpt_typedefs.h"
#include "tpt_tokenindex.h"
#include "ug_ttrack_base.h"
#include "tpt_tokenindex.h"
#include "util/exception.hh"
#include "moses/Util.h"

// define the corpus buffer size (in sentences) and the
// for adding additional sentences:
#define IMTTRACK_INCREMENT_SIZE 100000
#define IMTSA_INCREMENT_SIZE   1000000

namespace sapt
{
  namespace bio=boost::iostreams;

  template<typename Token> class imTSA;
  template<typename Token> class imTtrack;

  template<typename TOKEN>
  typename boost::shared_ptr<imTtrack<TOKEN> >
  append(typename boost::shared_ptr<imTtrack<TOKEN> > const &  crp, 
	 std::vector<TOKEN> const & snt);

  template<typename Token>
  class imTtrack : public Ttrack<Token>
  {

  private:
    size_t numToks;
    boost::shared_ptr<typename std::vector<std::vector<Token> > > myData;  
    // pointer to corpus data
    friend class imTSA<Token>;

    friend
    typename boost::shared_ptr<imTtrack<Token> >
    append<Token>(typename boost::shared_ptr<imTtrack<Token> > const & crp, std::vector<Token> const & snt);

    void m_check_token_count(); // debugging function

  public:

    imTtrack(boost::shared_ptr<std::vector<std::vector<Token> > > const& d);
    imTtrack(std::istream& in, TokenIndex& V, std::ostream* log = NULL);
    imTtrack(size_t reserve = 0);
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
  void
  imTtrack<Token>::
  m_check_token_count()
  { // sanity check
    size_t check = 0;
    BOOST_FOREACH(std::vector<Token> const& s, *myData)
      check += s.size();
    UTIL_THROW_IF2(check != this->numToks, "[" << HERE << "]"
		   << " Wrong token count after appending sentence!"
		   << " Counted " << check << " but expected "
		   << this->numToks << " in a total of " << myData->size()
		   << " sentences.");

  }

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
    return &(*myData)[sid].back()+1;
  }

  template<typename Token>
  size_t
  imTtrack<Token>::
  size() const // return size of corpus (in number of sentences)
  {
    // we assume that myIndex has pointers to both the beginning of the
    // first sentence and the end point of the last, so there's one more
    // offset in the myIndex than there are sentences
    return myData->size();
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
  imTtrack(std::istream& in, TokenIndex& V, std::ostream* log)
    : numToks(0)
  {
    myData.reset(new std::vector<std::vector<Token> >());
    std::string line,w;
    size_t linectr=0;
    boost::unordered_map<std::string,id_type> H;
    // for (id_type i = 0; i < V.knownVocabSize(); ++i)
    // H[V[i]] = i;
    while (getline(in,line))
      {
	// cout << line << std::endl;
	myData->push_back(std::vector<Token>());
	if (log && ++linectr%1000000==0)
	  *log << linectr/1000000 << "M lines of input processed" << std::endl;
	std::istringstream buf(line);
	// cout << line << std::endl;
	while (buf>>w)
	  {
	    myData->back().push_back(Token(V[w]));
	    // cout << w << " " << myData->back().back().id() << " " 
	    // << V[w] << std::endl;
	  }
	// myData->back().resize(myData->back().size(), Token(0));
	numToks += myData->back().size();
      }
  }

  template<typename Token>
  imTtrack<Token>::
  imTtrack(size_t reserve)
    : numToks(0)
  {
    myData.reset(new std::vector<std::vector<Token> >());
    if (reserve) myData->reserve(reserve);
  }

  template<typename Token>
  imTtrack<Token>::
  imTtrack(boost::shared_ptr<std::vector<std::vector<Token> > > const& d)
    : numToks(0)
  {
    myData  = d;
    BOOST_FOREACH(std::vector<Token> const& v, *d)
      numToks += v.size();
  }

  template<typename Token>
  id_type
  imTtrack<Token>::
  findSid(Token const* t) const
  {
    id_type i;
    for (i = 0; i < myData->size(); ++i)
      {
	std::vector<Token> const& v = (*myData)[i];
	if (v.size() == 0) continue;
	if (&v.front() <= t && &v.back() >= t)
	  break;
      }
    return i;
  }

  /// add a sentence to the database
  template<typename TOKEN>
  boost::shared_ptr<imTtrack<TOKEN> >
  append(boost::shared_ptr<imTtrack<TOKEN> > const& crp, std::vector<TOKEN> const & snt)
  {
#if 1
    if (crp) crp->m_check_token_count();
#endif
    boost::shared_ptr<imTtrack<TOKEN> > ret;
    if (crp == NULL)
      {
  	ret.reset(new imTtrack<TOKEN>());
	ret->myData->reserve(IMTTRACK_INCREMENT_SIZE);
      }
    else if (crp->myData->capacity() == crp->size())
      {
  	ret.reset(new imTtrack<TOKEN>());
	ret->myData->reserve(crp->size() + IMTTRACK_INCREMENT_SIZE);
	copy(crp->myData->begin(),crp->myData->end(),ret->myData->begin());
      }
    else ret = crp;
    ret->myData->push_back(snt);
    ret->numToks += snt.size();

#if 1
    ret->m_check_token_count();
#endif
    return ret;
  }

}
#endif
