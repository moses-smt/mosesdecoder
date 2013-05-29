
#include <vector>
#include <iostream>
#include <cstdlib>
#include <numeric>
#include <cstdio>
#include <sstream>
#include <string>
#include "zlib.h"

#include "reordering_classes.h"

using namespace std;

ModelScore::ModelScore()
{
  for(int i=MONO; i<=NOMONO; ++i) {
    count_fe_prev.push_back(0);
    count_fe_next.push_back(0);
    count_f_prev.push_back(0);
    count_f_next.push_back(0);
  }
}

ModelScore::~ModelScore() {}

ModelScore* ModelScore::createModelScore(const string& modeltype)
{
  if (modeltype.compare("mslr") == 0) {
    return new ModelScoreMSLR();
  } else if (modeltype.compare("msd") == 0) {
    return new ModelScoreMSD();
  } else if (modeltype.compare("monotonicity") == 0 ) {
    return new ModelScoreMonotonicity();
  } else if (modeltype.compare("leftright") == 0) {
    return new ModelScoreLR();
  } else {
    cerr << "Illegal model type given for lexical reordering model scoring: " << modeltype << ". The allowed types are: mslr, msd, monotonicity, leftright" << endl;
    exit(1);
  }
}

void ModelScore::reset_fe()
{
  for(int i=MONO; i<=NOMONO; ++i) {
    count_fe_prev[i] = 0;
    count_fe_next[i] = 0;
  }
}

void ModelScore::reset_f()
{
  for(int i=MONO; i<=NOMONO; ++i) {
    count_f_prev[i] = 0;
    count_f_next[i] = 0;
  }
}

void ModelScore::add_example
(const StringPiece& previous, const StringPiece& next, float weight)
{
  count_fe_prev[getType(previous)]+=weight;
  count_f_prev[getType(previous)]+=weight;
  count_fe_next[getType(next)]+=weight;
  count_f_next[getType(next)]+=weight;
}

const vector<double>& ModelScore::get_scores_fe_prev() const
{
  return count_fe_prev;
}

const vector<double>& ModelScore::get_scores_fe_next() const
{
  return count_fe_next;
}

const vector<double>& ModelScore::get_scores_f_prev() const
{
  return count_f_prev;
}

const vector<double>& ModelScore::get_scores_f_next() const
{
  return count_f_next;
}


ORIENTATION ModelScore::getType(const StringPiece& s)
{
  if (s.compare("mono") == 0) {
    return MONO;
  } else if (s.compare("swap") == 0) {
    return SWAP;
  } else if (s.compare("dright") == 0) {
    return DRIGHT;
  } else if (s.compare("dleft") == 0) {
    return DLEFT;
  } else if (s.compare("other") == 0) {
    return OTHER;
  } else if (s.compare("nomono") == 0) {
    return NOMONO;
  } else {
    cerr << "Illegal reordering type used: " << s << endl;
    exit(1);
  }
}


ORIENTATION ModelScoreMSLR::getType(const StringPiece& s)
{
  if (s.compare("mono") == 0) {
    return MONO;
  } else if (s.compare("swap") == 0) {
    return SWAP;
  } else if (s.compare("dright") == 0) {
    return DRIGHT;
  } else if (s.compare("dleft") == 0) {
    return DLEFT;
  } else if (s.compare("other") == 0 || s.compare("nomono") == 0) {
    cerr << "Illegal reordering type used: " << s << " for model type mslr. You have to re-run step 5 in order to train such a model." <<  endl;
    exit(1);
  } else {
    cerr << "Illegal reordering type used: " << s << endl;
    exit(1);
  }
}


ORIENTATION ModelScoreLR::getType(const StringPiece& s)
{
  if (s.compare("mono") == 0 || s.compare("dright") == 0) {
    return DRIGHT;
  } else if (s.compare("swap") == 0 || s.compare("dleft") == 0) {
    return DLEFT;
  } else if (s.compare("other") == 0 || s.compare("nomono") == 0) {
    cerr << "Illegal reordering type used: " << s << " for model type LeftRight. You have to re-run step 5 in order to train such a model." <<  endl;
    exit(1);
  } else {
    cerr << "Illegal reordering type used: " << s << endl;
    exit(1);
  }
}


ORIENTATION ModelScoreMSD::getType(const StringPiece& s)
{
  if (s.compare("mono") == 0) {
    return MONO;
  } else if (s.compare("swap") == 0) {
    return SWAP;
  } else if (s.compare("dleft") == 0 ||
             s.compare("dright") == 0 ||
             s.compare("other") == 0) {
    return OTHER;
  } else if (s.compare("nomono") == 0) {
    cerr << "Illegal reordering type used: " << s << " for model type msd. You have to re-run step 5 in order to train such a model." <<  endl;
    exit(1);
  } else {
    cerr << "Illegal reordering type used: " << s << endl;
    exit(1);
  }
}

