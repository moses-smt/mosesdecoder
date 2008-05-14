#include <iostream>
#include <vector>

#include "ScoreData.h"
#include "BleuScorer.h"

using namespace std;

int main(int argc, char** argv) {
	cout << "Testing the scorer" << endl;	
	//BleuScorer bs("test-scorer-data/cppstats.feats.opt");;
	vector<string> references;
	references.push_back("test_scorer_data/reference.txt");
	//bs.prepare(references, "test-scorer-data/nbest.out");
	Scorer scorer("test");
	ScoreData sd(scorer);
	sd.loadnbest("test_scorer_data/nbest.out");
}
