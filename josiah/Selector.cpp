/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

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

#include "Selector.h"

#include <fstream>

#include "StaticData.h"
#include "Util.h"

using namespace Moses;
using namespace std;

namespace Josiah {
  
  static void getScores(const TDeltaVector& deltas, vector<double>& scores) {
    for (TDeltaVector::const_iterator i = deltas.begin(); i != deltas.end(); ++i) {
      scores.push_back((*i)->getScore());
    }
  }
  
  static void normalize(vector<double>& scores) {
    double sum = scores[0];
    for (size_t i = 1; i < scores.size(); ++i) {
      sum = log_sum(sum,scores[i]);
    }
    transform(scores.begin(),scores.end(),scores.begin(),bind2nd(minus<double>(),sum));
  }
  
  static void getNormalisedScores(const TDeltaVector& deltas, vector<double>& scores, float temp) {
    getScores(deltas, scores);
    IFVERBOSE(2) {
      cerr << "Before annealing, scores are :";
      copy(scores.begin(),scores.end(),ostream_iterator<double>(cerr," "));
      cerr << endl;
    }
  //do annealling
    transform(scores.begin(),scores.end(),scores.begin(),bind2nd(multiplies<double>(), 1.0/temp));
    IFVERBOSE(2) {
      cerr << "After annealing, scores are :";
      copy(scores.begin(),scores.end(),ostream_iterator<double>(cerr," "));
      cerr << endl;
    }
  
    normalize(scores);
  }
  
  static size_t getSample(const vector<double>& scores, double random) {
    size_t position = 1;
    double sum = scores[0];
    for (; position < scores.size() && sum < random; ++position) {
      sum = log_sum(sum,scores[position]);
    }
  
    size_t chosen =  position-1;
    VERBOSE(3,"The chosen sample is " << chosen << endl);
    return chosen;
  }
  
  SamplingSelector::SamplingSelector() :
      m_annealingSchedule(NULL), m_temperature(1) {}
  
  void SamplingSelector::SetAnnealingSchedule(AnnealingSchedule* annealingSchedule) {
    m_annealingSchedule = annealingSchedule;
  }
  
  void SamplingSelector::SetTemperature(float temperature) {
    assert(temperature != 0);
    m_temperature = temperature;
    m_annealingSchedule = NULL;
  }
  
  TDeltaHandle SamplingSelector::Select(size_t, const TDeltaVector& deltas, const TDeltaHandle&, size_t iteration) 
  {
    float T = m_temperature;
    if (m_annealingSchedule) {
      T = m_annealingSchedule->GetTemperatureAtTime(iteration);
    }
    
    vector<double> scores;
    getNormalisedScores(deltas,scores,T);
  
    double random =  log(RandomNumberGenerator::instance().next());
    size_t chosen = getSample(scores, random);
  
    /*
    cerr << "deltas: " << endl;
    for (size_t i = 0; i < deltas.size(); ++i) {
      cerr << scores[i] << endl;
    }
    cerr << "chosen " << chosen << endl;
    cerr << random << endl;
    */
    
    return deltas[chosen];
  }
  
  
  
  RandomNumberGenerator::RandomNumberGenerator() :m_dist(0,1), m_generator(), m_random(m_generator,m_dist) {
    uint32_t seed;
    std::ifstream r("/dev/urandom");
    if (r) {
      r.read((char*)&seed,sizeof(uint32_t));
    }
    if (r.fail() || !r) {
      std::cerr << "Warning: could not read from /dev/urandom. Seeding from clock" << std::endl;
      seed = time(NULL);
    }
    std::cerr << "Seeding random number sequence to " << seed << endl;
    m_generator.seed(seed);
  }
  
  RandomNumberGenerator RandomNumberGenerator::s_instance;



}