ORIENTATION ModelScoreMonotonicity::getType(const StringPiece& s)
{
  if (s.compare("mono") == 0) {
    return MONO;
  } else if (s.compare("swap") == 0 ||
             s.compare("dleft") == 0 ||
             s.compare("dright") == 0 ||
             s.compare("other") == 0 ||
             s.compare("nomono") == 0 ) {
    return NOMONO;
  } else {
    cerr << "Illegal reordering type used: " << s << endl;
    exit(1);
  }
}



void ScorerMSLR::score(const vector<double>&  all_scores, vector<double>&  scores) const
{
  scores.push_back(all_scores[MONO]);
  scores.push_back(all_scores[SWAP]);
  scores.push_back(all_scores[DLEFT]);
  scores.push_back(all_scores[DRIGHT]);
}

void ScorerMSD::score(const vector<double>&  all_scores, vector<double>&  scores) const
{
  scores.push_back(all_scores[MONO]);
  scores.push_back(all_scores[SWAP]);
  scores.push_back(all_scores[DRIGHT]+all_scores[DLEFT]+all_scores[OTHER]);
}

void ScorerMonotonicity::score(const vector<double>&  all_scores, vector<double>&  scores) const
{
  scores.push_back(all_scores[MONO]);
  scores.push_back(all_scores[SWAP]+all_scores[DRIGHT]+all_scores[DLEFT]+all_scores[OTHER]+all_scores[NOMONO]);
}


void ScorerLR::score(const vector<double>&  all_scores, vector<double>&  scores) const
{
  scores.push_back(all_scores[MONO]+all_scores[DRIGHT]);
  scores.push_back(all_scores[SWAP]+all_scores[DLEFT]);
}


void ScorerMSLR::createSmoothing(const vector<double>&  scores, double weight, vector<double>& smoothing) const
{
  double total = accumulate(scores.begin(), scores.end(), 0);
  smoothing.push_back(weight*(scores[MONO]+0.1)/total);
  smoothing.push_back(weight*(scores[SWAP]+0.1)/total);
  smoothing.push_back(weight*(scores[DLEFT]+0.1)/total);
  smoothing.push_back(weight*(scores[DRIGHT]+0.1)/total);
}

void ScorerMSLR::createConstSmoothing(double weight, vector<double>& smoothing) const
{
  for (int i=1; i<=4; ++i) {
    smoothing.push_back(weight);
  }
}


void ScorerMSD::createSmoothing(const vector<double>&  scores, double weight, vector<double>& smoothing) const
{
  double total = accumulate(scores.begin(), scores.end(), 0);
  smoothing.push_back(weight*(scores[MONO]+0.1)/total);
  smoothing.push_back(weight*(scores[SWAP]+0.1)/total);
  smoothing.push_back(weight*(scores[DLEFT]+scores[DRIGHT]+scores[OTHER]+0.1)/total);
}

void ScorerMSD::createConstSmoothing(double weight, vector<double>& smoothing) const
{
  for (int i=1; i<=3; ++i) {
    smoothing.push_back(weight);
  }
}

void ScorerMonotonicity::createSmoothing(const vector<double>&  scores, double weight, vector<double>& smoothing) const
{
  double total = accumulate(scores.begin(), scores.end(), 0);
  smoothing.push_back(weight*(scores[MONO]+0.1)/total);
  smoothing.push_back(weight*(scores[SWAP]+scores[DLEFT]+scores[DRIGHT]+scores[OTHER]+scores[NOMONO]+0.1)/total);
}

void ScorerMonotonicity::createConstSmoothing(double weight, vector<double>& smoothing) const
{
  for (double i=1; i<=2; ++i) {
    smoothing.push_back(weight);
  }
}


void ScorerLR::createSmoothing(const vector<double>&  scores, double weight, vector<double>& smoothing) const
{
  double total = accumulate(scores.begin(), scores.end(), 0);
  smoothing.push_back(weight*(scores[MONO]+scores[DRIGHT]+0.1)/total);
  smoothing.push_back(weight*(scores[SWAP]+scores[DLEFT])/total);
}

void ScorerLR::createConstSmoothing(double weight, vector<double>& smoothing) const
{
  for (int i=1; i<=2; ++i) {
    smoothing.push_back(weight);
  }
}

