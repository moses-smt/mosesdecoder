#ifndef __SCORER_H__
#define __SCORER_H__

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "Types.h"
#include "ScoreData.h"

using namespace std;

enum ScorerRegularisationStrategy {REG_NONE, REG_AVERAGE, REG_MINIMUM};

class ScoreStats;

/**
  * Superclass of all scorers and dummy implementation. In order to add a new
  * scorer it should be sufficient to override prepareStats(), setReferenceFiles()
  * and score() (or calculateScore()). 
**/
class Scorer {
  private:
      string _name;
    
  public:
        
    Scorer(const string& name, const string& config): _name(name), _scoreData(0), _preserveCase(true){ 
        cerr << "Scorer config string: " << config << endl;
        size_t start = 0;
        while (start < config.size()) {
            size_t end = config.find(",",start);
            if (end == string::npos) {
                end = config.size();
            }
            string nv = config.substr(start,end-start);
            size_t split = nv.find(":");
            if (split == string::npos) {
                throw runtime_error("Missing colon when processing scorer config: " + config);
            }
            string name = nv.substr(0,split);
            string value = nv.substr(split+1,nv.size()-split-1);
            cerr << "name: " << name << " value: " << value << endl;
            _config[name] = value;
            start = end+1;
        }

        };
        virtual ~Scorer(){};


        /**
            * returns the number of statistics needed for the computation of the score
            **/
        virtual size_t NumberOfScores(){ cerr << "Scorer: 0" << endl; return 0; };
        
        /**
          * set the reference files. This must be called before prepareStats.
          **/
        virtual void setReferenceFiles(const vector<string>& referenceFiles) {
            //do nothing
        }

        /**
         * Process the given guessed text, corresponding to the given reference sindex
         * and add the appropriate statistics to the entry.
        **/
        virtual void prepareStats(size_t sindex, const string& text, ScoreStats& entry)
        {}

        virtual void prepareStats(const string& sindex, const string& text, ScoreStats& entry)
        {
            
//            cerr << sindex << endl;
            this->prepareStats((size_t) atoi(sindex.c_str()), text, entry);
            //cerr << text << std::endl;
        }
                
        /**
          * Score using each of the candidate index, then go through the diffs
          * applying each in turn, and calculating a new score each time.
          **/
        virtual void score(const candidates_t& candidates, const diffs_t& diffs,
                statscores_t& scores) {
            //dummy impl
            if (!_scoreData) {
                throw runtime_error("score data not loaded");
            }
            scores.push_back(0);
            for (size_t i = 0; i < diffs.size(); ++i) {
                scores.push_back(0);
            }
        }


        /**
          * Calculate the score of the sentences corresponding to the list of candidate
          * indices. Each index indicates the 1-best choice from the n-best list.
          **/
        float score(const candidates_t& candidates) {
            diffs_t diffs;
            statscores_t scores;
            score(candidates, diffs, scores);
            return scores[0];
        }

        const string& getName() const {return _name;}

        size_t getReferenceSize() {
            if (_scoreData) {
                return _scoreData->size();
            }
            return 0;
        }
        

        /**
          * Set the score data, prior to scoring.
          **/
        void setScoreData(ScoreData* data) {
            _scoreData = data;
        }

        protected:
      typedef map<string,int> encodings_t;
      typedef map<string,int>::iterator encodings_it;

      ScoreData* _scoreData;
      encodings_t _encodings;

      bool _preserveCase;

      /**
        * Value of config variable. If not provided, return default.
        **/
      string getConfig(const string& key, const string& def="") {
          map<string,string>::iterator i = _config.find(key);
          if (i  == _config.end()) {
              return def;
          } else {
              return i->second;
          }
      }
    

     /**
      * Tokenise line and encode.
      *     Note: We assume that all tokens are separated by single spaces
      **/
      void encode(const string& line, vector<int>& encoded) {
        //cerr << line << endl;
        istringstream in (line);
        string token;
        while (in >> token) {
            if (!_preserveCase) {
                for (string::iterator i = token.begin(); i != token.end(); ++i) {
                    *i = tolower(*i);
                }
            }
            encodings_it encoding = _encodings.find(token);
            int encoded_token;
            if (encoding == _encodings.end()) {
                encoded_token = (int)_encodings.size();
                _encodings[token] = encoded_token;
                //cerr << encoded_token << "(n) ";
            } else {
                encoded_token = encoding->second;
                //cerr << encoded_token << " ";
            }
            encoded.push_back(encoded_token);
        }
        //cerr << endl;
    }

    private:
         map<string,string> _config;


};



/**
  * Abstract base class for scorers that work by adding statistics across all 
  * outout sentences, then apply some formula, e.g. bleu, per. **/
class StatisticsBasedScorer : public Scorer {

    public:
  StatisticsBasedScorer(const string& name, const string& config): Scorer(name,config) {
    //configure regularisation
    static string KEY_TYPE = "regtype";
    static string KEY_WINDOW = "regwin";
    static string KEY_CASE = "case";
    static string TYPE_NONE = "none";
    static string TYPE_AVERAGE = "average";
    static string TYPE_MINIMUM = "min";
    static string TRUE = "true";
    static string FALSE = "false";
    
    
    string type = getConfig(KEY_TYPE,TYPE_NONE);
    if (type == TYPE_NONE) {
        _regularisationStrategy = REG_NONE;
    } else if (type == TYPE_AVERAGE) {
        _regularisationStrategy = REG_AVERAGE;
    } else if (type == TYPE_MINIMUM) {
        _regularisationStrategy = REG_MINIMUM;
    } else {
        throw runtime_error("Unknown scorer regularisation strategy: " + type);
    }
    cerr << "Using scorer regularisation strategy: " << type << endl;

    string window = getConfig(KEY_WINDOW,"0");
    _regularisationWindow = atoi(window.c_str());
    cerr << "Using scorer regularisation window: " << _regularisationWindow << endl;
    
    string preservecase = getConfig(KEY_CASE,TRUE);
    if (preservecase == TRUE) {
        _preserveCase = true;
    }else if (preservecase == FALSE) {
        _preserveCase = false;
    }
    cerr << "Using case preservation: " << _preserveCase << endl;


  }
    ~StatisticsBasedScorer(){};
    virtual void score(const candidates_t& candidates, const diffs_t& diffs,
                statscores_t& scores);

    protected:
        //calculate the actual score
        virtual statscore_t calculateScore(const vector<int>& totals) = 0;

        //regularisation
        ScorerRegularisationStrategy _regularisationStrategy;
        size_t  _regularisationWindow;

};


#endif //__SCORER_H
