/******************************************************************************
IrstLM: IRST Language Model Toolkit
Copyright (C) 2006 Marcello Federico, ITC-irst Trento, Italy

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

******************************************************************************/

using namespace std;

#include <cmath>
#include "mfstream.h"
#include "mempool.h"
#include "dictionary.h"
#include "n_gram.h"
#include "ngramtable.h"
#include "interplm.h"
#include "normcache.h"
#include "ngramcache.h"
#include "mdiadapt.h"
#include "shiftlm.h"
#include "linearlm.h"
#include "mixture.h"
#include "cmd.h"

//
//Mixture interpolated language model
//

static Enum_T SLmTypeEnum [] = {
  {    "ModifiedShiftBeta",  MOD_SHIFT_BETA }, 
  {    "msb",                MOD_SHIFT_BETA }, 
  {    "InterpShiftBeta",    SHIFT_BETA }, 
  {    "sb",                 SHIFT_BETA }, 
  {    "InterpShiftOne",     SHIFT_ONE },
  {    "s1",                 SHIFT_ONE },
  {    "InterpShiftZero",    SHIFT_ZERO },
  {    "s0",                 SHIFT_ZERO },
  {    "LinearWittenBell",   LINEAR_WB },
  {    "wb",                 LINEAR_WB },
  {    "Mixture",            MIXTURE},
  END_ENUM
};


mixture::mixture(char* bigtable,char* sublminfo,int depth,int prunefreq,char* ipfile,char* opfile):
  mdiadaptlm(bigtable,depth)
{
	
	prunethresh=prunefreq;
	ipfname=ipfile;
	opfname=opfile;
	
	mfstream inp(sublminfo,ios::in );
	if (!inp){
		cerr << "cannot open " << sublminfo << "\n";
		exit(1);
	}
	
	inp >> numslm;
	
	sublm=new interplm* [numslm];
	int slmtype;
	char *trainfile;
	
	DeclareParams(
				  "slm",CMDENUMTYPE, &slmtype, SLmTypeEnum,
				  "str",CMDSTRINGTYPE, &trainfile,
				  "sp",CMDSUBRANGETYPE, &prunefreq, 0 , 1000,
				  (char *)NULL);
	
	
	for (int i=0;i<numslm;i++)
    {
		int npar=4;
		char **par=new char*[npar];  
		for (int j=0;j<4;j++){
			par[j]=new char[100];
		}    
		
		par[0][0]='\0';
		inp >> par[1] >> par[2] >> par[3];
		
		trainfile=NULL;slmtype=0;prunefreq=-1;
		GetParams(&npar, &par, (char*) NULL);
		
		if (!slmtype || !trainfile || !prunefreq==-1){
			cerr << "slm incomplete parameters\n";
			exit(1);
		}
		
		switch (slmtype){
				
				
			case LINEAR_WB:
				sublm[i]=new linearwb(trainfile,depth,prunefreq,MSHIFTBETA_I);
				break;
				
			case SHIFT_BETA:
				sublm[i]=new shiftbeta(trainfile,depth,prunefreq,-1,SHIFTBETA_I);
				break;
				
			case SHIFT_ONE:
				sublm[i]=new shiftbeta(trainfile,depth,prunefreq,SIMPLE_I);
				break;
				
			case MOD_SHIFT_BETA:
				sublm[i]=new mshiftbeta(trainfile,depth,prunefreq,MSHIFTBETA_I);
				break;
				
			case MIXTURE:
				sublm[i]=new mixture((char *)NULL,trainfile,depth,prunefreq);
				break;
				
			default:
				cerr << "not implemented yet\n";
				exit(1);
		};
		
		cerr << "eventually generate OOV code ";
		cerr << sublm[i]->dict->encode(dict->OOV()) << "\n";      
		
		if ((slmtype==MIXTURE) || (i<(numslm-1)))
			augment(sublm[i]);
    }
	
	//tying parameters
	k1=2;
	k2=10;
	
};

