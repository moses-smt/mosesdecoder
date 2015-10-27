/*
 * Weights.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#include <cassert>
#include <string>
#include <vector>
#include "FeatureFunctions.h"
#include "Weights.h"
#include "moses/Util.h"

using namespace std;

Weights::Weights() {
	// TODO Auto-generated constructor stub

}

Weights::~Weights() {
	// TODO Auto-generated destructor stub
}

std::ostream& operator<<(std::ostream &out, const Weights &obj)
{

	return out;
}

void Weights::CreateFromString(const FeatureFunctions &ffs, const std::string &line)
{
	std::vector<std::string> toks = Moses::Tokenize(line);
	assert(toks.size());

	string ffName = toks[0];
	assert(ffName.back() == '=');

	ffName = ffName.substr(0, ffName.size() - 1);
	cerr << "ffName=" << ffName << endl;


}
