/*
 * score_reordering.cpp
 *
 *      Created by: Sara Stymne - Linköping University
 *      Machine Translation Marathon 2010, Dublin
 */

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>

#include "util/exception.hh"
#include "util/file_piece.hh"
#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"

#include "InputFileStream.h"
#include "reordering_classes.h"

using namespace std;

void split_line(const StringPiece& line, StringPiece& foreign, StringPiece& english, StringPiece& wbe, StringPiece& phrase, StringPiece& hier, float& weight);
void get_orientations(const StringPiece& pair, StringPiece& previous, StringPiece& next);

class FileFormatException : public util::Exception
{
public:
  FileFormatException() throw() {
    *this << "Invalid extract file format: ";
  }
  ~FileFormatException() throw() {}
};

int main(int argc, char* argv[])
{

  cerr << "Lexical Reordering Scorer\n"
       << "scores lexical reordering models of several types (hierarchical, phrase-based and word-based-extraction\n";

  if (argc < 3) {
    cerr << "syntax: score_reordering extractFile smoothingValue filepath (--model \"type max-orientation (specification-strings)\" )+\n";
    exit(1);
  }

  char* extractFileName = argv[1];
  double smoothingValue = atof(argv[2]);
  string filepath = argv[3];

  util::FilePiece eFile(extractFileName);

  bool smoothWithCounts = false;
  map<string,ModelScore*> modelScores;
  vector<Model*> models;
  bool hier = false;
  bool phrase = false;
  bool wbe = false;

  StringPiece e,f,w,p,h;
  StringPiece prev, next;

  int i = 4;
  while (i<argc) {
    if (strcmp(argv[i],"--SmoothWithCounts") == 0) {
      smoothWithCounts = true;
    } else if (strcmp(argv[i],"--model") == 0) {
      if (i+1 >= argc) {
        cerr << "score: syntax error, no model information provided to the option" << argv[i] << endl;
        exit(1);
      }
      istringstream is(argv[++i]);
      string m,t;
      is >> m >> t;
      modelScores[m] = ModelScore::createModelScore(t);
      if (m.compare("hier") == 0) {
        hier = true;
      } else if (m.compare("phrase") == 0) {
        phrase = true;
      }
      if (m.compare("wbe") == 0) {
        wbe = true;
      }

      if (!hier && !phrase && !wbe) {
        cerr << "WARNING: No models specified for lexical reordering. No lexical reordering table will be trained.\n";
        return 0;
      }

      string config;
      //Store all models
      while (is >> config) {
        models.push_back(Model::createModel(modelScores[m],config,filepath));
      }
    } else {
      cerr << "illegal option given to lexical reordering model score\n";
      exit(1);
    }
    i++;
  }

  ////////////////////////////////////
  //calculate smoothing
  if (smoothWithCounts) {
    util::FilePiece eFileForCounts(extractFileName);
    while (true) {
      StringPiece line;
      try {
        line = eFileForCounts.ReadLine();
      } catch (util::EndOfFileException &e) {
        break;
      }
      float weight = 1;
      split_line(line,e,f,w,p,h,weight);
      if (hier) {
        get_orientations(h, prev, next);
        modelScores["hier"]->add_example(prev,next,weight);
      }
      if (phrase) {
        get_orientations(p, prev, next);
        modelScores["phrase"]->add_example(prev,next,weight);
      }
      if (wbe) {
        get_orientations(w, prev, next);
        modelScores["wbe"]->add_example(prev,next,weight);
      }
    }

    // calculate smoothing for each model
    for (size_t i=0; i<models.size(); ++i) {
      models[i]->createSmoothing(smoothingValue);
    }

  } else {
    //constant smoothing
    for (size_t i=0; i<models.size(); ++i) {
      models[i]->createConstSmoothing(smoothingValue);
    }
  }

  ////////////////////////////////////
  //calculate scores for reordering table
  string f_current,e_current;
  bool first = true;
  while (true) {
    StringPiece line;
    try {
      line = eFile.ReadLine();
    } catch (util::EndOfFileException &e) {
      break;
    }
    float weight = 1;
    split_line(line,f,e,w,p,h,weight);

    if (first) {
      f_current = f.as_string(); //FIXME: Avoid the copy.
      e_current = e.as_string();
      first = false;
    } else if (f.compare(f_current) != 0 || e.compare(e_current) != 0) {
      //fe - score
      for (size_t i=0; i<models.size(); ++i) {
        models[i]->score_fe(f_current,e_current);
      }
      //reset
      for(map<string,ModelScore*>::const_iterator it = modelScores.begin(); it != modelScores.end(); ++it) {
        it->second->reset_fe();
      }

      if (f.compare(f_current) != 0) {
        //f - score
        for (size_t i=0; i<models.size(); ++i) {
          models[i]->score_f(f_current);
        }
        //reset
        for(map<string,ModelScore*>::const_iterator it = modelScores.begin(); it != modelScores.end(); ++it) {
          it->second->reset_f();
        }
      }
      f_current = f.as_string();
      e_current = e.as_string();
    }

    // uppdate counts
    if (hier) {
      get_orientations(h, prev, next);
      modelScores["hier"]->add_example(prev,next,weight);
    }
    if (phrase) {
      get_orientations(p, prev, next);
      modelScores["phrase"]->add_example(prev,next,weight);
    }
    if (wbe) {
      get_orientations(w, prev, next);
      modelScores["wbe"]->add_example(prev,next,weight);
    }
  }
  //Score the last phrases
  for (size_t i=0; i<models.size(); ++i) {
    models[i]->score_fe(f_current,e_current);
  }
  for (size_t i=0; i<models.size(); ++i) {
    models[i]->score_f(f_current);
  }

  //Zip all files
  for (size_t i=0; i<models.size(); ++i) {
    models[i]->zipFile();
  }

  return 0;
}

template <class It> StringPiece
GrabOrDie(It &it, const StringPiece& line)
{
  UTIL_THROW_IF(!it, FileFormatException, line.as_string());
  return *it++;
}


void split_line(
  const StringPiece& line,
  StringPiece& foreign,
  StringPiece& english,
  StringPiece& wbe,
  StringPiece& phrase,
  StringPiece& hier,
  float& weight)
{
  /*Format is source ||| target ||| orientations
    followed by one of the following 4 possibilities
      eps
       ||| weight
       | phrase | hier
       | phrase | hier ||| weight
  */

  util::TokenIter<util::MultiCharacter> pipes(line, util::MultiCharacter(" ||| "));
  foreign = GrabOrDie(pipes,line);
  english = GrabOrDie(pipes,line);
  StringPiece next = GrabOrDie(pipes,line);

  util::TokenIter<util::MultiCharacter> singlePipe(next, util::MultiCharacter(" | "));
  wbe = GrabOrDie(singlePipe,line);
  if (singlePipe) {
    phrase = GrabOrDie(singlePipe, line);
    hier = GrabOrDie(singlePipe, line);
  } else {
    phrase.clear();
    hier.clear();
  }

  if (pipes) {
    // read the weight
    char* errIndex;
    next = *pipes++;
    weight = static_cast<float>(strtod(next.data(), &errIndex));
    UTIL_THROW_IF(errIndex == next.data(), FileFormatException, line.as_string());
  }
}

void get_orientations(const StringPiece& pair, StringPiece& previous, StringPiece& next)
{
  util::TokenIter<util::SingleCharacter> tok(pair, util::SingleCharacter(' '));
  previous = GrabOrDie(tok,pair);
  next  = GrabOrDie(tok,pair);
}
