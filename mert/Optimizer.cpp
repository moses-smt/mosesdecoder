#include <cassert>
#include <vector>
#include <limits>
#include <list>
#include <cfloat>
#include <iostream>

#include "Optimizer.h"

using namespace std;

static const float MIN_FLOAT=-1.0*numeric_limits<float>::max();
static const float MAX_FLOAT=numeric_limits<float>::max();



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

Optimizer::Optimizer(unsigned Pd,vector<unsigned> i2O,vector<parameter_t> start):scorer(NULL),FData(NULL){
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

statscore_t Optimizer::GetStatScore(const Point& param)const{
  vector<unsigned> bests;
  Get1bests(param,bests);
  return GetStatScore(bests);
};

/**compute the intersection of 2 lines*/
float intersect (float m1, float b1,float m2,float b2){
  if(m1==m2)
    return MAX_FLOAT;//parrallel lines
  return((b2-b1)/(m1-m2));
}

statscore_t Optimizer::LineOptimize(const Point& origin,const Point& direction,Point& bestpoint)const{

  // we are looking for the best Point on the line y=Origin+x*direction
  float min_int=0.0001;
  typedef pair<unsigned,unsigned> diff;//first the sentence that changes, second is the new 1best for this sentence
  typedef pair<float,vector<diff> > threshold;  
  list<threshold> thresholdlist;
  thresholdlist.push_back(threshold(MIN_FLOAT,vector<diff>()));
  vector<unsigned> first1best;//the vector of nbrests for x=-inf
  for(int S=0;S<size();S++){
    //first we determine the translation with the best feature score for each sentence and each value of x
    multimap<float,unsigned> gradient;
    vector<float> f0;
    f0.resize(FData->get(S).size());
    for(unsigned j=0;j<FData->get(S).size();j++){
      gradient.insert(pair<float,unsigned>(direction*(FData->get(S,j)),j));//gradient of the feature function for this particular target sentence
      f0[j]=origin*FData->get(S,j);//compute the feature function at the origin point
    }
    //now lets compute the 1best for each value of x
    
    vector<pair<float,unsigned> > onebest;
    
    
    multimap<float,unsigned>::iterator it=gradient.begin();
    float smallest=it->first;//smallest gradient
    unsigned index=it->second;
    //several candidates can have the lowest slope (eg for word penalty where the gradient is an integer )
    it++;
    while(it!=gradient.end()&&it->first==smallest){
      //   cerr<<"ni"<<it->second<<endl;;
      //cerr<<"fos"<<f0[it->second]<<" "<<f0[index]<<" "<<index<<endl;
      if(f0[it->second]>f0[index])
	index=it->second;//the highest line is the one with he highest f0
      it++;
    }
    --it;//we went one step too far in the while loop
    onebest.push_back(pair<float,unsigned>(MIN_FLOAT,index));//first 1best is the lowest gradient. 
    //now we look for the intersections points indicating a change of 1 best
    //we use the fact that the function is convex, which means that the gradient can only go up   
    int c=0;
    while(it!=gradient.end()){
      map<float,unsigned>::iterator leftmost=it;
      float leftmostx=onebest.back().first;
      float m=it->first;
      float b=f0[it->second];
      multimap<float,unsigned>::iterator it2=it;
      it2++;
      int d=0;
      for(;it2!=gradient.end();it2++){
	//cerr<<"--"<<d++<<' '<<it2->first<<' '<<it2->second<<endl;
	//look for all candidate with a gradient bigger than the current one and find the one with the leftmost intersection
	float curintersect;
	if(m!=it2->first){
	  curintersect=intersect(m,b,it2->first,f0[it2->second]);
	  if(curintersect<leftmostx){
	    //we have found and intersection to the left of the leftmost we had so far.
	  leftmostx=curintersect;
	  leftmost=it2;//this is the new reference
	  }
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
      if(abs(leftmostx-onebest.back().first)<min_int)
	onebest.back()=pair<float,unsigned>(leftmostx,leftmost->second);//leftmost->first is the gradient, we are interested in the value of the intersection
      else //normal case: we add a new threshold
	onebest.push_back(pair<float,unsigned>(leftmostx,leftmost->second));
      if(it==leftmost)
	it=gradient.end();
      else
	it=leftmost;
    }
    //we have the onebest list and the threshold for the current sentence.
    //now we update the thresholdlist: we add the new threshold and the  value of the onebest.

    //add the 1best for x=-inf to the corresponding threshold
    //    (this first threshold is the same for everybody)
    first1best.push_back(onebest[0].second);  
    assert(first1best.size()==S+1);

    list<threshold >::iterator current=thresholdlist.begin();
    list<threshold >::iterator lit;
  
    
    unsigned prev1best=onebest[0].second;
    for(int t=1;t<onebest.size();t++){
      float ref=onebest[t].first;
      for( lit=current;lit!=thresholdlist.end()&&ref>lit->first;lit++)
	;
      //we have found where we must insert the new threshold(before lit)
      if(lit==thresholdlist.end()){
	thresholdlist.push_back(threshold(ref,vector<diff>()));
	lit--;
      }
      else 
	if(ref!=lit->first)//normal case
	  lit=thresholdlist.insert(lit,threshold(ref,vector<diff>()));
      //if ref==lit->first:unlikely (but this is the reason why we have a vector of diff); in that case the threshold is already created
      //lit is at the right place now we add the diff pair
      lit->second.push_back(diff(S,onebest[t].second));
      current=lit;//we will continue after that point
      current++;
      prev1best=onebest[t].second;
    }//loop on onebest.size()
  }//loop on S
  //now the thresholdlist is up to date: 
  //it contains a list of all the parameter_ts where the function changed its value, along with the nbest list for the interval after each threshold
  //last thing to do is compute the Stat score (ie BLEU) and find the minimum
  
  //  cerr<<"thesholdlist size"<<thresholdlist.size()<<endl;  
  list<threshold>::iterator lit2=thresholdlist.begin();
  ++lit2;
  
  vector<vector<diff> > diffs;
  for(;lit2!=thresholdlist.end();lit2++)
    diffs.push_back(lit2->second);
  vector<statscore_t> scores=GetIncStatScore(first1best,diffs);
  
  lit2=thresholdlist.begin();
  statscore_t bestscore=MIN_FLOAT;
  float bestx=MIN_FLOAT;
  assert(scores.size()==thresholdlist.size());//we skipped the first el of thresholdlist but GetIncStatScore return 1 more for first1best
  for(int sc=0;sc!=scores.size();sc++){
    lit2++;//We move the list iterator and the vector index at the same time
    //because we need to get the value of lambda back from the list
    //cerr<<lit2->first<<endl;
    if(scores[sc]>bestscore){
      bestscore=scores[sc];
      if(lit2!=thresholdlist.end()){
	//we dont want to stay exactly at the threshold where the function is discontinuous so we move just a little to the right
	//but  we dont want to cross a threshold 
	bestx=lit2->first;
	lit2++;
	if(lit2->first-bestx>0.0001)//distance to next threshold
	  bestx+=0.0001;
	lit2--;
      }else
	bestx=lit2->first+0.0001;
    }
  }
  if(abs(bestx)<0.00015){
    bestx=0.0;//the origin of the line is the best point!we put it back at 0 so we do not propagate rounding erros
  //finally! we manage to extract the best score;
  //now we convert bestx  (position on the line) to a point!
    if(verboselevel()>4)
      cerr<<"best point on line at origin"<<endl;
  }
  if(verboselevel()>3)
    cerr<<"end Lineopt, bestx="<<bestx<<endl;
    bestpoint=direction*bestx+origin;
  bestpoint.score=bestscore;
  return bestscore;  
};

void  Optimizer::Get1bests(const Point& P,vector<unsigned>& bests)const{
  assert(FData);
  bests.clear();
  bests.resize(size());
  
  for(unsigned i=0;i<size();i++){
    float bestfs=MIN_FLOAT;
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

statscore_t Optimizer::Run(Point& P)const{
  if(!FData){
    cerr<<"error trying to optimize without Features loaded"<<endl;
    exit(2);
  }
  if(!scorer){
    cerr<<"error trying to optimize without a Scorer loaded"<<endl;
    exit(2);
  }
  if (scorer->getReferenceSize()!=FData->size()){
    cerr<<"errror size mismatch between FeatureData and Scorer"<<endl;
    exit(2);
  }
  if(verboselevel()>1)
    cerr<<"starting Point "<<P;
  statscore_t s=TrueRun(P);
  P.score=s;//just in case its not done in TrueRun
  if (verboselevel()>1)
    cerr<<"best found Point "<<P<<"score="<<s<<endl;
  return s;
}
 

vector<statscore_t> Optimizer::GetIncStatScore(vector<unsigned> thefirst,vector<vector <pair<unsigned,unsigned> > > thediffs)const{
  assert(scorer);

  vector<statscore_t> theres;

  scorer->score(thefirst,thediffs,theres);
  return theres;
};




//---------------- code for the powell optimizer
float SimpleOptimizer::eps=0.0001;
statscore_t SimpleOptimizer::TrueRun(Point& P)const{
 
  statscore_t prevscore=0;
  statscore_t bestscore=MIN_FLOAT;
  int nrun=0;
  do{
    ++nrun;    
    if(verboselevel()>2&&nrun>1)
      cerr<<"last diff="<<bestscore-prevscore<<"nrun "<<nrun<<endl;
    prevscore=bestscore;
    
    Point  best;
    Point  linebest;
    
    for(int d=0;d<Point::getdim();d++){
      if(verboselevel()>4)
	cerr<<"minimizing along direction "<<d<<endl;
      Point direction;
      for(int i=0;i<Point::getdim();i++)
	direction[i];
      direction[d]=1.0;
      statscore_t curscore=LineOptimize(P,direction,linebest);//find the minimum on the line
      if(verboselevel()>5){
	cerr<<"direction: "<<d<<" score="<<curscore<<endl;
	cerr<<"\tPoint: "<<linebest<<endl;
      }
      if(curscore>bestscore){
	bestscore=curscore;
	best=linebest;	
	if(verboselevel()>3){
	cerr<<"new best dir:"<<d<<" ("<<nrun<<")"<<endl;
	cerr<<"new best Point "<<best<<endl;
	  }
      }
    }
    P=best;//update the current vector with the best point on all line tested
    if(verboselevel()>3)
      cerr<<nrun<<"\t"<<P<<endl;
}while(bestscore-prevscore>eps);

  if(verboselevel()>2){
    cerr<<"end Powell Algo, nrun="<<nrun<<endl;
    cerr<<"last diff="<<bestscore-prevscore<<endl;
    cerr<<"\t"<<P<<endl;
}
  return bestscore;
}


/**RandomOptimizer to use as beaseline and test.\n
Just return a random point*/

statscore_t RandomOptimizer::TrueRun(Point& P)const{
  vector<parameter_t> min(Point::getdim());
  vector<parameter_t> max(Point::getdim());
  for(int d=0;d<Point::getdim();d++){
    min[d]=0.0;
    max[d]=1.0;
  }
    P.Randomize(min,max);
    statscore_t score=GetStatScore(P);
    P.score=score;
    return score;
}
//--------------------------------------
vector<string> OptimizerFactory::typenames;

void OptimizerFactory::SetTypeNames(){
  if(typenames.empty()){
    typenames.resize(NOPTIMIZER);
    typenames[POWELL]="powell";
    typenames[RANDOM]="random";
    //add new type there
      }
}
vector<string> OptimizerFactory::GetTypeNames(){
  if(typenames.empty())
    SetTypeNames();
  return typenames;
}

OptimizerFactory::OptType OptimizerFactory::GetOType(string type){
  int thetype;
  if(typenames.empty())
    SetTypeNames();
  for(thetype=0;thetype<typenames.size();thetype++)
    if(typenames[thetype]==type)
      break;
  return((OptType)thetype);
};

Optimizer* OptimizerFactory::BuildOptimizer(unsigned dim,vector<unsigned> i2o,vector<parameter_t> start,string type){

  OptType T=GetOType(type);
  if(T==NOPTIMIZER){
    cerr<<"Error unknow Optimizer type "<<type<<endl;
    cerr<<"Known Algorithm are:"<<endl;
    int thetype;
    for(thetype=0;thetype<typenames.size();thetype++)
      cerr<<typenames[thetype]<<endl;
    throw ("unknown Optimizer Type");
  }
  
  switch((OptType)T){
  case POWELL:
    return new SimpleOptimizer(dim,i2o,start);
    break;
  case NOPTIMIZER:
    cerr<<"error unknwon optimizer"<<type<<endl;
    return NULL; 
  }  
}
