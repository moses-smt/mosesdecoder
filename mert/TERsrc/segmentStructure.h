#ifndef __SEGMENTSTRUCTURE_H__
#define __SEGMENTSTRUCTURE_H__


#include <vector>
#include <stdio.h>
#include <string>
#include <sstream>
#include "tools.h"
#include "tercalc.h"


using namespace std;
using namespace Tools;

namespace TERCpp
{
    class segmentStructure
    {
        private:
            string segId;
            vecString content;
            terAlignment evaluation;
            string bestDocId;
    	    float averageLength;

        public:
            segmentStructure();
            segmentStructure ( string id, vecString vecS );
            segmentStructure ( string id, string txt );
	    void setAverageLength(float f);
	    float getAverageLength();
            string getSegId();
            terAlignment getAlignment();
            void setAlignment(terAlignment& l_align);
            void setSegId ( string s );
            void setBestDocId ( string s );
            string getBestDocId();
            void addContent ( vecString vecS );
            void addContent ( string s );
	    int getSize();
// 	  {
// 	    return segId;
// 	  }
            vecString getContent();
// 	  {
// 	    return content;
// 	  }
// 	alignmentStruct();
// 	alignmentStruct (int _start, int _end, int _moveto, int _newloc);
// 	alignmentStruct (int _start, int _end, int _moveto, int _newloc, vector<string> _shifted);
// 	string toString();
// 	int distance() ;
// 	bool leftShift();
// 	int size();
// 	alignmentStruct operator=(alignmentStruct t);
// 	string vectorToString(vector<string> vec);

//   int start;
//   int end;
//   int moveto;
//   int newloc;
            vector<string> nwords; // The words we shifted
            vector<char> alignment ; // for pra_more output
            vector<vecInt> aftershift; // for pra_more output
            // This is used to store the cost of a shift, so we don't have to
            // calculate it multiple times.
            double cost;
            string toString();
    };

}
#endif
