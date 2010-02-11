
#include <vector>
#include <iostream>
#include <cstdlib>
#include <numeric>
#include <cstdio>
//#include <iostream>
#include <sstream>
#include <string>
#include "zlib.h"

#include "reordering_classes.h"

using namespace std;

ModelScore::ModelScore() {
  for(int i=MONO; i<=NOMONO; ++i) {
    count_fe_prev.push_back(0);
    count_fe_next.push_back(0);
    count_f_prev.push_back(0);
    count_f_next.push_back(0);
  }
}

ModelScore* ModelScore::createModelScore(const string& modeltype) {
  if (modeltype.compare("mslr") == 0) {
    return new ModelScoreMSLR();
  } else if (modeltype.compare("msd") == 0) {
    return new ModelScoreMSD();
  } else if (modeltype.compare("monotonicity") == 0 ) {
    return new ModelScoreMonotonicity();
  } else if (modeltype.compare("leftright") == 0) {
    return new ModelScoreLR();
  } else {
    cerr << "Illegal model type given for lexical reordering model scoring: " << modeltype << endl;
    exit(1);
  }
}

void ModelScore::reset_fe() {
  for(int i=MONO; i<=NOMONO; ++i) {
    count_fe_prev[i] = 0;
    count_fe_next[i] = 0;
  }
}

void ModelScore::reset_f() {
  for(int i=MONO; i<=NOMONO; ++i) {
    count_f_prev[i] = 0;
    count_f_next[i] = 0;
  }
}

void ModelScore::add_example(const std::string& previous, std::string& next) { 
  count_fe_prev[getType(previous)]++;
  count_f_prev[getType(previous)]++;
  count_fe_next[getType(next)]++;
  count_f_next[getType(next)]++;
}

const std::vector<double>& ModelScore::get_scores_fe_prev() const {
  return count_fe_prev;
}

const std::vector<double>& ModelScore::get_scores_fe_next() const {
  return count_fe_next;
}

const std::vector<double>& ModelScore::get_scores_f_prev() const {
  return count_f_prev;
}

const std::vector<double>& ModelScore::get_scores_f_next() const {
  return count_f_next;
}


