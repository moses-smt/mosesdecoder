#include <cassert>
#include "Optimizer.h"
#include <vector>
#include <cfloat>
#include <iostream>

using namespace std;

static const float MINFLOAT=numeric_limits<float>::min();
static const float MAXFLOAT=numeric_limits<float>::max();

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

float intersect (float b1, float m1,float b2,float m2){
  if(m1==m2)
    return MAXFLOAT;
  return((b2-b1)/(m1-m2));
}

statscore Optimizer::LineOptimize(const Point& start,Point direction,Point& best){
  
  direction.normalize();//we pass by value so changing is ok
  // we are looking for the best Point on the line y=start+x*direction
  //vector< vector<float,unsigned> > onebest;
  float min_int=0.00001;
  typedef pair<float,vector<unsigned> > threshold;  
  vector<threshold> thresholdlist;
  
  thresholdlist.push_back(pair<float,vector<unsigned> >(MINFLOAT,vector<unsigned>()));

  for(int i=0;i<N;i++){
    //first we determine the translation with the best feature score for each sentence and each value of x
    multimap<float,unsigned> gradient;
    vector<float> f0;
    for(unsigned j=0;j<FData[i].size();j++){
      gradient.insert(pair<float,unsigned>(direction*(FData->get(i,j)),j));
      f0[j]=start*FData->get(i).get(j);
    }
   //now lets compute the 1best for each value of x
    
    
    vector<pair<float,unsigned> > onebest;
    onebest.push_back(pair<float,unsigned>(MINFLOAT,gradient.begin()->second));//first 1best is the lowest gradient.

    for(multimap<float,unsigned>::iterator it=gradient.begin();it!=gradient.end();){
      map<float,unsigned>::iterator leftmost=it;
      float m=it->first;
      float b=f0[it->second];
      float leftmostx=onebest.back().first;
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

      if((leftmostx-onebest.back().first)<min_int)
	onebest.back()=pair<float,unsigned>(leftmostx,leftmost->second);//leftmost->first is theg gradient, we are interested in the value of the intersection
      else
	onebest.push_back(pair<float,unsigned>(leftmostx,leftmost->second));
      it=leftmost;
    }
    //we have the onebest list and the threshold for the current sentence.
    //now we update the thresholdlist: we add the new threshold and the  value of the onebest.
    thresholdlist[0].second.push_back(onebest[0].second);//add the 1best for x=-inf to the corresponding threshold
    unsigned curthres=1;
    for(int t=1;t<onebest.size();){
      for(int gt=curthres;gt<thresholdlist.size()&&onebest[t].first>thresholdlist[curthres].first;gt++){
}
    }
  }
   
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
