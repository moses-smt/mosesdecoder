#include <iostream>
#include "StaticData.h"
#include "Manager.h"
#include "Phrase.h"
#include "moses/InputFileStream.h"

using namespace std;

int main(int argc, char** argv)
{
	cerr << "Starting..." << endl;

	StaticData staticData;

	string line;
	while (getline(cin, line)) {

		Manager mgr(staticData, line);
		mgr.Decode();
	}

	cerr << "Finished" << endl;
}
