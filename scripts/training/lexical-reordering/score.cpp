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
#include "InputFileStream.h"

#include "reordering_classes.h"

using namespace std;

void split_line(const string& line, string& foreign, string& english, string& wbe, string& phrase, string& hier);
void get_orientations(const string& pair, string& previous, string& next);


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

  Moses::InputFileStream eFile(extractFileName);
  if (!eFile) {
    cerr << "Could not open the extract file " << extractFileName <<"for scoring of lexical reordering models\n";
    exit(1);
  }

  bool smoothWithCounts = false;
  map<string,ModelScore*> modelScores;
  vector<Model*> models;
  bool hier = false;
  bool phrase = false;
  bool wbe = false;

  string e,f,w,p,h;
  string prev, next;

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
    string line;
    while (getline(eFile,line)) {
      split_line(line,e,f,w,p,h);
      if (hier) {
        get_orientations(h, prev, next);
        modelScores["hier"]->add_example(prev,next);
      }
      if (phrase) {
        get_orientations(p, prev, next);
        modelScores["phrase"]->add_example(prev,next);
      }
      if (wbe) {
        get_orientations(w, prev, next);
        modelScores["wbe"]->add_example(prev,next);
      }
    }

    // calculate smoothing for each model
    for (size_t i=0; i<models.size(); ++i) {
      models[i]->createSmoothing(smoothingValue);
    }

    //reopen eFile
    eFile.Close();
    eFile.Open(extractFileName);
  } else {
    //constant smoothing
    for (size_t i=0; i<models.size(); ++i) {
      models[i]->createConstSmoothing(smoothingValue);
    }
  }

  ////////////////////////////////////
  //calculate scores for reordering table
  string line,f_current,e_current;
  bool first = true;
  while (getline(eFile, line)) {
    split_line(line,f,e,w,p,h);

    if (first) {
      f_current = f;
      e_current = e;
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
      f_current = f;
      e_current = e;
    }

    // uppdate counts
    if (hier) {
      get_orientations(h, prev, next);
      modelScores["hier"]->add_example(prev,next);
    }
    if (phrase) {
      get_orientations(p, prev, next);
      modelScores["phrase"]->add_example(prev,next);
    }
    if (wbe) {
      get_orientations(w, prev, next);
      modelScores["wbe"]->add_example(prev,next);
    }
  }
  //Score the last phrases
  for (size_t i=0; i<models.size(); ++i) {
    models[i]->score_fe(f,e);
  }
  for (size_t i=0; i<models.size(); ++i) {
    models[i]->score_f(f);
  }

  //Zip all files
  for (size_t i=0; i<models.size(); ++i) {
    models[i]->zipFile();
  }

  return 0;
}



void split_line(const string& line, string& foreign, string& english, string& wbe, string& phrase, string& hier)
{

  int begin = 0;
  int end = line.find(" ||| ");
  foreign = line.substr(begin, end - begin);

  begin = end+5;
  end = line.find(" ||| ", begin);
  english = line.substr(begin, end - begin);

  begin = end+5;
  end = line.find(" | ", begin);
  wbe = line.substr(begin, end - begin);

  begin = end+3;
  end = line.find(" | ", begin);
  phrase = line.substr(begin, end - begin);

  begin = end+3;
  hier = line.substr(begin, line.size() - begin);
}

void get_orientations(const string& pair, string& previous, string& next)
{
  istringstream is(pair);
  is >> previous >> next;
}
