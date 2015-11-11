// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2014- University of Edinburgh

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
  * Compute paraphrases from the phrase table
**/
#include <cmath>
#include <iostream>
#include <map>

#include <boost/program_options.hpp>

#include "util/double-conversion/double-conversion.h"
#include "util/exception.hh"
#include "util/file_piece.hh"
#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"

//using namespace Moses;
using namespace std;

namespace po = boost::program_options;

typedef multimap<float,string> Probs;

static float threshold = 1e-04;
static size_t maxE = 10000; //histogram pruning

static void add(const string& e, const vector<float> scores,
  Probs& p_e_given_f, Probs& p_f_given_e) {
  if (scores[0] > threshold) {
    p_f_given_e.insert(pair<float,string>(scores[0],e));
  }
  while(p_f_given_e.size() > maxE) p_f_given_e.erase(p_f_given_e.begin());
  if (scores[2] > threshold) {
    p_e_given_f.insert(pair<float,string>(scores[2],e));
  }
  while(p_e_given_f.size() > maxE) p_e_given_f.erase(p_e_given_f.begin());
}

static void finalise(Probs& p_e_given_f, Probs& p_f_given_e) {
  //cerr << "Sizes: p(e|f): " << p_e_given_f.size() << " p(f|e): " << p_f_given_e.size() << endl;
  for (Probs::const_iterator e1_iter = p_f_given_e.begin() ;
          e1_iter !=  p_f_given_e.end(); ++e1_iter) {
    for (Probs::const_iterator e2_iter = p_e_given_f.begin() ;
          e2_iter != p_e_given_f.end(); ++e2_iter) {

      if (e1_iter->second == e2_iter->second) continue;
      cout << e1_iter->second << " ||| " << e2_iter->second << " ||| " <<
        e1_iter->first * e2_iter->first << " |||  " << endl;
    }
  }
  p_e_given_f.clear();
  p_f_given_e.clear();
}

int main(int argc, char** argv) {

  string input_file;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "Print help message and exit")
    ("threshold,t", po::value<float>(&threshold), "Threshold for p(e|f) and p(f|e)")
    ("max-target,m", po::value<size_t>(&maxE), "Maximum number of target phrases")
    ("input-file", po::value<string>(&input_file)->required(), "Input phrase table")
    ;

  po::positional_options_description pos;
  pos.add("input-file",1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc,argv).options(desc).positional(pos).run(), vm);


  if (vm.count("help")) {
    cerr << "Usage: " << string(argv[0]) + " [options] input-file" << endl;
    cerr << desc << endl;
    return 0;
  }

  po::notify(vm);


  cerr << "Reading from " << input_file << endl;
  util::FilePiece in(input_file.c_str(), &std::cerr);
  vector<float> scoreVector;
  StringPiece line;
  double_conversion::StringToDoubleConverter converter(double_conversion::StringToDoubleConverter::NO_FLAGS, NAN, NAN, "inf", "nan");


  string previousSourcePhrase;
  Probs p_f_given_e_table;
  Probs p_e_given_f_table;

  size_t count = 0;
  while(true) {
    try {
      line = in.ReadLine();
    } catch (const util::EndOfFileException &e) {
      break;
    }
    ++count;

    util::TokenIter<util::MultiCharacter> pipes(line, " ||| ");
    StringPiece sourcePhrase(*pipes);
    StringPiece targetPhrase(*++pipes);
    StringPiece scoreString(*++pipes);
    scoreVector.clear();
    for (util::TokenIter<util::AnyCharacter, true> s(scoreString, " \t"); s; ++s) {
      int processed;
      float score = converter.StringToFloat(s->data(), s->length(), &processed);
      UTIL_THROW_IF2(isnan(score), "Bad score " << *s << " on line " << count);
      scoreVector.push_back(score);
    }

    if (sourcePhrase.size() && sourcePhrase != previousSourcePhrase) {
      finalise(p_e_given_f_table, p_f_given_e_table);
    }
    add(targetPhrase.as_string(),scoreVector, p_e_given_f_table, p_f_given_e_table);
    previousSourcePhrase = sourcePhrase.as_string();
  }
  finalise(p_e_given_f_table, p_f_given_e_table);



  return 0;
}
