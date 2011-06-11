// $Id: compile-lm.cpp 3677 2010-10-13 09:06:51Z bertoldi $

/******************************************************************************
 IrstLM: IRST Language Model Toolkit, compile LM
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

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdlib.h>
#include "util.h"
#include "math.h"
#include "lmtable.h"


/* GLOBAL OPTIONS ***************/
std::string sinvert = "no";
std::string stxt = "no";
std::string sscore = "no";
std::string seval = "";
std::string srandcalls = "0";
std::string sfilter = "";
std::string sdebug = "0"; 
std::string smemmap = "0";
std::string sdub = "10000000";//10^7
std::string skeepunigrams = "yes";
std::string tmpdir = "";
std::string sdictionary_load_factor = "0.0";
std::string sngramcache_load_factor = "0.0";

/********************************/

void usage(const char *msg = 0) {

	if (msg) { std::cerr << msg << std::endl; }
	std::cerr << "Usage: compile-lm [options] input-file.lm [output-file.blm]" << std::endl;
	if (!msg) std::cerr << std::endl
		<< "  compile-lm reads a standard LM file in ARPA format and produces" << std::endl
		<< "  a compiled representation that the IRST LM toolkit can quickly" << std::endl
		<< "  read and process. LM file can be compressed with gzip." << std::endl << std::endl;
	std::cerr << "Options:\n"
        << "--text|-t [yes|no]  (output is again in text format)" << std::endl
	<< "--invert|-i [yes|no]  (build an inverted n-gram binary table for fast access: default no)" << std::endl
        << "--filter|-f wordlist (filter a binary language model with a word list)"<< std::endl
        << "--keepunigrams|-ku [yes|no] (filter by keeping all unigrams in the table: default yes)"<< std::endl
	<< "--eval|-e text-file (computes perplexity of text-file and returns)"<< std::endl
        << "--randcalls|-r N (computes N random calls on the eval text-file)"<< std::endl
	<< "--dub dict-size (dictionary upperbound to compute OOV word penalty: default 10^7)"<< std::endl
	<< "--score|-s [yes|no]  (computes log-prob scores from standard input)"<< std::endl
	<< "--debug|-d 1 (verbose output for --eval option)"<< std::endl
	<< "--memmap|-mm 1 (uses memory map to read a binary LM)"<< std::endl
	<< "--ngram_load_factor <value> (set the load factor for ngram cache ; it should be a positive real value; if not defined a default value is used)" << std::endl
	<< "--dict_load_factor <value> (set the load factor for ngram cache ; it should be a positive real value; if not defined a default value is used)" << std::endl
	<< "--tmpdir directory (directory for temporary computation, default is either the environment variable TMP if defined or \"/tmp\")"
	<< std::endl;
}

bool starts_with(const std::string &s, const std::string &pre) {
  if (pre.size() > s.size()) return false;

  if (pre == s) return true;
  std::string pre_equals(pre+'=');
  if (pre_equals.size() > s.size()) return false;
  return (s.substr(0,pre_equals.size()) == pre_equals);
}

std::string get_param(const std::string& opt, int argc, const char **argv, int& argi)
{
  std::string::size_type equals = opt.find_first_of('=');
  if (equals != std::string::npos && equals < opt.size()-1) {
    return opt.substr(equals+1);
  }
  std::string nexto;
  if (argi + 1 < argc) { 
    nexto = argv[++argi]; 
  } else {
    usage((opt + " requires a value!").c_str());
    exit(1);
  }
  return nexto;
}

