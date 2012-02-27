#include "ScorerFactory.h"
#include "InterpolatedScorer.h"
#include "Util.h"

using namespace std;


InterpolatedScorer::InterpolatedScorer (const string& name, const string& config): Scorer(name,config)
{

  // name would be: HAMMING,BLEU or similar
  string scorers = name;
  while (scorers.length() > 0) {
    string scorertype = "";
    getNextPound(scorers,scorertype,",");
    Scorer *theScorer=ScorerFactory::getScorer(scorertype,config);
    _scorers.push_back(theScorer);
  }
  if (_scorers.size() == 0) {
    throw runtime_error("There are no scorers");
  }
  cerr << "Number of scorers: " << _scorers.size() << endl;

  //TODO debug this
  string wtype = getConfig("weights","");
  //Default weights set to uniform ie. if two weights 0.5 each
  //weights should add to 1
  if (wtype.length() == 0) {
    float weight = 1.0/_scorers.size() ;
    //cout << " Default weights:" << weight << endl;
    for (size_t i = 0; i < _scorers.size(); i ++) {
      _scorerWeights.push_back(weight);
    }
  } else {
    float tot=0;
    //cout << "Defined weights:"  << endl;
    while (wtype.length() > 0) {
      string scoreweight = "";
      getNextPound(wtype,scoreweight,"+");
      float weight = atof(scoreweight.c_str());
      _scorerWeights.push_back(weight);
      tot += weight;
      //cout << " :" << weight ;
    }
    //cout << endl;
    if (tot != float(1)) {
      for (vector<float>::iterator it = _scorerWeights.begin(); it != _scorerWeights.end(); ++it)
      {
        *it /= tot;
      }
    }

    if (_scorers.size() != _scorerWeights.size()) {
      throw runtime_error("The number of weights does not equal the number of scorers!");
    }
  }
  cerr << "The weights for the interpolated scorers are: " << endl;
  for (vector<float>::iterator it = _scorerWeights.begin(); it < _scorerWeights.end(); it++) {
    cerr << *it << " " ;
  }
  cerr <<endl;
}

void InterpolatedScorer::setScoreData(ScoreData* data)
{
  size_t last = 0;
  m_score_data = data;
  for (ScopedVector<Scorer>::iterator itsc = _scorers.begin(); itsc!=_scorers.end(); itsc++) {
    int numScoresScorer = (*itsc)->NumberOfScores();
    ScoreData* newData =new ScoreData(**itsc);
    for (size_t i = 0; i < data->size(); i++) {
      ScoreArray scoreArray = data->get(i);
      ScoreArray newScoreArray;
      std::string istr;
      std::stringstream out;
      out << i;
      istr = out.str();
      size_t numNBest = scoreArray.size();
      //cout << " Datasize " << data->size() <<  " NumNBest " << numNBest << endl ;
      for (size_t j = 0; j < numNBest ; j++) {
        ScoreStats scoreStats = data->get(i, j);
        //cout << "Scorestats " << scoreStats << " i " << i << " j " << j << endl;
        ScoreStats newScoreStats;
        for (size_t k = last; k < size_t(numScoresScorer + last); k++) {
          ScoreStatsType score = scoreStats.get(k);
          newScoreStats.add(score);
        }
        //cout << " last " << last << " NumScores " << numScoresScorer << "newScorestats " << newScoreStats << endl;
        newScoreArray.add(newScoreStats);
      }
      newScoreArray.setIndex(istr);
      newData->add(newScoreArray);
    }
    //newData->dump();

    // NOTE: This class takes the ownership of the heap allocated
    // ScoreData objects to avoid the memory leak issues.
    m_scorers_score_data.push_back(newData);

    (*itsc)->setScoreData(newData);
    last += numScoresScorer;
  }
}


/** The interpolated scorer calls a vector of scorers and combines them with
    weights **/
void InterpolatedScorer::score(const candidates_t& candidates, const diffs_t& diffs,
                               statscores_t& scores) const
{
  //cout << "*******InterpolatedScorer::score" << endl;
  size_t scorerNum = 0;
  for (ScopedVector<Scorer>::const_iterator itsc =  _scorers.begin(); itsc!=_scorers.end(); itsc++) {
    //int numScores = (*itsc)->NumberOfScores();
    statscores_t tscores;
    (*itsc)->score(candidates,diffs,tscores);
    size_t inc = 0;
    for (statscores_t::iterator itstatsc =  tscores.begin(); itstatsc!=tscores.end(); itstatsc++) {
      //cout << "Scores " << (*itstatsc) << endl;
      float weight = _scorerWeights[scorerNum];
      if (weight == 0) {
        stringstream msg;
        msg << "No weights for scorer" << scorerNum ;
        throw runtime_error(msg.str());
      }
      if (scorerNum == 0) {
        scores.push_back(weight * (*itstatsc));
      } else {
        scores[inc] +=  weight * (*itstatsc);
      }
      //cout << "Scorer:" << scorerNum <<  " scoreNum:" << inc << " score: " << (*itstatsc) << " weight:" << weight << endl;
      inc++;

    }
    scorerNum++;
  }

}

void InterpolatedScorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  for (ScopedVector<Scorer>::iterator itsc =  _scorers.begin(); itsc!=_scorers.end(); itsc++) {
    (*itsc)->setReferenceFiles(referenceFiles);
  }
}

void InterpolatedScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  stringstream buff;
  int i=0;
  for (ScopedVector<Scorer>::iterator itsc =  _scorers.begin(); itsc!=_scorers.end(); itsc++) {
    ScoreStats tempEntry;
    (*itsc)->prepareStats(sid, text, tempEntry);
    if (i > 0) buff <<  " ";
    buff << tempEntry;
    i++;
  }
  //cout << " Scores for interpolated: " << buff << endl;
  string str = buff.str();
  entry.set(str);
}
