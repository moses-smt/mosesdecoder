#include <iostream>
#include "StaticData.h"
#include "Manager.h"

using namespace std;

int main()
{
	cerr << "Starting..." << endl;

	StaticData staticData;
	Manager mgr(staticData);

	cerr << "Finished" << endl;
}
