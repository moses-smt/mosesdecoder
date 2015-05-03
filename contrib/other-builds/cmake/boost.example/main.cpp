
#include <iostream>
#include <boost/locale.hpp>
#include <ctime>

using namespace std;

int main(int argc, char* argv[])
{

  using namespace boost::locale;
  using namespace std;

  generator gen;
  locale loc=gen("");

  cout.imbue(loc);

  cout << "Hello, World" << endl;

  cout << "This is how we show currency in this locale " << as::currency << 103.34 << endl;

  return 0;
}