double mixture::reldist(double *l1,double *l2,int n){
  double dist=0.0,size=0.0;
  for (int i=0;i<n;i++){
    dist+=(l1[i]-l2[i])*(l1[i]-l2[i]);
    size+=l1[i]*l1[i];
  }
  return sqrt(dist/size);
}


double rand01(){
  return (double)rand()/(double)RAND_MAX;
}

int mixture::genpmap(){
	dictionary* d=sublm[0]->dict;
	
	cerr << "Computing parameters mapping: ..." << d->size() << " ";
	pm=new int[d->size()];
	//initialize
	for (int i=0;i<d->size();i++) pm[i]=0;
	
	pmax=k2-k1+1; //update # of parameters	
	
	for (int w=0;w<d->size();w++){
		int f=d->freq(w);
		if ((f>k1) && (f<=k2)) pm[w]=f-k1;
		else if (f>k2){
			pm[w]=pmax++;
		}
	}
	cerr << "pmax " << pmax << " ";
	return 1;
}

int mixture::pmap(ngram ng,int lev){
  
  ngram h(sublm[0]->dict);
  h.trans(ng);
  
  if (lev<=1) return 0;
  //get the last word of history  
  if (!sublm[0]->get(h,2,1)) return 0;
  return (int) pm[*h.wordp(2)];
}


int mixture::savepar(char* opf){
  mfstream out(opf,ios::out);
  
  cerr << "saving parameters in " << opf << "\n";
  out << lmsize() << " " << pmax << "\n";

  for (int i=0;i<=lmsize();i++)
    for (int j=0;j<pmax;j++)
      out.writex(l[i][j],sizeof(double),numslm);

  
  return 1;
}


int mixture::loadpar(char* ipf){

  mfstream inp(ipf,ios::in);
  
  if (!inp){
    cerr << "cannot open file with parameters: " << ipf << "\n";
    exit(1);
  }

  cerr << "loading parameters from " << ipf << "\n";

  // check compatibility
  char header[100];
  inp.getline(header,100);
  int value1,value2;
  sscanf(header,"%d %d",&value1,&value2);
  
  if (value1 != lmsize() || value2 != pmax){
    cerr << "parameter file " << ipf << " is incompatible\n";
    exit(1);
  }

  for (int i=0;i<=lmsize();i++)
    for (int j=0;j<pmax;j++)
      inp.readx(l[i][j],sizeof(double),numslm);

  return 1;
}

