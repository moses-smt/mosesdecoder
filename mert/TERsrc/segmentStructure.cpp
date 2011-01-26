#include "segmentStructure.h"

using namespace std;
namespace TERCpp
{
    vecString segmentStructure::getContent()
    {
        return content;
    }
    string segmentStructure::getSegId()
    {
        return segId;
    }
    string segmentStructure::toString()
    {
//         return vectorToString(content);
        return "";
    }
    void segmentStructure::addContent ( vecString vecS )
    {
        content = vecS;
    }
    void segmentStructure::setSegId ( string s )
    {
        segId = s;
    }
    segmentStructure::segmentStructure ( string id, vecString vecS )
    {
        segId = id;
        content = vecS;
    }
    segmentStructure::segmentStructure ( string id, string txt )
    {
        segId = id;
        content = stringToVector ( txt, " " );
    }
    void segmentStructure::addContent ( string s )
    {
        content = stringToVector ( s, " " );
    }
    segmentStructure::segmentStructure()
    {
        segId = "";
    }
    terAlignment segmentStructure::getAlignment()
    {
        return evaluation;
    }
    void segmentStructure::setAlignment ( terAlignment& l_align )
    {
	evaluation = l_align;
    }

    string segmentStructure::getBestDocId()
    {
        return bestDocId;
    }
    void segmentStructure::setBestDocId ( string s )
    {
        bestDocId = s;
    }



}
