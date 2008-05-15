#include <cassert>
#include "Optimizer.h"
#include <vector>
#include<list>
#include <cfloat>
#include <iostream>

using namespace std;

static const float MINFLOAT=numeric_limits<float>::min();
static const float MAXFLOAT=numeric_limits<float>::max();

enum OptType{POWELL=0,NOPTIMIZER};//Add new optimizetr here

string names[NOPTIMIZER]={string("powell")};

Optimizer *BuildOptimizer(unsigned dim,vector<unsigned>to,vector<lambda>s,string type){
  int thetype;
  for(thetype=0;thetype<(int)NOPTIMIZER;thetype++)
    if(names[thetype]==type)
      break;
  switch((OptType)thetype){
  case POWELL:
    return new SimpleOptimizer(dim,to,s);
  case NOPTIMIZER:
    cerr<<"error unknwon optimizer"<<type<<endl;
    return NULL;
  }
  return NULL;//Should nver go there
};


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

Optimizer::Optimizer(unsigned Pd,vector<unsigned> i2O,vector<lambda> start):scorer(NULL),FData(NULL){
  //warning: the init vector is a full set of parameters, of dimension pdim!
  Point::pdim=Pd;
  assert(start.size()==Pd);
  Point::dim=i2O.size();
  Point::optindices=i2O;
  if(Point::pdim<Point::dim){
    for(int i=0;i<Point::pdim;i++){
      int j;
      for(j=0;j<Point::dim;j++)
	if(i==i2O[j])
	  break;
      if(j==Point::dim)//the index i wasnt found on optindices, it is a fixed index, we use the valu of hte start vector
	Point::fixedweights[i]=start[i];
    }
  }
};

Optimizer::~Optimizer(){
  delete scorer;
  delete FData;
}

statscore Optimizer::GetStatScore(const Point& param)const{
  vector<unsigned> bests;
  Get1bests(param,bests);
  return GetStatScore(bests);
};

/**compute the intersection of 2 lines*/
float intersect (float m1, float b1,float m2,float b2){
  if(m1==m2)
    return MAXFLOAT;//parrallel lines
  return((b2-b1)/(m1-m2));
}

