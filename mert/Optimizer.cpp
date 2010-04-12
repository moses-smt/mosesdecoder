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
  if (Point::pdim>Point::dim){
    for (unsigned int i=0;i<Point::pdim;i++){
      unsigned int j = 0;
      while (j<Point::dim && i!=i2O[j])
	j++;

      if (j==Point::dim)//the index i wasnt found on optindices, it is a fixed index, we use the value of the start vector
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
  //copy(bests.begin(),bests.end(),ostream_iterator<unsigned>(cerr," "));
  statscore_t score = GetStatScore(bests);
  return score;
};

/**compute the intersection of 2 lines*/
float intersect (float m1, float b1,float m2,float b2){
  float isect = ((b2-b1)/(m1-m2));
  if (!isfinite(isect)) {
      isect = MAX_FLOAT;
  }
  return isect;
}

map<float,diff_t >::iterator AddThreshold(map<float,diff_t >& thresholdmap,float newt,pair<unsigned,unsigned> newdiff){
  map<float,diff_t>::iterator it=thresholdmap.find(newt);
  if(it!=thresholdmap.end()){
    //the threshold already exists!! this is very unlikely
    if(it->second.back().first==newdiff.first)
      it->second.back().second=newdiff.second;//there was already a diff for this sentence, we change the 1 best;
    else
      it->second.push_back(newdiff);
  }else{
    //normal case
    pair< map<float,diff_t >::iterator,bool > ins=thresholdmap.insert(threshold(newt,diff_t(1,newdiff)));
    assert(ins.second);//we really inserted something
    it=ins.first;
  }
  return it;
};


statscore_t Optimizer::LineOptimize(const Point& origin,const Point& direction,Point& bestpoint)const{

// we are looking for the best Point on the line y=Origin+x*direction
  float min_int=0.0001;
  //typedef pair<unsigned,unsigned> diff;//first the sentence that changes, second is the new 1best for this sentence
  //list<threshold> thresholdlist;
	
  map<float,diff_t> thresholdmap;
  thresholdmap[MIN_FLOAT]=diff_t();
  vector<unsigned> first1best;//the vector of nbests for x=-inf
  for(unsigned int S=0;S<size();S++){
    map<float,diff_t >::iterator previnserted=thresholdmap.begin();
    //first we determine the translation with the best feature score for each sentence and each value of x
    //cerr << "Sentence " << S << endl;
    multimap<float,unsigned> gradient;
    vector<float> f0;
    f0.resize(FData->get(S).size());
    for(unsigned j=0;j<FData->get(S).size();j++){
      gradient.insert(pair<float,unsigned>(direction*(FData->get(S,j)),j));//gradient of the feature function for this particular target sentence
      f0[j]=origin*FData->get(S,j);//compute the feature function at the origin point
    }
    //now lets compute the 1best for each value of x
    
    //    vector<pair<float,unsigned> > onebest;
    

    multimap<float,unsigned>::iterator gradientit=gradient.begin();
    multimap<float,unsigned>::iterator highest_f0=gradient.begin();
  
    float smallest=gradientit->first;//smallest gradient
    //several candidates can have the lowest slope (eg for word penalty where the gradient is an integer )

    gradientit++;
    while(gradientit!=gradient.end()&&gradientit->first==smallest){
      //   cerr<<"ni"<<gradientit->second<<endl;;
      //cerr<<"fos"<<f0[gradientit->second]<<" "<<f0[index]<<" "<<index<<endl;
      if(f0[gradientit->second]>f0[highest_f0->second])
	   highest_f0=gradientit;//the highest line is the one with he highest f0
      gradientit++;
    }
    
    gradientit = highest_f0;
    first1best.push_back(highest_f0->second); 

    //now we look for the intersections points indicating a change of 1 best
    //we use the fact that the function is convex, which means that the gradient can only go up   
    while(gradientit!=gradient.end()){
      map<float,unsigned>::iterator leftmost=gradientit;
      float m=gradientit->first;
      float b=f0[gradientit->second];
      multimap<float,unsigned>::iterator gradientit2=gradientit;
      gradientit2++;
      float leftmostx=MAX_FLOAT;
      for(;gradientit2!=gradient.end();gradientit2++){
	//cerr<<"--"<<d++<<' '<<gradientit2->first<<' '<<gradientit2->second<<endl;
	//look for all candidate with a gradient bigger than the current one and find the one with the leftmost intersection
	float curintersect;
	if(m!=gradientit2->first){
	  curintersect=intersect(m,b,gradientit2->first,f0[gradientit2->second]);
          //cerr << "curintersect: " << curintersect << " leftmostx: " << leftmostx << endl;
	  if(curintersect<=leftmostx){
	    //we have found an intersection to the left of the leftmost we had so far.
	    //we might have curintersect==leftmostx for example is 2 candidates are the same
	    //in that case its better its better to update leftmost to gradientit2 to avoid some recomputing later
	  leftmostx=curintersect;
	  leftmost=gradientit2;//this is the new reference
	  }
	}
      }
      if (leftmost == gradientit) {
          //we didn't find any more intersections
	//the rightmost bestindex is the one with the highest slope.
	assert(abs(leftmost->first-gradient.rbegin()->first)<0.0001);//they should be egal but there might be  
	//a small difference due to rounding error
	break;
      }
      //we have found the next intersection!

      pair<unsigned,unsigned> newd(S,leftmost->second);//new onebest for Sentence S is leftmost->second

      if(leftmostx-previnserted->first<min_int){
	/* Require that the intersection Point be at least min_int
         to the right of the previous one(for this sentence). If not, we replace the
         previous intersection Point with this one. Yes, it can even
         happen that the new intersection Point is slightly to the
         left of the old one, because of numerical imprecision.
	 we do not check that we are to the right of the penultimate point also. it this happen the 1best the inteval will be wrong
	  we are going to replace previnsert by the new one because we do not want to keep
	  2 very close threshold: if the minima is there it could be an artifact
	*/
	map<float,diff_t>::iterator tit=thresholdmap.find(leftmostx);
	if(tit==previnserted){
	  //the threshold is the same as before can happen if 2 candidates are the same for example
	  assert(previnserted->second.back().first==newd.first);
	  previnserted->second.back()=newd;//just replace the 1 best fors sentence S
	  //previnsert doesnt change
	}else{

	  if(tit==thresholdmap.end()){
	    thresholdmap[leftmostx]=previnserted->second;//We keep the diffs at previnsert
	    thresholdmap.erase(previnserted);//erase old previnsert
	    previnserted=thresholdmap.find(leftmostx);//point previnsert to the new threshold
	    previnserted->second.back()=newd;//we update the diff for sentence S
	  }else{//threshold already exists but is not the previous one.
	    //we append the diffs in previnsert to tit before destroying previnsert
	    tit->second.insert(tit->second.end(),previnserted->second.begin(),previnserted->second.end());
	    assert(tit->second.back().first==newd.first);
	    tit->second.back()=newd;//change diff for sentence S
	    thresholdmap.erase(previnserted);//erase old previnsert
	    previnserted=tit;//point previnsert to the new threshold
	  }
	}

	assert(previnserted != thresholdmap.end());
      }else{//normal insertion process
	previnserted=AddThreshold(thresholdmap,leftmostx,newd);
      }
      gradientit=leftmost;
    }   //while(gradientit!=gradient.end()){
  }  //loop on S
  //now the thresholdlist is up to date: 
  //it contains a list of all the parameter_ts where the function changed its value, along with the nbest list for the interval after each threshold
  
  map<float,diff_t >::iterator thrit;
  if(verboselevel()>6){
    cerr << "Thresholds:(" <<thresholdmap.size()<<")"<< endl;
    for (thrit = thresholdmap.begin();thrit!=thresholdmap.end();thrit++){
      cerr << "x: " << thrit->first << " diffs";
      for (size_t j = 0; j < thrit->second.size(); ++j) {
	cerr << " " <<thrit->second[j].first << "," << thrit->second[j].second;
      }
      cerr << endl;
    }
  }
  
  //last thing to do is compute the Stat score (ie BLEU) and find the minimum
  thrit=thresholdmap.begin();
  ++thrit;//first diff corrrespond to MIN_FLOAT and first1best
  diffs_t diffs;
  for(;thrit!=thresholdmap.end();thrit++)
    diffs.push_back(thrit->second);
  vector<statscore_t> scores=GetIncStatScore(first1best,diffs);
  
  thrit=thresholdmap.begin();
  statscore_t bestscore=MIN_FLOAT;
  float bestx=MIN_FLOAT;
  assert(scores.size()==thresholdmap.size());//we skipped the first el of thresholdlist but GetIncStatScore return 1 more for first1best
  for(unsigned int sc=0;sc!=scores.size();sc++){
    //cerr << "x=" << thrit->first << " => " << scores[sc] << endl;
    if (scores[sc] > bestscore) {
        //This is the score for the interval [lit2->first, (lit2+1)->first]
        //unless we're at the last score, when it's the score
        //for the interval [lit2->first,+inf]
        bestscore = scores[sc];

        //if we're not in [-inf,x1] or [xn,+inf] then just take the value
        //if x which splits the interval in half. For the rightmost interval,
        //take x to be the last interval boundary + 0.1, and for the leftmost
        //interval, take x to be the first interval boundary - 1000.
        //These values are taken from cmert.
        float leftx = thrit->first;
        if (thrit == thresholdmap.begin()) {
            leftx = MIN_FLOAT;
        }
        ++thrit;
        float rightx = MAX_FLOAT;
        if (thrit != thresholdmap.end()) {
            rightx = thrit->first;
        }
        --thrit;
        //cerr << "leftx: " << leftx << " rightx: " << rightx << endl;
        if (leftx == MIN_FLOAT) {
            bestx = rightx-1000;
        } else if (rightx == MAX_FLOAT) {
            bestx = leftx+0.1;
        } else {
            bestx = 0.5 * (rightx + leftx);
        }
        //cerr << "x = " << "set new bestx to: " << bestx << endl;
    }
    ++thrit;
  }

  if(abs(bestx)<0.00015){
    bestx=0.0;//the origin of the line is the best point!we put it back at 0 so we do not propagate rounding erros
  //finally! we manage to extract the best score;
  //now we convert bestx  (position on the line) to a point!
    if(verboselevel()>4)
      cerr<<"best point on line at origin"<<endl;
  }
  if(verboselevel()>3){
//    cerr<<"end Lineopt, bestx="<<bestx<<endl;
	}
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
    cerr<<"error size mismatch between FeatureData and Scorer"<<endl;
    exit(2);
  }

	statscore_t score=GetStatScore(P);
	P.score=score;
  
	if(verboselevel()>2)
    cerr<<"Starting point: "<< P << " => "<< P.score << endl;
  statscore_t s=TrueRun(P);
  P.score=s;//just in case its not done in TrueRun
  if (verboselevel()>2)
    cerr<<"Ending point: "<< P <<" => "<< s << endl;
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
  Point  best;
	
	//If P is already defined and provides a score
  //we must improve over this score
	if(P.score>bestscore){
		bestscore=P.score;
		best=P;
	}
	
  int nrun=0;
  do{
    ++nrun;    
    if(verboselevel()>2&&nrun>1)
      cerr<<"last diff="<<bestscore-prevscore<<" nrun "<<nrun<<endl;
    prevscore=bestscore;
    
    Point  linebest;
    
    for(unsigned int d=0;d<Point::getdim();d++){
      if(verboselevel()>4){
				//	cerr<<"minimizing along direction "<<d<<endl;
				cerr<<"starting point: " << P << " => " << prevscore << endl;
      }
      Point direction;
      for(unsigned int i=0;i<Point::getdim();i++)
				direction[i];
      direction[d]=1.0;
      statscore_t curscore=LineOptimize(P,direction,linebest);//find the minimum on the line
				if(verboselevel()>5){
					cerr<<"direction: "<< d << " => " << curscore << endl;
					cerr<<"\tending point: "<< linebest << " => " << curscore << endl;
				}
				if(curscore>bestscore){
					bestscore=curscore;
					best=linebest;	
					if(verboselevel()>3){
						cerr<<"new best dir:"<<d<<" ("<<nrun<<")"<<endl;
						cerr<<"new best Point "<<best<< " => " <<curscore<<endl;
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
  for(unsigned int d=0;d<Point::getdim();d++){
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
  unsigned int thetype;
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
    cerr<<"Error: unknown Optimizer type "<<type<<endl;
    cerr<<"Known Algorithm are:"<<endl;
    unsigned int thetype;
    for(thetype=0;thetype<typenames.size();thetype++)
      cerr<<typenames[thetype]<<endl;
    throw ("unknown Optimizer Type");
  }
  
  switch((OptType)T){
  case POWELL:
    return new SimpleOptimizer(dim,i2o,start);
    break;
  case RANDOM:
    return new RandomOptimizer(dim,i2o,start);
    break;
  default:
    cerr<<"Error: unknown optimizer"<<type<<endl;
    return NULL; 
  }  
}
