#ifndef __TERCPPTOOLS_H__
#define __TERCPPTOOLS_H__


#include <vector>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <boost/xpressive/xpressive.hpp>


using namespace std;

namespace Tools
{
    typedef vector<double> vecDouble;
    typedef vector<char> vecChar;
    typedef vector<int> vecInt;
    typedef vector<float> vecFloat;
    typedef vector<string> vecString;
    typedef vector<string> alignmentElement;
    typedef vector<alignmentElement> WERalignment;


// class tools{
// private:
// public:

    string vectorToString ( vector<string> vec );
    string vectorToString ( vector<string> vec, string s );
    vector<string> subVector ( vector<string> vec, int start, int end );
    vector<string> copyVector ( vector<string> vec );
    vector<string> stringToVector ( string s, string tok );
    vector<int> stringToVectorInt ( string s, string tok );
    string lowerCase(string str);
    string removePunct(string str);
    string tokenizePunct(string str);
// };

}
#endif