#ifndef TYPE_H
#define TYPE_H
#include<vector>
#include <map>

using namespace std;
typedef float parameter_t;
//typedef vector<parameter_t> parameters_t;confusing; use vector<parameter_t>
typedef vector<pair<unsigned int, unsigned int> > diff_t;
typedef pair<float,diff_t > threshold;
typedef vector<diff_t> diffs_t;
typedef vector<unsigned int> candidates_t;
typedef float statscore_t;
typedef vector<statscore_t> statscores_t;

#endif
