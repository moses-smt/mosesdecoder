#include <iostream>
#include "StaticData.h"
#include "Manager.h"
#include "Phrase.h"

using namespace std;

int main()
{
	cerr << "Starting..." << endl;

	StaticData staticData;

	string line;
	while (getline(cin, line)) {
		Phrase *input = Phrase::CreateFromString(line);

		Manager mgr(staticData, *input);

		delete input;
	}

	cerr << "Finished" << endl;
}