int mixture::train(){
	
	double zf;
	
	srand(1333);
	
	genpmap();
	
	if (dub()<=dict->size()){
		cerr << "\nWarning: DUB value is too small: the LM will\n";
		cerr << "possibly compute wrong probabilities if sub-LMs\n";
		cerr << "have different vocabularies!\n";
	}
	
	cerr << "mixlm --> DUB: " << dub() << endl;
	for (int i=0;i<numslm;i++){
		cerr << i << " sublm --> DUB: " << sublm[i]->dub()  << endl;
		cerr << "eventually generate OOV code ";
		cerr << sublm[i]->dict->encode(sublm[i]->dict->OOV()) << "\n";
		sublm[i]->train();
	}
	
	//initialize parameters
	
	for (int i=0;i<=lmsize();i++){
		l[i]=new double*[pmax];
		for (int j=0;j<pmax;j++){
			l[i][j]=new double[numslm];
			for (int k=0;k<numslm;k++)
				l[i][j][k]=1.0/(double)numslm;
		}}
	
	if (ipfname){
		//load parameters from file
		loadpar(ipfname);
	}
	else{
		//start training of mixture model
		
		double oldl[pmax][numslm];
		char alive[pmax],used[pmax];
		int totalive;
		
		ngram ng(sublm[0]->dict);
		
		for (int lev=1;lev<=lmsize();lev++){
			
			zf=sublm[0]->zerofreq(lev);
			
			cerr << "Starting training at lev:" << lev << "\n";
			
			for (int i=0;i<pmax;i++) {
				alive[i]=1;
				used[i]=0;
			}
			totalive=1;
			int iter=0;
			while (totalive && (iter < 20) ){
				
				iter++;
				
				for (int i=0;i<pmax;i++)
					if (alive[i])
						for (int j=0;j<numslm;j++) {
							oldl[i][j]=l[lev][i][j];
							l[lev][i][j]=1.0/(double)numslm;
						}
				
				sublm[0]->scan(ng,INIT,lev);
				while(sublm[0]->scan(ng,CONT,lev)){

					//do not include oov for unigrams
					if ((lev==1) && (*ng.wordp(1)==sublm[0]->dict->oovcode()))
						continue;
					
					int par=pmap(ng,lev);
					used[par]=1;

					//controllo se aggiornare il parametro
					if (alive[par]){
						
						double backoff=(lev>1?prob(ng,lev-1):1); //backoff
						double numer[numslm],denom=0.0;
						double fstar,lambda;
						
						//int cv=(int)floor(zf * (double)ng.freq + rand01());      	
						//int cv=1; //old version of leaving-one-out
						int cv=(int)floor(zf * (double)ng.freq)+1;      	
						//int cv=1; //old version of leaving-one-out
						//if (lev==3)q
						
						//if (iter>10)
						// cout << ng 
						// << " backoff " << backoff 
						// << " level " << lev 
						// << "\n";	  
						
						for (int i=0;i<numslm;i++){
							
							//use cv if i=0
							
							sublm[i]->discount(ng,lev,fstar,lambda,(i==0)*(cv)); 
							numer[i]=oldl[par][i]*(fstar + lambda * backoff);
							
							assert(numer[i]>0);
							
							ngram ngslm(sublm[i]->dict);
							ngslm.trans(ng);
							if ((*ngslm.wordp(1)==sublm[i]->dict->oovcode()) &&
								(dict->dub() > sublm[i]->dict->size()))
								numer[i]/=(double)(dict->dub() - sublm[i]->dict->size());
							
							assert(numer[i]>0);
							
							denom+=numer[i];
						};
						
						for (int i=0;i<numslm;i++){ 
							l[lev][par][i]+=(ng.freq * (numer[i]/denom));
							//if (iter>10)
							//cout << ng << " l: " << l[lev][par][i] << "\n";	  
						}
					}
				}
				
				//normalize all parameters
				totalive=0;
				for (int i=0;i<pmax;i++){
					double tot=0;
					if (alive[i]){
						for (int j=0;j<numslm;j++) tot+=(l[lev][i][j]);
						for (int j=0;j<numslm;j++) l[lev][i][j]/=tot;
						
						//decide if to continue to update
						if (!used[i] || (reldist(l[lev][i],oldl[i],numslm)<=0.05))
							alive[i]=0;
					}
					totalive+=alive[i];
				}
				
				cerr << "Lev " << lev << " iter " << iter << " tot alive " << totalive << "\n";
				
			}
		}
	}
	
	if (opfname) savepar(opfname);
	
	
	return 1;
}

int mixture::discount(ngram ng_,int size,double& fstar,double& lambda,int /* unused parameter: cv */){
	
	ngram ng(dict);ng.trans(ng_);
	
	double lambda2,fstar2;
	fstar=0.0;lambda=0.0;
	int p=pmap(ng,size);
	assert(p <= pmax);
	double lsum=0;
	

	for (int i=0;i<numslm;i++){
		sublm[i]->discount(ng,size,fstar2,lambda2,0);

			
		ngram ngslm(sublm[i]->dict);
		ngslm.trans(ng);
		
		if (dict->dub() > sublm[i]->dict->size())
			if (*ngslm.wordp(1) == sublm[i]->dict->oovcode()){
				fstar2/=(double)(dict->dub() - sublm[i]->dict->size()+1);
			}
		
		
		fstar+=(l[size][p][i]*fstar2);
		lambda+=(l[size][p][i]*lambda2);
		lsum+=l[size][p][i];
	}
	
	if (dict->dub() > dict->size())
		if (*ng.wordp(1) == dict->oovcode()){
			fstar*=(double)(dict->dub() - dict->size()+1);
		}
	
	assert((lsum>0.999999999999) && (lsum <=1.000000000001));
	return 1;
}









