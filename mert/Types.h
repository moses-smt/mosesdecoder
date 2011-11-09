#ifndef TYPE_H
#define TYPE_H
#include <vector>
#include <map>
#include <string>

using namespace std;

class FeatureStats;
class FeatureArray;
class FeatureData;
class ScoreStats;
class ScoreArray;
class ScoreData;

typedef float parameter_t;
//typedef vector<parameter_t> parameters_t;confusing; use vector<parameter_t>
typedef vector<pair<unsigned int, unsigned int> > diff_t;
typedef pair<float,diff_t > threshold;
typedef vector<diff_t> diffs_t;
typedef vector<unsigned int> candidates_t;

typedef float statscore_t;
typedef vector<statscore_t> statscores_t;


typedef float FeatureStatsType;
typedef FeatureStatsType* featstats_t;
//typedef vector<FeatureStatsType> featstats_t;
typedef vector<FeatureStats> featarray_t;
typedef vector<FeatureArray> featdata_t;

typedef int ScoreStatsType;
typedef ScoreStatsType* scorestats_t;
//typedef vector<ScoreStatsType> scorestats_t;
typedef vector<ScoreStats> scorearray_t;
typedef vector<ScoreArray> scoredata_t;

typedef map<size_t, std::string> idx2name;
typedef map<std::string, size_t> name2idx;

#endif
