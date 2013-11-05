#ifndef __MEGAM_FORMAT_HPP_
#define __MEGAM_FORMAT_HPP_

#include <set>
#include <vector>
#include <string>
#include <map>
#include <string>

#include "ContextFeatureSet.h"

using namespace std;

string getMegamTestingInstance(vector<string>& context);

string getMegamTrainingInstance(vector<string>& context,int label);

#endif
