//-*- c++ -*-
#pragma once

// string distance measures
// Code by Ulrich Germann
#include<iostream>


#include <unicode/stringpiece.h>
#include <unicode/translit.h>
#include <unicode/utypes.h>
#include <unicode/unistr.h>
#include <unicode/uchar.h>
#include <unicode/utf8.h>
#include <vector>

#include "moses/TranslationModel/UG/mm/tpt_typedefs.h"


using namespace std;
//using namespace boost;
using namespace ugdiss;

namespace stringdist
{
  float
  levenshtein(UChar const* a, size_t const lenA,
	      UChar const* b, size_t const lenB);

  UErrorCode strip_accents(UnicodeString & trg);

  float
  fillAlignmentMatrix(UChar const* a, size_t const lenA,
		      UChar const* b, size_t const lenB,
		      vector<vector<float> > & M);

  class StringDiff
  {
  public:
    enum MATCHTYPE
      {
	same,        // a and b are identical
	cap,         // a and b differ only in capitalization
	flip,        // two-letter flip
	permutation, // a and b have same letters but in different order
	accent,      // a and b are the same basic letters, ignoring accents
	duplication, // a is empty
	insertion,   // a is empty
	deletion,    // b is empty
	mismatch,    // none of the above
	noinit       // not initialized
      };

    struct Segment
    {
      static char const* elabel[];
      int start_a, end_a;
      int start_b, end_b;
      MATCHTYPE match;
      float      dist;
      Segment();
      Segment(size_t const as, size_t const ae,
	      size_t const bs, size_t const be,
	      UnicodeString const& a,
	      UnicodeString const& b);
      char const* label() const;
    };
  private:
    UnicodeString a,b;
    vector<Segment> difflist;
    vector<int> diffcnt;
  public:
    UnicodeString const& set_a(string const& a);
    UnicodeString const& set_b(string const& b);
    UnicodeString const& get_a() const;
    UnicodeString const& get_b() const;
    StringDiff(string const& a, string const& b);
    StringDiff();
    size_t size();
    size_t align(bool force=false); // returns the levenshtein distance
    void showDiff(std::ostream& out);
    float levenshtein();
    Segment const& operator[](uint32_t i) const;
    void fillAlignmentMatrix(vector<vector<float> > & M) const;
    vector<int> const& getFeatures() const;
  };
}
