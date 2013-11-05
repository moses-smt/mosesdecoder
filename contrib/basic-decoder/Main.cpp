
#include <iostream>
#include <string>
#include "Sentence.h"
#include "Global.h"
#include "Util.h"
#include "Search/Manager.h"

using namespace std;

void temp();

int main(int argc, char** argv)
{
  //temp();
  
  Fix(cerr, 3);

  Global &global = Global::InstanceNonConst();
  global.timer.start("Starting...");

  global.Init(argc, argv);

  global.timer.check("Ready for input:");

  string line;
  while (getline(global.GetInputStream(), line)) {
    if (line == "EXIT") {
      break;
    }

    Sentence *input = Sentence::CreateFromString(line);
    cerr << "input=" << input->Debug() << endl;

    Manager manager(*input);

    const Hypothesis *hypo = manager.GetHypothesis();
    if (hypo) {
      cerr << "TRANSLATION FOUND" << hypo->Debug() << endl;
      hypo->Output(cout);
    } else {
      cerr << "NO BEST TRANSLATION" << endl;
    }
    cout << endl;

    cerr << "Ready for input:" << endl;
  }

  cerr << "Shutting down" << endl;

  cerr << "hypotheses created=" << Hypothesis::GetNumHypothesesCreated() << endl;

  global.timer.check("Finished");
}



