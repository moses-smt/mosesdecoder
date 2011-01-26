#ifndef __DOCUMENTSTRUCTURE_H__
#define __DOCUMENTSTRUCTURE_H__


#include <vector>
#include <stdio.h>
#include <string>
#include <sstream>
#include "tools.h"
#include "segmentStructure.h"


using namespace std;
using namespace Tools;

namespace TERCpp
{
    class documentStructure
    {
        private:
            string docId;
            string sysId;
            vector<segmentStructure> seg;
        public:
            string getDocId();
            string getSysId();
            vector<segmentStructure>* getSegments();
            segmentStructure* getLastSegments();
            void setDocId ( string s );
            void setSysId ( string s );
            void addSegments ( segmentStructure s );
            void addSegments ( string id, string text );
            segmentStructure* getSegment ( string id );
            int getSize();

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
//             vector<string> nwords; // The words we shifted
//             vector<char> alignment ; // for pra_more output
//             vector<vecInt> aftershift; // for pra_more output
            // This is used to store the cost of a shift, so we don't have to
            // calculate it multiple times.
//             double cost;
            string toString();
    };

}
#endif
