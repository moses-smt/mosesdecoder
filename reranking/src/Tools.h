/*
 *  nbest: tool to process moses n-best lists
 *
 *  File: Tools.cpp
 *        basic utility functions
 *
 *  Created by Holger Schwenk, University of Le Mans, 05/16/2008
 *
 */


#ifndef _TOOLS_H_
#define _TOOLS_H_

using namespace std;

#include <iostream>
#include <fstream>
#include <vector>

class Weights
{
  vector<float> val;
public:
  Weights() {};
  ~Weights() {};
  int Read(const char *);
  friend class Hypo;
};

//******************************************************

/*
template<typename T>
inline T Scan(const std::string &input)
{
         std::stringstream stream(input);
         T ret;
         stream >> ret;
         return ret;
}
*/

//******************************************************

inline void Error (char *msg)
{
  cerr << "ERROR: " << msg << endl;
  exit(1);
}

//******************************************************
// From Moses code:


/*
 * Outputting debugging/verbose information to stderr.
 * Use TRACE_ENABLE flag to redirect tracing output into oblivion
 * so that you can output your own ad-hoc debugging info.
 * However, if you use stderr diretly, please delete calls to it once
 * you finished debugging so that it won't clutter up.
 * Also use TRACE_ENABLE to turn off output of any debugging info
 * when compiling for a gui front-end so that running gui won't generate
 * output on command line
 * */
#ifdef TRACE_ENABLE
#define TRACE_ERR(str) std::cerr << str
#else
#define TRACE_ERR(str) {}
#endif

#endif

