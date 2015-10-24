#include <iostream>
#include "StaticData.h"
#include "Manager.h"
#include "Phrase.h"
#include "moses/InputFileStream.h"

using namespace std;

int main()
{
	cerr << "Starting..." << endl;

	StaticData staticData;

	string line;
	while (getline(cin, line)) {

		Manager mgr(staticData, line);

	}

	cerr << "Finished" << endl;
}
