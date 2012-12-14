#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include "moses/Parameter.h"

using namespace std;
using namespace Moses;

int main(int argc, char** argv)
{
  try {

    // load all the settings into the Parameter class
    // (stores them as strings, or array of strings)
    Parameter* params = new Parameter();
    if (!params->LoadParam(argc,argv)) {
      exit(1);
    }

		const PARAM_MAP &setting = params->GetParams();

		PARAM_MAP::const_iterator iterOuter;
		for (iterOuter = setting.begin(); iterOuter != setting.end(); ++iterOuter) {
			const PARAM_VEC &vec = iterOuter->second;
			string key = iterOuter->first;

			if (key != "weight") {
				cout << "[" << key << "]" << endl;
				PARAM_VEC::const_iterator iterInner;
				for (iterInner = vec.begin(); iterInner != vec.end(); ++iterInner) {
					string value = *iterInner;
					cout << value << endl;
				}	
			}
		}

  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

}