void handle_option(const std::string& opt, int argc, const char **argv, int& argi)
{
  if (opt == "--help" || opt == "-h") { usage(); exit(1); }
  
  if (starts_with(opt, "--text") || starts_with(opt, "-t"))
    stxt = get_param(opt, argc, argv, argi);
  else
  if (starts_with(opt, "--filter") || starts_with(opt, "-f"))
    sfilter = get_param(opt, argc, argv, argi);
  else
  if (starts_with(opt, "--keepunigrams") || starts_with(opt, "-ku"))
    skeepunigrams = get_param(opt, argc, argv, argi);
  else
    if (starts_with(opt, "--eval") || starts_with(opt, "-e"))
      seval = get_param(opt, argc, argv, argi);
  else
     if (starts_with(opt, "--randcalls") || starts_with(opt, "-r"))
      srandcalls = get_param(opt, argc, argv, argi);
  else
    if (starts_with(opt, "--score") || starts_with(opt, "-s"))
      sscore = get_param(opt, argc, argv, argi);  
  else
    if (starts_with(opt, "--debug") || starts_with(opt, "-d"))
      sdebug = get_param(opt, argc, argv, argi);
  else
    if (starts_with(opt, "--memmap") || starts_with(opt, "-mm") || starts_with(opt, "-m"))
      smemmap = get_param(opt, argc, argv, argi);     
  else
    if (starts_with(opt, "--dub") || starts_with(opt, "-dub"))
      sdub = get_param(opt, argc, argv, argi);     
  else
    if (starts_with(opt, "--tmpdir") || starts_with(opt, "-tmpdir"))
      tmpdir = get_param(opt, argc, argv, argi);
  else
    if (starts_with(opt, "--invert") || starts_with(opt, "-i"))
      sinvert = get_param(opt, argc, argv, argi);
  else
    if (starts_with(opt, "--dict_load_factor"))
    sdictionary_load_factor = get_param(opt, argc, argv, argi);
  else
    if (starts_with(opt, "--ngram_load_factor"))
    sngramcache_load_factor = get_param(opt, argc, argv, argi);


  else {
    usage(("Don't understand option " + opt).c_str());
    exit(1);
  }
}

