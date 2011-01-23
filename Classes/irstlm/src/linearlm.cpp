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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "mfstream.h"
#include "mempool.h"
#include "htable.h"
#include "dictionary.h"
#include "n_gram.h"
#include "mempool.h"
#include "ngramtable.h"
#include "ngramcache.h"
#include "normcache.h"
#include "interplm.h"
#include "mdiadapt.h"
#include "linearlm.h"

//
//Linear interpolated language model: Witten & Bell discounting scheme
//


linearwb::linearwb(char* ngtfile,int depth,int prunefreq,TABLETYPE tt):
  mdiadaptlm(ngtfile,depth,tt)
{
  prunethresh=prunefreq;
  cerr << "PruneThresh: " << prunethresh << "\n";
	
};




int linearwb::train(){
  
  trainunigr();

  
  gensuccstat();
	
 return 1;
}


int linearwb::discount(ngram ng_,int size,double& fstar,double& lambda,int cv)
{
	
	ngram ng(dict);ng.trans(ng_);
	
	if (size > 1){
		
		ngram history=ng;
		
		if (ng.ckhisto(size) && get(history,size,size-1) && (history.freq>cv) &&
			
			((size < 3) || ((history.freq-cv) > prunethresh))){ 
			
			// apply history pruning on trigrams only 

			
			if (get(ng,size,size) && (!prunesingletons() || ng.freq>1 || size<3)){ 
				
				// apply frequency pruning on trigrams only 
				
				cv=(cv>ng.freq)?ng.freq:cv;
				
				if (ng.freq >cv){
					
					fstar=(double)(ng.freq-cv)/(double)(history.freq -cv + history.succ);
					
					lambda=(double)history.succ/(double)(history.freq -cv + history.succ);
					
					if (size>=3 && prunesingletons())  // correction due to frequency pruning
						lambda+=(double)succ1(history.link)/(double)(history.freq -cv + history.succ);      

					// succ1(history.link) is not affected when ng.freq > cv 
					
				}
				else{ // ng.freq == cv
					
					fstar=0.0;
					
					lambda=(double)(history.succ-1)/  // remove cv n-grams from data
					(double)(history.freq - cv + history.succ - 1);
					
					if (size>=3 && prunesingletons())  // correction due to frequency pruning
						
						lambda+=(double)succ1(history.link)-(cv==1 && ng.freq==1?1:0)/(double)(history.freq -cv + history.succ -1);      

				}
			}
			else{
				
				fstar=0.0;
				
				lambda=(double)history.succ/(double)(history.freq + history.succ);
				
				if (size>=3 && prunesingletons())  // correction due to frequency pruning
					lambda+=(double)succ1(history.link)/(double)(history.freq + history.succ);      
			}
			
			//cerr << "ngram :" << ng << "\n";
			
			// if current word is OOV then back-off to unigrams!
			
			if (*ng.wordp(1)==dict->oovcode()){
				lambda+=fstar;
				fstar=0.0;
				assert(lambda<=1 && lambda>0);
			}
			else{  // add f*(oov|...) to lambda
				*ng.wordp(1)=dict->oovcode();
				if (get(ng,size,size) && (!prunesingletons() || ng.freq>1 || size<3))
					lambda+=(double)ng.freq/(double)(history.freq - cv + history.succ);
			}
		}
		else{
			fstar=0;
			lambda=1;
		}
	}
	else{
		fstar=unigr(ng);
		lambda=0;
	}
	
	return 1;
}


