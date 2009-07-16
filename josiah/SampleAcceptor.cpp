#include "SampleAcceptor.h"
#include "TranslationDelta.h"

using namespace std;

namespace Josiah {
  
void SampleAcceptor::getScores(const vector<TranslationDelta*>& deltas, vector<double>& scores) {
  for (vector<TranslationDelta*>::const_iterator i = deltas.begin(); i != deltas.end(); ++i) {
    scores.push_back((**i).getScore());
  }
}

void SampleAcceptor::normalize(vector<double>& scores) {
  double sum = scores[0];
  for (size_t i = 1; i < scores.size(); ++i) {
    sum = log_sum(sum,scores[i]);
  }
  transform(scores.begin(),scores.end(),scores.begin(),bind2nd(minus<double>(),sum));
}

double SampleAcceptor::getRandom() {
  double random =  RandomNumberGenerator::instance().next();//(double)rand() / RAND_MAX;
  random = log(random);
  return random;  
}

size_t SampleAcceptor::getSample(const vector<double>& scores, double random) {
  size_t position = 1;
  double sum = scores[0];
  for (; position < scores.size() && sum < random; ++position) {
    sum = log_sum(sum,scores[position]);
  }
  
  size_t chosen =  position-1;
  VERBOSE(3,"The chosen sample is " << chosen << endl);
  return chosen;
}
  
size_t FixedTempAcceptor::choose(const vector<TranslationDelta*>& deltas) {
  
  //get the scores
  vector<double> scores;
  getScores(deltas, scores);
  //do annealling
  transform(scores.begin(),scores.end(),scores.begin(),bind2nd(multiplies<double>(), 1.0/m_temp));
  normalize(scores);
  
  double random = getRandom();
  size_t chosen = getSample(scores, random);
  
  return chosen;
}

size_t RegularAcceptor::choose(const vector<TranslationDelta*>& deltas) {
  
  //get the scores
  vector<double> scores;
  getScores(deltas, scores);
  normalize(scores);
  
  double random = getRandom();
  size_t chosen = getSample(scores, random);
  
  return chosen;
}


size_t GreedyAcceptor::choose(const vector<TranslationDelta*>& deltas) {
  
  size_t chosen = maxScore(deltas);
  return chosen;
}


size_t GreedyAcceptor::maxScore(const vector<TranslationDelta*>& deltas) {
  
  size_t best(0);
  float bestScore = -1e10;
  float score;
  
  for (vector<TranslationDelta*>::const_iterator i = deltas.begin(); i != deltas.end(); ++i) {
    score = (**i).getScore();
    if (score > bestScore) {
      bestScore = score;
      best =  i - deltas.begin();
    }
  }
  return best;
}

size_t BestNeighbourTgtAssigner::getTarget(const vector<TranslationDelta*>& deltas, const TranslationDelta* noChangeDelta) {
  //Only do best neighbour for the moment
  float bestGain = -1;
  int bestGainIndex = -1;
  for (vector<TranslationDelta*>::const_iterator i = deltas.begin(); i != deltas.end(); ++i) {
    if ((*i)->getGain() > bestGain) {
      bestGain = (*i)->getGain();
      bestGainIndex = i - deltas.begin();
    }
  }
  IFVERBOSE(1) {
    if (bestGainIndex > -1) {
      cerr << "best nbr has score " << deltas[bestGainIndex]->getScore() << " and gain " << deltas[bestGainIndex]->getGain() << endl;
      cerr << "No change has score " << noChangeDelta->getScore() << " and gain " << noChangeDelta->getGain() << endl;    
    }
  }
  return bestGainIndex;
}

size_t ClosestBestNeighbourTgtAssigner::getTarget(const vector<TranslationDelta*>& deltas, const TranslationDelta* noChangeDelta) {
  //Only do best neighbour for the moment
  float minScoreDiff = 10e10;
  int closestBestNbr = -1;
  float noChangeGain = noChangeDelta->getGain();
  float noChangeScore = noChangeDelta->getScore(); 
  for (vector<TranslationDelta*>::const_iterator i = deltas.begin(); i != deltas.end(); ++i) {
    if ((*i)->getGain() > noChangeGain ) {
      float scoreDiff = noChangeScore -  (*i)->getScore();
      if (scoreDiff < minScoreDiff) {
        minScoreDiff = scoreDiff;
        closestBestNbr = i - deltas.begin();
      }
    }
  }
  
  IFVERBOSE(1) {
    if(closestBestNbr > -1) {
      cerr << "Closest best nbr has score " << deltas[closestBestNbr]->getScore() << " and gain " << deltas[closestBestNbr]->getGain() << endl;
      cerr << "No change has score " << noChangeDelta->getScore() << " and gain " << noChangeDelta->getGain() << endl;     
    } 
  }
  return closestBestNbr;
}
  
}