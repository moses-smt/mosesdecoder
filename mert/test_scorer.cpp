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
	BleuScorer scorer;
	scorer.setReferenceFiles(references);
	ScoreData sd(scorer);
	sd.loadnbest("test_scorer_data/nbest.out");
	//sd.savetxt();

	//calculate a  bleu scores
	scorer.setScoreData(&sd);
	unsigned int index = 0;
	vector<unsigned int> candidates;
	for (size_t i  = 0; i < sd.size(); ++i) {
        sd.get(i,index).savetxt("/dev/stdout");
		candidates.push_back(index++);
		if (index == 10) {
			index = 0;
		}
	}

	cout << "Bleu ";
	float bleu = scorer.score(candidates);
	cout << bleu << endl;
}
