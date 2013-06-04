#ifndef MERT_TYPE_H_
#define MERT_TYPE_H_

#include <vector>
#include <map>
#include <string>
#include <utility>

namespace MosesTuning
{

class FeatureStats;
class FeatureArray;
class FeatureData;
class ScoreStats;
class ScoreArray;
class ScoreData;

typedef float parameter_t;
//typedef std::vector<parameter_t> parameters_t;confusing; use std::vector<parameter_t>
typedef std::vector<std::pair<unsigned int, unsigned int> > diff_t;
typedef std::pair<float,diff_t > threshold;
typedef std::vector<diff_t> diffs_t;
typedef std::vector<unsigned int> candidates_t;

typedef float statscore_t;
typedef std::vector<statscore_t> statscores_t;


typedef float FeatureStatsType;
typedef FeatureStatsType* featstats_t;
//typedef std::vector<FeatureStatsType> featstats_t;
typedef std::vector<FeatureStats> featarray_t;
typedef std::vector<FeatureArray> featdata_t;

typedef int ScoreStatsType;
typedef ScoreStatsType* scorestats_t;
//typedef std::vector<ScoreStatsType> scorestats_t;
typedef std::vector<ScoreStats> scorearray_t;
typedef std::vector<ScoreArray> scoredata_t;

typedef std::map<std::size_t, int> idx2name;
typedef std::map<int, std::size_t> name2idx;

typedef enum { HAMMING_DISTANCE=0, KENDALL_DISTANCE } distanceMetric_t;
typedef enum { REFERENCE_CHOICE_AVERAGE=0, REFERENCE_CHOICE_CLOSEST } distanceMetricReferenceChoice_t;

}

#endif  // MERT_TYPE_H_
