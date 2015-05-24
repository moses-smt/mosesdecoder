#include <iostream>
#include <string>
#include <iomanip>
#include "ug_http_client.h"

using namespace std;
int main()
{
  string line;
  while (getline(cin,line))
    cout << Moses::uri_encode(line) << endl;
}

