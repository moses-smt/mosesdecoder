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

// Adapted LM classes: extension of interp classes

#ifndef MF_MDIADAPTLM_H
#define MF_MDIADAPTLM_H

class mdiadaptlm:public interplm{

  int adaptlev;
  interplm* forelm;
  double zeta0;
  double oovscaling;

protected:
  normcache *cache;

//to improve access speed
  NGRAMCACHE_t** probcache;
  NGRAMCACHE_t** backoffcache; 
  int max_caching_level;

 public:

  mdiadaptlm(char* ngtfile,int depth=0,TABLETYPE tt=FULL);

  inline normcache* get_zetacache(){ return cache; }
  inline NGRAMCACHE_t* get_probcache(int level);
  inline NGRAMCACHE_t* get_backoffcache(int level);
	
  void create_caches(int mcl);
  void init_caches();
  void init_caches(int level);
  void delete_caches();
  void delete_caches(int level);
		
  void check_cache_levels();
  void check_cache_levels(int level);
  void reset_caches();
  void reset_caches(int level);
	
  void caches_stat();
	
  double gis_step;

  double zeta(ngram ng,int size);
  
  int discount(ngram ng,int size,double& fstar,double& lambda,int cv=0);

  int bodiscount(ngram ng,int size,double& fstar,double& lambda,double& bo);

  int compute_backoff();

  double backunig(ngram ng);

  double foreunig(ngram ng);
  
  int adapt(char* ngtfile,int alev=1,double gis_step=0.4);
  
  int scalefact(char* ngtfile);
  
  double scalefact(ngram ng);
  
  double prob(ngram ng,int size);
  double prob(ngram ng,int size,double& fstar,double& lambda, double& bo);
		
  double prob2(ngram ng,int size,double & fstar);
  
  double txclprob(ngram ng,int size);

  int saveASR(char *filename,int backoff,char* subdictfile=NULL);
  int saveMT(char *filename,int backoff,char* subdictfile=NULL,int resolution=10000000,double decay=0.999900);
  int saveARPA(char *filename,int backoff=0,char* subdictfile=NULL);
  int saveBIN(char *filename,int backoff=0,char* subdictfile=NULL,int mmap=0);
  
  int netsize();
	
  ~mdiadaptlm();

  double myround(double x){
    long int i=(long int)x;
    return (x-i)>0.500?i+1.0:(double)i;
  }
  
};

#endif






