/*
 *  Util.cpp
 *  met - Minimum Error Training
 * 
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include "Util.h"

int verbose=0;

int verboselevel(){
  return verbose;
}

int setverboselevel(int v){
  verbose=v;
  return verbose;
}

int getNextPound(std::string &theString, std::string &substring, const std::string delimiter)
{
        int pos = 0;
        
        //skip all occurrences of delimiter
        while ( pos == 0 )
        {
                if ((pos = theString.find(delimiter)) != std::string::npos){
                        substring.assign(theString, 0, pos);
                        theString.assign(theString, pos + delimiter.size(), theString.size());
                }
                else{
                        substring.assign(theString);
                        theString.assign("");
                }
        }
        return (pos);
};