ORIENTATION ModelScore::getType(const std::string& s) {
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


ORIENTATION ModelScoreMSLR::getType(const std::string& s) {
  if (s.compare("mono") == 0) {
    return MONO;
  } else if (s.compare("swap") == 0) {
    return SWAP;
  } else if (s.compare("dright") == 0) {
    return DRIGHT;
  } else if (s.compare("dleft") == 0) {
    return DLEFT;
  } else if (s.compare("other") == 0 || s.compare("nomono") == 0) {
    cerr << "Illegal reordering type used: " << s << " for model type MSLR" <<  endl;
    exit(1);   
  } else {
    cerr << "Illegal reordering type used: " << s << endl;
    exit(1);
  }
}


ORIENTATION ModelScoreLR::getType(const std::string& s) {
  if (s.compare("mono") == 0 || s.compare("dright") == 0) {
    return DRIGHT;
  } else if (s.compare("swap") == 0 || s.compare("dleft") == 0) {
    return DLEFT;
  } else if (s.compare("other") == 0 || s.compare("nomono") == 0) {
    cerr << "Illegal reordering type used: " << s << " for model type LeftRight" <<  endl;
    exit(1);
  } else {
    cerr << "Illegal reordering type used: " << s << endl;
    exit(1);
  }
}


ORIENTATION ModelScoreMSD::getType(const std::string& s) {
  if (s.compare("mono") == 0) {
    return MONO;
  } else if (s.compare("swap") == 0) {
    return SWAP;
  } else if (s.compare("dleft") == 0 ||
	     s.compare("dright") == 0 || 
	     s.compare("other") == 0) {
    return OTHER;
  } else if (s.compare("nomono") == 0) {
    cerr << "Illegal reordering type used: " << s << " for model type MSD" <<  endl;
    exit(1);
  } else {
    cerr << "Illegal reordering type used: " << s << endl;
    exit(1);
  }
}

ORIENTATION ModelScoreMonotonicity::getType(const std::string& s) {
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


std::vector<double> ScorerMSLR::createSmoothing(std::vector<double> scores, double weight) const {
  double total = accumulate(scores.begin(), scores.end(), 0);
  vector<double> res;
  res.push_back(weight*(scores[MONO]+0.1)/total);
  res.push_back(weight*(scores[SWAP]+0.1)/total);
  res.push_back(weight*(scores[DRIGHT]+0.1)/total);
  res.push_back(weight*(scores[DLEFT]+0.1)/total);
  return res;
}

std::vector<double> ScorerMSLR::createConstSmoothing(double weight) const {
  vector<double> smoothing;
  for (int i=1; i<=4; ++i) {
    smoothing.push_back(weight);
  }
  return smoothing;
}


std::vector<double> ScorerMSD::createSmoothing(std::vector<double> scores, double weight) const {
  double total = accumulate(scores.begin(), scores.end(), 0);
  vector<double> res;
  res.push_back(weight*(scores[MONO]+0.1)/total);
  res.push_back(weight*(scores[SWAP]+0.1)/total);
  res.push_back(weight*(scores[DLEFT]+scores[DRIGHT]+scores[OTHER]+0.1)/total);
  return res;
}

std::vector<double> ScorerMSD::createConstSmoothing(double weight) const {
  vector<double> smoothing;
  for (int i=1; i<=3; ++i) {
    smoothing.push_back(weight);
  }
  return smoothing;
}

std::vector<double> ScorerMonotonicity::createSmoothing(std::vector<double> scores, double weight) const {
  double total = accumulate(scores.begin(), scores.end(), 0);
  vector<double> res;
  res.push_back(weight*(scores[MONO]+0.1)/total);
  res.push_back(weight*(scores[SWAP]+scores[DLEFT]+scores[DRIGHT]+scores[OTHER]+scores[NOMONO]+0.1)/total);
  return res;
}

std::vector<double> ScorerMonotonicity::createConstSmoothing(double weight) const {
  vector<double> smoothing;
  for (double i=1; i<=2; ++i) {
    smoothing.push_back(weight);
  }
  return smoothing;
}


std::vector<double> ScorerLR::createSmoothing(std::vector<double> scores, double weight) const {
  double total = accumulate(scores.begin(), scores.end(), 0);
  vector<double> res;
  res.push_back(weight*(scores[MONO]+scores[DRIGHT]+0.1)/total);
  res.push_back(weight*(scores[SWAP]+scores[DLEFT])/total);
  return res;
}

std::vector<double> ScorerLR::createConstSmoothing(double weight) const {
  vector<double> smoothing;
  for (int i=1; i<=2; ++i) {
    smoothing.push_back(weight);
  }
  return smoothing;
}
  
std::vector<double> ScorerMSLR::score(vector<double> all_scores) const {
  vector<double> s;
  s.push_back(all_scores[MONO]);
  s.push_back(all_scores[SWAP]);
  s.push_back(all_scores[DRIGHT]);
  s.push_back(all_scores[DLEFT]);
  return s;
}
  
std::vector<double> ScorerMSD::score(vector<double> all_scores) const {
  vector<double> s;
  s.push_back(all_scores[MONO]);
  s.push_back(all_scores[SWAP]);
  s.push_back(all_scores[DRIGHT]+all_scores[DLEFT]+all_scores[OTHER]);
  return s;
}

std::vector<double> ScorerMonotonicity::score(vector<double> all_scores) const {
  vector<double> s;
  s.push_back(all_scores[MONO]);
  s.push_back(all_scores[SWAP]+all_scores[DRIGHT]+all_scores[DLEFT]+all_scores[OTHER]+all_scores[NOMONO]);
  return s;
}

  
std::vector<double> ScorerLR::score(vector<double> all_scores) const {
  vector<double> s;
  s.push_back(all_scores[MONO]+all_scores[DRIGHT]);
  s.push_back(all_scores[SWAP]+all_scores[DLEFT]);
  return s;
}

void Model::score_fe(const string& f, const string& e)  {
  if (!fe)    //Make sure we do not do anything if it is not a fe model
    return;
  //file >> f >> " " >> e >> " ||| ";
  fprintf(file,"%s ||| %s ||| ",f.c_str(),e.c_str());
  //condition on the previous phrase
  if (previous) {
    vector<double> scores = scorer->score(modelscore->get_scores_fe_prev());
    double sum = 0;
    for(int i=0; i<scores.size(); ++i) {
      scores[i] += smoothing_prev[i];
      sum += scores[i];
    }
    for(int i=0; i<scores.size(); ++i) {
      //file >> scores[i]/sum >> " ";
      fprintf(file,"%f ",scores[i]/sum);
    }
  }
  //condition on the next phrase
  if (next) {
    //file >> "||| ";
    fprintf(file, "||| ");
    vector<double> scores = scorer->score(modelscore->get_scores_fe_next());
    double sum = 0;
    for(int i=0; i<scores.size(); ++i) {
      scores[i] += smoothing_next[i];
      sum += scores[i];
    }
    for(int i=0; i<scores.size(); ++i) {
      //file >> scores[i]/sum >> " ";
      fprintf(file, "%f ", scores[i]/sum);
    }
  }
  //file >> "\n";
  fprintf(file,"\n");
}

void Model::score_f(const string& f) {
  if (fe)      //Make sure we do not do anything if it is not a f model
    return;
  //file >> f >> " ||| ";
  fprintf(file, "%s ||| ", f.c_str());
  //condition on the previous phrase
  if (previous) {
    vector<double> scores = scorer->score(modelscore->get_scores_f_prev());
    double sum = 0;
    for(int i=0; i<scores.size(); ++i) {
      scores[i] += smoothing_prev[i];
      sum += scores[i];
    }
    for(int i=0; i<scores.size(); ++i) {
      fprintf(file, "%f ", scores[i]/sum);
    }
  }
  //condition on the next phrase
  if (next) {
    //file >> "||| ";
    fprintf(file, "||| ");
    vector<double> scores = scorer->score(modelscore->get_scores_f_next());
    double sum = 0;
    for(int i=0; i<scores.size(); ++i) {
      scores[i] += smoothing_next[i];
      sum += scores[i];
    }
    for(int i=0; i<scores.size(); ++i) {
      //file >> scores[i]/sum >> " ";
      fprintf(file, "%f ", scores[i]/sum);
    }
  }
  //file >> "\n";
  fprintf(file, "\n");
}

Model::Model(ModelScore* ms, Scorer* sc, const string& dir, const string& lang, const string& fn)
  : modelscore(ms), scorer(sc), filename(fn) {

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

Model::~Model() {
  fclose(file);
  delete modelscore;
  delete scorer;
}

void Model::zipFile() {
  fclose(file);
  file = fopen(filename.c_str(), "rb");
  FILE* gzfile = (FILE*) gzopen((filename+".gz").c_str(),"wb");
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

void Model::split_config(const string& config, string& dir, string& lang, string& orient) {
  istringstream is(config);
  string type;
  getline(is, type, '-');
  getline(is, orient, '-');
  getline(is, dir, '-');
  getline(is, lang, '-');
}

Model* Model::createModel(ModelScore* modelscore, const std::string& config, const std::string& filepath) {
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

void Model::createSmoothing(double w)  {
  smoothing_prev = scorer->createSmoothing(modelscore->get_scores_fe_prev(),w);
  smoothing_next = scorer->createSmoothing(modelscore->get_scores_fe_prev(),w);
}

void Model::createConstSmoothing(double w)  {
  vector<double> i;
  smoothing_prev = scorer->createConstSmoothing(w);
  smoothing_next = scorer->createConstSmoothing(w);
}
