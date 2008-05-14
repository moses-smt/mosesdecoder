#include <cassert>
#include "Optimizer.h"
#include <vector>
#include <cfloat>
#include <iostream>

using namespace std;

void Optimizer::SetScorer(Scorer *S){
  if(scorer)
    delete scorer;
  scorer=S;
}

void Optimizer::SetFData(FeatureData *F){
  if(FData)
    delete FData;
  FData=F;
};

float intersect (float b1,m1,b2,m2){
  if(m1==m2)
    return numeric_limit<float>::max();
  return((b2-b1)/(m2-m1));
}

statscore Optimizer::LineOptimize(const Point& start,Point direction,Point& best){
  direction.normalize();//we pass by value so changing is ok
  // we are looking for the best Point on the line y=start+x*direction
  vector< vector<float,unsigned> > onebest;
  float min_int=0.00001;
  multimap<float,unsigned> thresholdlist;
  for(int i=0;i<N;i++){
    //first we determine the translation with the best feature score for each sentence and each value of x
    multimap<float,unsigned> gradient;
    vector<float> f0;
    for(unsigned j=0;j<FData[i].size();j++){
      gradient.insert(pair<float,unsigned>(direction*FData->get(i).get(j),j));
      f0[j]=start*FData->get(i).get(j);
    }
   //now lets compute the 1best for each value of x
    
    unsigned lastindex=gradient.rbegin()->second;

    onebest[i].push_back(pair<float,unsigned>(numeric_limit<float>::min(),gradient.begin()->second));//first 1best is the lowest gradient.
    for(multimap<float,unsigned>::iterator it=gradient.begin();it!=gradient.end();){
      map<float,unsigned>::iterator leftmost=it;
      float m=it->first;
      float b=f0[it->second];
      leftmostx=onebest[i].rbegin()->first;
      for(multimap<float,unsigned>::iterator it2=it;it2!=gradient.end();it2++){
	float curintersect=intersect(m,b,it2->first,f0[it2->second]);
	if(curintersect<leftmostx){
	  //we have found and intersection to the left of the best one.
	  leftmostx=curintersect;
	  leftmost=it2;//this is the new reference
	}
      }
      /* Require that the intersection Point be at least min_int
	 to the right of the previous one. If not, we replace the
	 previous intersection Point with this one. Yes, it can even
	 happen that the new intersection Point is slightly to the
	 left of the old one, because of numerical imprecision. We
	 don't check that the new Point is also min_interval to the
	 right of the penultimate one. In that case, the Points would
	 switch places in the sort, resulting in a bogus score for
	 that inteval. */

      if((leftmostx-onebest[i].rbegin()->first)<min_int)
	onebest[i][onebest[i].size()-1]=pair<float,unsigned>(leftmostx,it2->second);
      else
	onebest[i].push_back(pair<float,unsigned>(leftmostx,it2->second));
      it=it2;
    }
  }
//now we have a list of threshold and corresponding onebest. for each sentence.
//now we will compute the stat score for each part of the line and extract the best (with respect to stat score)
 float curthreshold=numeric_limit<float>::min();
 

 for(k=0;k<N;k++)
   cur1best[k]=onebest[k]->second;
 statscore best=GetStatScore(cur1best); 
do{
  float smallestthreshold=curthreshold;
  unsigned changeindex=N;
  for(k=0;k<N;k++)
    
 }while

   
};


Point SimpleOptimizer::run(const Point& init){
  assert(dimension==init.size());
  Point cur=init;
  statscore prevscore=FLT_MAX;
  statscore bestscore=FLT_MAX;
  do{
    Point  best(dimension);
    Point  linebest(dimension);
    for(int d=0;d<dimension;d++){
      Point direction(dimension);
      direction[d]=1.0;
      statscore curscore=LineOptimize(cur,direction,linebest);//find the minimum on the line
      if(curscore<bestscore){
	bestscore=curscore;
	best=linebest;
      }
    }
    cur=best;//update the current vector with the best result
}while(bestscore-prevscore<eps);
  return cur;
}
