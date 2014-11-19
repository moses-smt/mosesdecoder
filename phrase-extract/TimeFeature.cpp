#include "TimeFeature.h"
#include "ExtractionPhrasePair.h"
#include "tables-core.h"
#include "InputFileStream.h"

#include <ctime>

using namespace std;

namespace MosesTraining
{

long str2days(const std::string& str) {
  time_t rawtime;
  struct tm * timeinfo;
  int year, month ,day;
  
  sscanf(str.c_str(), "%d-%d-%d", &year, &month, &day);
  
  time (&rawtime);
  timeinfo = localtime ( &rawtime );
  timeinfo->tm_year = year - 1900;
  timeinfo->tm_mon = month - 1;
  timeinfo->tm_mday = day;

  return static_cast<long>(mktime(timeinfo));
}

// handling of dates load database with sentence-id / date info
void Time::load( const std::string &timeFileName )
{
  Moses::InputFileStream fileS( timeFileName );
  istream *fileP = &fileS;

  string line;
  while(getline(*fileP, line)) {
    // read
    vector< string > timeSpecLine = tokenize( line.c_str() );
    int lineNumber;
    if (timeSpecLine.size() != 2 ||
        ! sscanf(timeSpecLine[0].c_str(), "%d", &lineNumber)) {
      std::cerr << "ERROR: in time specification line: '" << line << "'" << endl;
      exit(1);
    }
    // store
    string &date = timeSpecLine[1];
    m_times[lineNumber] = date;
    if(m_epochs.count(date) == 0) {
      long epoch = str2days(date);
      m_epochs[date] = epoch;
       
      //std::cerr << date << " " << epoch << std::endl;
      
      if(epoch < m_oldest) m_oldest = epoch;
      if(epoch > m_newest) m_newest = epoch;
    }
  }
}

// get date based on sentence number
string Time::getTimeOfSentence( int sentenceId ) const
{
  std::map<int, std::string>::const_iterator found = m_times.find(sentenceId);
  if(found != m_times.end()) {
    return found->second;
  }
  return "undefined";
}

long Time::getEpoch(std::string date) const {
  std::map<std::string, long>::const_iterator found = m_epochs.find(date);
  if(found != m_epochs.end()) {
    return found->second;
  }
  return 0;
}

TimeFeature::TimeFeature(const string& timeFile) : m_propertyKey("time")
{
  //process time file
  m_time.load(timeFile);
}

void TimeFeature::addPropertiesToPhrasePair(ExtractionPhrasePair &phrasePair, 
                                              float count, 
                                              int sentenceId) const
{
  std::string value = m_time.getTimeOfSentence(sentenceId);
  phrasePair.AddProperty(m_propertyKey, value, count);
}

void TimeFeature::add(const ScoreFeatureContext& context,
                        std::vector<float>& denseValues,
                        std::map<std::string,float>& sparseValues)  const
{
  const map<string,float> *timeCount = context.phrasePair.GetProperty(m_propertyKey);
  assert( timeCount != NULL );
  add(*timeCount, 
      context.phrasePair.GetCount(), 
      context.maybeLog, 
      denseValues, sparseValues);
}

float TimeFeature::calculate(const map<string, float>& timeCount,
                 TimeFeature::Mapper mapper,
                 TimeFeature::Reducer reducer) const {
  return (this->*reducer)(timeCount, mapper);
}

void TimeFeature::add(const map<string, float>& timeCount,
              float count,
                      const MaybeLog& maybeLog,
                      std::vector<float>& denseValues,
                      std::map<std::string, float>& sparseValues) const
{  
  /*denseValues.push_back(maybeLog(calculate(timeCount,
                       &TimeFeature::hyperbola,
                       &TimeFeature::max)));
  
  denseValues.push_back(maybeLog(calculate(timeCount,
                       &TimeFeature::hyperbola,
                       &TimeFeature::avg)));
  
  denseValues.push_back(maybeLog(calculate(timeCount,
                       &TimeFeature::hyperbola,
                       &TimeFeature::min)));
  */
  denseValues.push_back(maybeLog(calculate(timeCount,
                       &TimeFeature::linear,
                       &TimeFeature::max)));
  /*
  denseValues.push_back(maybeLog(calculate(timeCount,
                       &TimeFeature::linear,
                       &TimeFeature::avg)));
  
  denseValues.push_back(maybeLog(calculate(timeCount,
                       &TimeFeature::linear,
                       &TimeFeature::min)));
  */
}

/* Mappers */

float TimeFeature::hyperbola(const std::string& date) const {
  long delta_seconds = m_time.m_newest - m_time.getEpoch(date);
  long seconds_in_year = 365 * 24 * 60 * 60;
  float years_fraction = delta_seconds / (float)seconds_in_year;
  return 1.0 / (1.0 + years_fraction);
}

float TimeFeature::linear(const std::string& date) const {
  float n = static_cast<float>(m_time.m_newest);
  float o = static_cast<float>(m_time.m_oldest);
  float x = static_cast<float>(m_time.getEpoch(date));
  
  return (x - o) / (n - o);
}

/* Reducers */

float TimeFeature::max(const map<string, float>& timeCount,
               TimeFeature::Mapper mapper) const {
  float newest = 0;
  for(map<string, float>::const_iterator i = timeCount.begin();
      i != timeCount.end(); ++i) {
    float r = (this->*mapper)(i->first);
    if(r > newest) newest = r;
    if(newest == 1)
      return newest;
  }
  return newest;
}

float TimeFeature::min(const map<string, float>& timeCount,
               TimeFeature::Mapper mapper) const {
  float oldest = 1;
  for(map<string, float>::const_iterator i = timeCount.begin();
      i != timeCount.end(); ++i) {
    float r = (this->*mapper)(i->first);
    if(r < oldest) oldest = r;
    if(oldest == 0)
      return oldest;
  }
  return oldest;
}

float TimeFeature::avg(const map<string, float>& timeCount,
               TimeFeature::Mapper mapper) const {
  float sum = 0;
  long n = 0;
  for(map<string, float>::const_iterator i = timeCount.begin();
      i != timeCount.end(); ++i) {
    float r = (this->*mapper)(i->first) * i->second;
    sum += r;
    n += i->second;
  }
  if(sum == 0) return 0;
  return sum / n;
}

}

