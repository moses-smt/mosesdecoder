/*
 * reordering_classes.h
 * Utility classes for lexical reordering table scoring
 *
 *      Created by: Sara Stymne - Linköping University
 *      Machine Translation Marathon 2010, Dublin
 */

#pragma once

#include <vector>
#include <string>
#include <fstream>

#include "util/string_piece.hh"


enum ORIENTATION {MONO, SWAP, DRIGHT, DLEFT, OTHER, NOMONO};


//Keeps the counts for the different reordering types
//(Instantiated in 1-3 instances, one for each type of model (hier, phrase, wbe))
class ModelScore
{
private:
  std::vector<double> count_fe_prev;
  std::vector<double> count_fe_next;
  std::vector<double> count_f_prev;
  std::vector<double> count_f_next;

protected:
  virtual ORIENTATION getType(const StringPiece& s);

public:
  ModelScore();
  virtual ~ModelScore();
  void add_example(const StringPiece& previous, const StringPiece& next, float weight);
  void reset_fe();
  void reset_f();
  const std::vector<double>& get_scores_fe_prev() const;
  const std::vector<double>& get_scores_fe_next() const;
  const std::vector<double>& get_scores_f_prev() const;
  const std::vector<double>& get_scores_f_next() const;

  static ModelScore* createModelScore(const std::string& modeltype);
};

class ModelScoreMSLR : public ModelScore
{
protected:
  virtual ORIENTATION getType(const StringPiece& s);
};

class ModelScoreLR : public ModelScore
{
protected:
  virtual ORIENTATION getType(const StringPiece& s);
};

class ModelScoreMSD : public ModelScore
{
protected:
  virtual ORIENTATION getType(const StringPiece& s);
};

class ModelScoreMonotonicity : public ModelScore
{
protected:
  virtual ORIENTATION getType(const StringPiece& s);
};

//Class for calculating total counts, and to calculate smoothing
class Scorer
{
public:
  virtual ~Scorer() {}
  virtual void score(const std::vector<double>&, std::vector<double>&) const = 0;
  virtual void createSmoothing(const std::vector<double>&, double, std::vector<double>&) const = 0;
  virtual void createConstSmoothing(double, std::vector<double>&) const = 0;
};

class ScorerMSLR : public Scorer
{
public:
  virtual void score(const std::vector<double>&, std::vector<double>&) const;
  virtual void createSmoothing(const std::vector<double>&, double, std::vector<double>&) const;
  virtual void createConstSmoothing(double, std::vector<double>&) const;
};

class ScorerMSD : public Scorer
{
public:
  virtual void score(const std::vector<double>&, std::vector<double>&) const;
  virtual void createSmoothing(const std::vector<double>&, double, std::vector<double>&) const;
  virtual void createConstSmoothing(double, std::vector<double>&) const;
};

class ScorerMonotonicity : public Scorer
{
public:
  virtual void score(const std::vector<double>&, std::vector<double>&) const;
  virtual void createSmoothing(const std::vector<double>&, double, std::vector<double>&) const;
  virtual void createConstSmoothing(double, std::vector<double>&) const;
};

class ScorerLR : public Scorer
{
public:
  virtual void score(const std::vector<double>&, std::vector<double>&) const;
  virtual void createSmoothing(const std::vector<double>&, double, std::vector<double>&) const;
  virtual void createConstSmoothing(double, std::vector<double>&) const;
};


//Class for representing each model
//Contains a modelscore and scorer (which can be of different model types (mslr, msd...)),
//and file handling.
//This class also keeps track of bidirectionality, and which language to condition on
class Model
{
private:
  ModelScore* modelscore;
  Scorer* scorer;

  std::FILE* file;
  std::string filename;

  bool fe;
  bool previous;
  bool next;

  std::vector<double> smoothing_prev;
  std::vector<double> smoothing_next;

  static void split_config(const std::string& config, std::string& dir,
                           std::string& lang, std::string& orient);
public:
  Model(ModelScore* ms, Scorer* sc, const std::string& dir,
        const std::string& lang, const std::string& fn);
  ~Model();
  static Model* createModel(ModelScore*, const std::string&, const std::string&);
  void createSmoothing(double w);
  void createConstSmoothing(double w);
  void score_fe(const std::string& f, const std::string& e);
  void score_f(const std::string& f);
  void zipFile();
};