void Model::score_fe(const string& f, const string& e)
{
  if (!fe)    //Make sure we do not do anything if it is not a fe model
    return;
  fprintf(file,"%s ||| %s ||| ",f.c_str(),e.c_str());
  //condition on the previous phrase
  if (previous) {
    vector<double> scores;
    scorer->score(modelscore->get_scores_fe_prev(), scores);
    double sum = 0;
    for(size_t i=0; i<scores.size(); ++i) {
      scores[i] += smoothing_prev[i];
      sum += scores[i];
    }
    for(size_t i=0; i<scores.size(); ++i) {
      fprintf(file,"%f ",scores[i]/sum);
    }
    //fprintf(file, "||| ");
  }
  //condition on the next phrase
  if (next) {
    vector<double> scores;
    scorer->score(modelscore->get_scores_fe_next(), scores);
    double sum = 0;
    for(size_t i=0; i<scores.size(); ++i) {
      scores[i] += smoothing_next[i];
      sum += scores[i];
    }
    for(size_t i=0; i<scores.size(); ++i) {
      fprintf(file, "%f ", scores[i]/sum);
    }
  }
  fprintf(file,"\n");
}

void Model::score_f(const string& f)
{
  if (fe)      //Make sure we do not do anything if it is not a f model
    return;
  fprintf(file, "%s ||| ", f.c_str());
  //condition on the previous phrase
  if (previous) {
    vector<double> scores;
    scorer->score(modelscore->get_scores_f_prev(), scores);
    double sum = 0;
    for(size_t i=0; i<scores.size(); ++i) {
      scores[i] += smoothing_prev[i];
      sum += scores[i];
    }
    for(size_t i=0; i<scores.size(); ++i) {
      fprintf(file, "%f ", scores[i]/sum);
    }
    //fprintf(file, "||| ");
  }
  //condition on the next phrase
  if (next) {
    vector<double> scores;
    scorer->score(modelscore->get_scores_f_next(), scores);
    double sum = 0;
    for(size_t i=0; i<scores.size(); ++i) {
      scores[i] += smoothing_next[i];
      sum += scores[i];
    }
    for(size_t i=0; i<scores.size(); ++i) {
      fprintf(file, "%f ", scores[i]/sum);
    }
  }
  fprintf(file, "\n");
}

Model::Model(ModelScore* ms, Scorer* sc, const string& dir, const string& lang, const string& fn)
  : modelscore(ms), scorer(sc), filename(fn)
{

  file = fopen(filename.c_str(),"w");
  if (!file) {
    cerr << "Could not open the model output file: " << filename << endl;
    exit(1);
  }

  fe = false;
  if (lang.compare("fe") == 0) {
    fe = true;
  } else if (lang.compare("f") != 0) {
    cerr << "You have given an illegal language to condition on: "  << lang
         << "\nLegal types: fe (on both languages), f (only on source language)\n";
    exit(1);
  }

  previous = true;
  next = true;
  if (dir.compare("backward") == 0) {
    next = false;
  } else if (dir.compare("forward") == 0) {
    previous = false;
  }
}

Model::~Model()
{
  fclose(file);
  delete modelscore;
  delete scorer;
}

void Model::zipFile()
{
  fclose(file);
  file = fopen(filename.c_str(), "rb");
  gzFile gzfile = gzopen((filename+".gz").c_str(),"wb");
  char inbuffer[128];
  int num_read;
  while ((num_read = fread(inbuffer, 1, sizeof(inbuffer), file)) > 0) {
    gzwrite(gzfile, inbuffer, num_read);
  }
  fclose(file);
  gzclose(gzfile);

  //Remove the unzipped file
  remove(filename.c_str());
}

void Model::split_config(const string& config, string& dir, string& lang, string& orient)
{
  istringstream is(config);
  string type;
  getline(is, type, '-');
  getline(is, orient, '-');
  getline(is, dir, '-');
  getline(is, lang, '-');
}

Model* Model::createModel(ModelScore* modelscore, const string& config, const string& filepath)
{
  string dir, lang, orient, filename;
  split_config(config,dir,lang,orient);

  filename = filepath + config;
  if (orient.compare("mslr") == 0) {
    return new Model(modelscore, new ScorerMSLR(), dir, lang, filename);
  } else if (orient.compare("msd") == 0) {
    return new Model(modelscore, new ScorerMSD(), dir, lang, filename);
  } else if (orient.compare("monotonicity") == 0) {
    return new Model(modelscore, new ScorerMonotonicity(), dir, lang, filename);
  } else if (orient.compare("leftright") == 0) {
    return new Model(modelscore, new ScorerLR(), dir, lang, filename);
  } else {
    cerr << "Illegal orientation type of reordering model: " << orient
         << "\n allowed types: mslr, msd, monotonicity, leftright\n";
    exit(1);
  }
}



void Model::createSmoothing(double w)
{
  scorer->createSmoothing(modelscore->get_scores_fe_prev(), w, smoothing_prev);
  scorer->createSmoothing(modelscore->get_scores_fe_next(), w, smoothing_next);
}

void Model::createConstSmoothing(double w)
{
  scorer->createConstSmoothing(w, smoothing_prev);
  scorer->createConstSmoothing(w, smoothing_next);
}