statscore Optimizer::LineOptimize(const Point& origin,const Point& direction,Point& bestpoint)const{

  // we are looking for the best Point on the line y=Origin+x*direction
  float min_int=0.00001;
  typedef pair<float,vector<unsigned> > threshold;  
  list<threshold> thresholdlist;
  
  thresholdlist.push_back(pair<float,vector<unsigned> >(MINFLOAT,vector<unsigned>()));

  for(int S=0;S<size();S++){
    //first we determine the translation with the best feature score for each sentence and each value of x
    multimap<float,unsigned> gradient;
    vector<float> f0;
    for(unsigned j=0;j<FData->get(S).size();j++){
      gradient.insert(pair<float,unsigned>(direction*(FData->get(S,j)),j));//gradient of the feature function for this particular target sentence
      f0[j]=origin*FData->get(S,j);//compute the feature function at the origin point
    }
   //now lets compute the 1best for each value of x
    
    vector<pair<float,unsigned> > onebest;
    
    
    multimap<float,unsigned>::iterator it=gradient.begin();
    float smallest=it->first;//smallest gradient
    unsigned index=it->second;
    float biggestf0=f0[index];
    //several candidates can have the lowest slope (eg for word penalty where the gradient is an integer)
    it++;
    while(it!=gradient.end()&&it->first==smallest){
      if(f0[it->second]>f0[index])
	index=it->second;//the highest line is the one with he highest f0
      }
    --it;//we went one step too far in the while loop
    onebest.push_back(pair<float,unsigned>(MINFLOAT,index));//first 1best is the lowest gradient. 
    //now we look for the intersections points indicating a change of 1 best
    //we use the fact that the function is convex, which means that the gradient can only go up   
    while(it!=gradient.end()){
      map<float,unsigned>::iterator leftmost=it;
      float leftmostx=onebest.back().first;
      float m=it->first;
      float b=f0[it->second];
      multimap<float,unsigned>::iterator it2=it;
      it2++;
      for(;it2!=gradient.end();it2++){
	//look for all candidate with a gradient bigger than the current one and fond the one with the leftmost intersection
	float curintersect=intersect(m,b,it2->first,f0[it2->second]);
	if(curintersect<leftmostx){
	  //we have found and intersection to the left of the leftmost we had so far.
	  leftmostx=curintersect;
	  leftmost=it2;//this is the new reference
	}
      }
      //we have found the next intersection!

      /* Require that the intersection Point be at least min_int
	 to the right of the previous one. If not, we replace the
	 previous intersection Point with this one. Yes, it can even
	 happen that the new intersection Point is slightly to the
	 left of the old one, because of numerical imprecision. We
	 don't check that the new Point is also min_interval to the
	 right of the penultimate one. In that case, the Points would
	 switch places in the sort, resulting in a bogus score for
	 that interval. */
      if((leftmostx-onebest.back().first)<min_int)
	onebest.back()=pair<float,unsigned>(leftmostx,leftmost->second);//leftmost->first is the gradient, we are interested in the value of the intersection
      else //normal case: we add a new threshold
	onebest.push_back(pair<float,unsigned>(leftmostx,leftmost->second));
      it=leftmost;
    }
    //we have the onebest list and the threshold for the current sentence.
    //now we update the thresholdlist: we add the new threshold and the  value of the onebest.
    
    list<threshold >::iterator current=thresholdlist.begin();
    list<threshold >::iterator lit;
  
//add the 1best for x=-inf to the corresponding threshold
//    (this first threshold is the same for everybody)
    current->second.push_back(onebest[0].second);  
    assert(current->second.size()==S+1);
    unsigned prev1best=onebest[0].second;
    for(int t=1;t<onebest.size();t++){
      float ref=onebest[t].first;
      
      for( lit=current;lit!=thresholdlist.end()&&ref>lit->first;lit++){
	lit->second.push_back(prev1best);//whe update the threshold state with the 1best index for the current value
	assert(lit->second.size()==S+1);
      }
      if(lit!=thresholdlist.end()&&lit->first==ref){
	lit->second.push_back(onebest[t].second);
//this threshold was already created by a previous sentence (unlikely)
//We do not need to insert a new threshold in the list
	assert(lit->second.size()==S+1);
	current=lit;
      }else{
	//we have found where we must insert the threshold(before lit)
	current=lit;//we will continue after that point
	lit--;//We need to go back 1 to get the 1best vector
	//(ie we will use lit.second to initialize the new vector of 1best
	if(current!=thresholdlist.end())//insert just before current(just after lit)
	  thresholdlist.insert(current,pair<float,vector<unsigned> >(ref,lit->second));
	else //insert at the end of list
	  thresholdlist.push_back(pair<float,vector<unsigned> >(ref,lit->second));
	lit++;//now lit points on the threshold we just inserted
	lit->second.push_back(onebest[t].second);
	assert(lit->second.size()==S+1);
      }
      prev1best=onebest[t].second;
      assert(current==thresholdlist.end() || current->second.size()==S);//current has not been updated yet
    }//loop on onebest.size()
    //if the current last threshold in onebest is not the last in thresholdlist,
    //we need to update the 1bestvector above this last threshold.

    for(lit=current;lit!=thresholdlist.end();lit++){
      lit->second.push_back(onebest.front().second);
      assert(lit->second.size()==S+1);
    }
  }//loop on S
  //now the thresholdlist is up to date: it contains a list of all the lambdas where the function changed its value, along with the nbest list for the interval after each threshold
  //last thing to do is compute the Stat score (ie BLEU) and find the minimum
  list<threshold>::iterator best;
  list<threshold>::iterator lit2;
  statscore bestscore=MINFLOAT;
  for(lit2=thresholdlist.begin();lit2!=thresholdlist.end();lit2){
    assert(lit2->second.size()==FData->size());
    statscore cur=GetStatScore(lit2->second);
    if(cur>bestscore){
      bestscore=cur;
      best=lit2;
    }
  }
    
    //finally! we manage to extract the best score and convert it to a point!
    float bestx=best->first+min_int/2;//we dont want to stay exactly at the threshold where the function is discontinuous so we move just a little to the right
    bestpoint=direction*bestx+origin;
    bestpoint.score=bestscore;
    return bestscore;  
};

void  Optimizer::Get1bests(const Point& P,vector<unsigned>& bests)const{
  assert(FData);
  bests.clear();
  bests.resize(size());
  
  for(unsigned i=0;i<size();i++){
    float bestfs=MINFLOAT;
    unsigned idx=0;
    unsigned j;
    for(j=0;j<FData->get(i).size();j++){
      float curfs=P*FData->get(i,j);
      if(curfs>bestfs){
	bestfs=curfs;
	idx=j;
      }
    }
    bests[i]=idx;
  }
  
}

statscore Optimizer::Run(Point& P)const{
  if(!FData){
    cerr<<"error trying to optimize without Feature loaded"<<endl;
    exit(2);
  }
  if(!scorer){
    cerr<<"error trying to optimize without a Scorer loaded"<<endl;
    exit(2);
  }
  statscore s=TrueRun(P);
  P.score=s;//just in case its not done in TrueRun
  return s;
}
statscore SimpleOptimizer::TrueRun(Point& P)const{
 
  statscore prevscore=MAXFLOAT;
  statscore bestscore=MAXFLOAT;
  do{
    Point  best;
    Point  linebest;
    for(int d=0;d<Point::getdim();d++){
      Point direction;
      direction[d]=1.0;
      statscore curscore=LineOptimize(P,direction,linebest);//find the minimum on the line
      if(curscore>bestscore){
	bestscore=curscore;
	best=linebest;
      }
    }
    P=best;//update the current vector with the best points on all line tested
}while(bestscore-prevscore<eps);
  return bestscore;
}