int main(int argc, const char **argv)
{

	if (argc < 2) { usage(); exit(1); }
	std::vector<std::string> files;
	for (int i=1; i < argc; i++) {
		std::string opt = argv[i];
		if (opt[0] == '-') { handle_option(opt, argc, argv, i); }
		else files.push_back(opt);
	}
	
	if (files.size() > 2) { usage("Too many arguments"); exit(1); }
	if (files.size() < 1) { usage("Please specify a LM file to read from"); exit(1); }
	
	bool textoutput = (stxt == "yes"? true : false);
	bool invert = (sinvert == "yes"? true : false);

	//Define output type of table
	OUTFILE_TYPE outtype;
	if (textoutput) 
		outtype=TEXT;
	else if	(seval != "" || sscore == "yes") 
		outtype=NONE;
	else  
		outtype=BINARY;
	
	
	int debug = atoi(sdebug.c_str()); 
	int memmap = atoi(smemmap.c_str());
	int dub = atoi(sdub.c_str());
	int randcalls = atoi(srandcalls.c_str());
	float ngramcache_load_factor = (float) atof(sngramcache_load_factor.c_str());
	float dictionary_load_factor = (float) atof(sdictionary_load_factor.c_str());	
	
	std::string infile = files[0];
	std::string outfile="";
	
	if (files.size() == 1) {  
		outfile=infile;
		
		//remove path information
		std::string::size_type p = outfile.rfind('/');
		if (p != std::string::npos && ((p+1) < outfile.size()))           
			outfile.erase(0,p+1);
		
		//eventually strip .gz 
		if (outfile.compare(outfile.size()-3,3,".gz")==0)
			outfile.erase(outfile.size()-3,3);
		
		outfile+=(textoutput?".lm":".blm");
	}
	else
		outfile = files[1];
	
	std::cerr << "inpfile: " << infile << std::endl;
	if (sscore=="" && seval=="") std::cerr << "outfile: " << outfile << std::endl;
	if (sscore=="") std::cerr << "interactive: " << sscore << std::endl;
	if (memmap) std::cerr << "memory mapping: " << memmap << std::endl;
	std::cerr << "dub: " << dub<< std::endl;
	if (tmpdir != ""){
		if (setenv("TMP",tmpdir.c_str(),1))
			std::cerr << "temporary directory has not been set" << std::endl;
		std::cerr << "tmpdir: " << tmpdir << std::endl;
	}
	
	lmtable* lmt=new lmtable(ngramcache_load_factor,dictionary_load_factor); 
	
	//let know that table has inverted n-grams
	if (invert) lmt->is_inverted(invert);

	std::cerr << "Reading " << infile << "..." << std::endl;
	inputfilestream inp(infile.c_str());
	
	if (!inp.good()) {
		std::cerr << "Failed to open " << infile << "!" << std::endl;
		exit(1);
	}  
    
	if (sfilter != ""){
		std::cerr << "loading filtered version of LM ... \n";
		lmt->load(inp,infile.c_str(),outfile.c_str(),memmap=1,outtype);
		dictionary *dict; dict=new dictionary((char *)sfilter.c_str());
		lmtable* sublmt; sublmt=lmt->cpsublm(dict,(skeepunigrams=="yes"));
		delete lmt; lmt=sublmt;
		delete dict;
		std::cerr << "...done\n";
	}
	else{	
		lmt->load(inp,infile.c_str(),outfile.c_str(),memmap,outtype);
	}
	
	
	if (debug) 
		std::cout << "lmtable has " << (lmt->is_inverted()?"inverted":"direct") << " ngrams\n";
		
	if (dub) lmt->setlogOOVpenalty((int)dub);

        //use caches to save time (only if PS_CACHE_ENABLE is defined through compilation flags)
        lmt->init_caches(lmt->maxlevel());
	
	if (seval != ""){		
		if (randcalls>0){ 
			
			cerr << "perform random " << randcalls << " using dictionary of test set\n";
			dictionary *dict; dict=new dictionary((char *)seval.c_str());

			//build extensive histogram
			int histo[dict->totfreq()]; //total frequency
		    int totfreq=0;
			for (int n=0;n<dict->size();n++)
				for (int m=0;m<dict->freq(n);m++) 
				histo[totfreq++]=n;
			
			ngram ng(lmt->dict); 
			srand(1234);
			double bow; int bol=0; 

			if (debug>1) ResetUserTime();			
			
			for (int n=0;n<randcalls;n++){
				//extracts a random word from dict
				int w=histo[rand() % totfreq];
				
				
				ng.pushc(lmt->dict->encode(dict->decode(w)));	

				lmt->clprob(ng,&bow,&bol);  //(using caches if available)  
	
				if (debug==1){
					std::cout << ng.dict->decode(*ng.wordp(1)) << "[" << lmt->maxlevel()-bol << "]" << " "; 
					std::cout << std::endl;
				}
				
				if ((n % 100000)==0){ 
					std::cerr << ".";
					lmt->check_caches_levels();   
				}
			}
			std::cerr << "\n";
			if (debug>1) PrintUserTime("Finished in");
			if (debug>1) lmt->stat();

			delete lmt;
			return 0;    
			
		}
		else
		{
			std::cerr << "Start Eval" << std::endl;
			std::cerr << "OOV code: " << lmt->dict->oovcode() << std::endl;
			ngram ng(lmt->dict);    
			std::cout.setf(ios::fixed);
			std::cout.precision(2);
			
			if (debug>0) std::cout.precision(8);
			std::fstream inptxt(seval.c_str(),std::ios::in);
			
			int Nbo=0;
			int Nw=0,Noov=0;
			double logPr=0,PP=0,PPwp=0,Pr;

			ng.dict->incflag(1);
			int bos=ng.dict->encode(ng.dict->BoS());
			int eos=ng.dict->encode(ng.dict->EoS());
			ng.dict->incflag(0);
			
			double bow; int bol=0; char *msp; unsigned int statesize;
			while(inptxt >> ng){      

				if (ng.size>lmt->maxlevel()) ng.size=lmt->maxlevel();
				
				// reset ngram at begin of sentence
				if (*ng.wordp(1)==bos) {
					ng.size=1;
					continue;
				}
				
				if (ng.size>=1){
					Pr=lmt->clprob(ng,&bow,&bol,&msp,&statesize); 
					logPr+=Pr; 

					if (debug==1){
						std::cout << ng.dict->decode(*ng.wordp(1)) << "[" << ng.size-bol << "]" << " "; 
						if (*ng.wordp(1)==eos) std::cout << std::endl;
					}
					if (debug==2){
						std::cout << ng << "[" << ng.size-bol << "-gram]" << " " << Pr << std::endl; 
					}
					if (debug==3){
						std::cout << ng << "[" << ng.size-bol << "-gram]" << " " << Pr << " bow:" << bow << std::endl; 
					}
					if (debug==4){
						std::cout << ng << "[" << ng.size-bol << "-gram: recombine:" << statesize << " state:" << (void*) msp << "]" << " " << Pr << " bow:" << bow << std::endl;		
					}
					if (debug>4){
						std::cout << ng << "[" << ng.size-bol << "-gram: recombine:" << statesize << " state:" << (void*) msp << "]" << " " << Pr << " bow:" << bow << std::endl;
						double totp=0.0; int oldw=*ng.wordp(1);
						double oovp=lmt->getlogOOVpenalty();lmt->setlogOOVpenalty2(0);
						for (int c=0;c<ng.dict->size();c++){
							*ng.wordp(1)=c;
							totp+=pow(10.0,lmt->clprob(ng)); //(using caches if available)  
						}
						*ng.wordp(1)=oldw;
						
						if ( totp < (1.0 - 1e-5) || totp > (1.0 + 1e-5))
							std::cout << "  [t=" << totp << "] POSSIBLE ERROR\n";
						else 
							std::cout << "\n";
						
						lmt->setlogOOVpenalty2((double)oovp);
					}
					
					if (*ng.wordp(1) == lmt->dict->oovcode()){
						Noov++;
					} 
					if (bol){  Nbo++; }
					Nw++;                 
				}
			} 
			
			PP=exp((-logPr * log(10.0)) /Nw);
			
			PPwp= PP * (1 - 1/exp((Noov *  lmt->getlogOOVpenalty()) * log(10.0) / Nw));

			std::cout << "%% Nw=" << Nw 
			<< " PP=" << PP 
			<< " PPwp=" << PPwp
			<< " Nbo=" << Nbo 
			<< " Noov=" << Noov 
			<< " OOV=" << (float)Noov/Nw * 100.0 << "%" << std::endl;
	lmt->used_caches();
	lmt->stat();
		
			if (debug>1) lmt->stat();
			
			delete lmt;
			return 0;    
		};
	}	
	
	if (sscore == "yes"){   
		
		ngram ng(lmt->dict);    
		int bos=ng.dict->encode(ng.dict->BoS());

		int bol; double bow;
		unsigned int n=0;

                std::cout.setf(ios::scientific);
                std::cout << "> ";

		while(std::cin >> ng){
			
                        //std::cout << ng << std::endl;;
			// reset ngram at begin of sentence
                        if (*ng.wordp(1)==bos) {ng.size=1;continue;}

			if (ng.size>=lmt->maxlevel()){
				ng.size=lmt->maxlevel();
				++n;
				if ((n % 10000000)==0){ 
					std::cerr << ".";
					lmt->check_caches_levels();   
				}
                                std::cout << ng << " p= " << lmt->clprob(ng,&bow,&bol) * M_LN10;
                                std::cout << " bo= " << bol << std::endl;
			}
			else{
				std::cout << ng << " p= NULL" << std::endl;      
			}
			std::cout << "> ";                 
		}
		std::cout << std::endl;                
		if (debug>1) lmt->stat();
		delete lmt;
		return 0;
	}
	
	if (textoutput) {
		std::cout << "Saving in txt format to " << outfile << std::endl;
		lmt->savetxt(outfile.c_str());    
	} else if (sfilter != "" || !memmap) {
		std::cout << "Saving in bin format to " << outfile << std::endl;
		lmt->savebin(outfile.c_str());
	}
	delete lmt;
	return 0;
}

