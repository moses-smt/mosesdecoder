#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "LanguageModelRemote.h"
#include "Factor.h"
#include "StaticData.h"

namespace Moses {

const Factor* LanguageModelRemote::BOS = NULL;
const Factor* LanguageModelRemote::EOS = (LanguageModelRemote::BOS + 1);

LanguageModelRemote::LanguageModelRemote(bool registerScore, ScoreIndexManager &scoreIndexManager) 
:LanguageModelSingleFactor(registerScore, scoreIndexManager)
{
}

bool LanguageModelRemote::Load(const std::string &filePath
                                        , FactorType factorType
                                        , float weight
                                        , size_t nGramOrder) 
{
        m_factorType    = factorType;
        m_weight                        = weight;
        m_nGramOrder    = nGramOrder;

	int cutAt = filePath.find(':',0);
	std::string host = filePath.substr(0,cutAt);
        //std::cerr << "port string = '" << filePath.substr(cutAt+1,filePath.size()-cutAt) << "'\n";
	int port = atoi(filePath.substr(cutAt+1,filePath.size()-cutAt).c_str());
	bool good = start(host,port);
	if (!good) {
		std::cerr << "failed to connect to lm server on " << host << " on port " << port << std::endl;
	}
	ClearSentenceCache();
	return good;
}


bool LanguageModelRemote::start(const std::string& host, int port) {
  //std::cerr << "host = " << host << ", port = " << port << "\n";
  sock = socket(AF_INET, SOCK_STREAM, 0);
  hp = gethostbyname(host.c_str());
  if (hp==NULL) { herror("gethostbyname failed"); exit(1); }

  bzero((char *)&server, sizeof(server));
  bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
  server.sin_family = hp->h_addrtype;
  server.sin_port = htons(port);

  int errors = 0;
  while (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
      //std::cerr << "Error: connect()\n";
      sleep(1);
      errors++;
      if (errors > 5) return false;
  }
  return true;
}

void readNBytesForSure(int sock, char * buf, int size) {
	int r = read(sock, buf, size);
	int errors = 0;
	int cnt = 0;
	while (1) {
		if (r < 0) {
			errors++; sleep(1);
			if (errors > 5) exit(1);
		}
		else if (r==0 || buf[cnt] == '\n') {
			break;
		}
		else {
			cnt += r;
			if (cnt==size) break;
			read(sock, &buf[cnt], size-cnt);
		}
	}
}

float LanguageModelRemote::GetValue(const std::vector<const Word*> &contextFactor, State* finalState, unsigned int* len) const {
  size_t count = contextFactor.size();
  if (count == 0) {
    if (finalState) *finalState = NULL;
    return 0;
  }
  //std::cerr << "contextFactor.size() = " << count << "\n";
  size_t max = m_nGramOrder;
  const FactorType factor = GetFactorType();
  if (max > count) max = count;
 
  Cache* cur = &m_cache;
  int pc = static_cast<int>(count) - 1;
  for (int i = 0; i < pc; ++i) {
    const Factor* f = contextFactor[i]->GetFactor(factor);
    cur = &cur->tree[f ? f : BOS];
  }
  const Factor* event_word = contextFactor[pc]->GetFactor(factor);
  cur = &cur->tree[event_word ? event_word : EOS];
  if (cur->prob) {
    if (finalState) *finalState = cur->boState;
    if (len) *len = m_nGramOrder;
    return cur->prob;
  }
  cur->boState = *reinterpret_cast<const State*>(&m_curId);
  ++m_curId;

	float lmScore = 0;
	
	// OK, I really need a solution to store the ngram context factors in a
	// hashable form somewhere.  I guess the LM would be the natural place?
	// It is mandatory to patch the LM destructor code to clean up later!!!
	/*
	std::map<std::vector<const Word*>,float>::iterator it =	m_cachedNGrams.find(contextFactor);
	if (it != m_cachedNGrams.end())
	{
		lmScore = it->second;
	}
	else
	{
	*/
	if (true)
	{
	  std::ostringstream os;
	  os << "prob ";
	  if (event_word == NULL) {
		os << "</s>";
	  } else {
		os << event_word->GetString();
	  }
	  for (size_t i=1; i<max; i++) {
	        const Factor* f = contextFactor[count-1-i]->GetFactor(factor);
		if (f == NULL) {
			os << " <s>";
		} else {
			os << ' ' << f->GetString();
		}
	  }
	  os << std::endl;
	  std::string out = os.str();
	  write(sock, out.c_str(), out.size());
	  char res[6];
	  readNBytesForSure(sock, res, 6);
	  
	  /*
	  int r = read(sock, res, 6);
	  int errors = 0;
	  int cnt = 0;
	  while (1) {
	      if (r < 0) {
	        errors++; sleep(1);
	        //std::cerr << "Error: read()\n";
	        if (errors > 5) exit(1);
	        } else if (r==0 || res[cnt] == '\n') { break; }
	      else {
	        cnt += r;
	        if (cnt==6) break;
	        read(sock, &res[cnt], 6-cnt);
	      }
	  }
	  */
	  
	  lmScore = FloorScore(TransformSRIScore(*reinterpret_cast<float*>(res)));
	}
	
	cur->prob = lmScore;
	
  if (finalState) {
    *finalState = cur->boState;
    if (len) *len = m_nGramOrder;
  }
  return cur->prob;
}

void LanguageModelRemote::ScoreNGrams(const std::vector<std::vector<const Word*>* >& batchedNGrams)
{
	std::cout << "WE NEED TO OVERWRITE ScoreNGrams WITH A \"BATCHED\" VERSION!!!" << std::endl;
	for (size_t currPos = 0; currPos < batchedNGrams.size(); ++currPos)
	{
		// cfedermann: we should lookup ngrams that are already scored here!
		//             This will further optimize LM score computation.
		float lmScore = GetValue(*batchedNGrams[currPos]);
		m_cachedNGrams.insert(make_pair(batchedNGrams[currPos], lmScore));
	}
}

//===============================

bool NgramCmp::operator()(Ngram * k1, Ngram * k2) {
	int s1 = k1->tail->size();
	int s2 = k2->tail->size();
	
	if (s1 != s2) {
		return (s1 < s2);
	}
	
	int cmpRes = k1->head->compare(*(k2->head));
	
	if (cmpRes != 0) {
		return (cmpRes > 0);
	}
	
	for (int j = 0; j < s1; j++) {
		cmpRes = k1->tail->at(j)->compare(*(k2->tail->at(j)));
		
		if (cmpRes != 0) {
			return (cmpRes > 0);
		}
	}
	
	return false;
}

NgramSet * generateSetFromHypos(std::vector<Hypothesis*> * hypoBatch) {
	//TODO
}

#define COMMUNICATION_BATCH_SIZE 100

int serializeSet(NgramSet * set, NgramSet::iterator * it, int maxSize, std::string * tgt) {
	ostringstream os;
	
	os << "batch";
	
	int count = 0;
	
	for (; *it != set->end() and count < maxSize; *it++) {
		Ngram * currCtx = **it;
		Tail * currTail = currCtx->tail;
		
		os << ' ' << (currTail->size() + 1) << ' ' << *(currCtx->head);
		
		for (Tail::reverse_iterator it2 = currTail->rbegin(); it2 != currTail->rend(); it2++) {
			os << ' ' << **it2;
		}
		
		count++;
	}
	
	os << std::endl;
	
	*tgt = os.str();
	
	return count;
}

float * evaluateSerSet(int sock, std::string * serStr, int size) {
	write(sock, serStr->c_str(), serStr->size());
	
	int bufSize = size * sizeof(float);
	
	char * buf = (char *)malloc(bufSize);
	
	readNBytesForSure(sock, buf, bufSize);
	
	float * result = reinterpret_cast<float*>(buf);
	
	return result;
}

void placeResults(NgramSet::iterator * it, float * raw, int size) {
	int idx = 0;
	
	for (; idx < size; *it++) {
		(**it)->prob = FloorScore(TransformSRIScore(raw[idx]));
		idx++;
	}
}

void evaluateNgramSet(int sock, NgramSet * ngramSet) {
	NgramSet::iterator constructIt = ngramSet->begin();
	NgramSet::iterator fillIt = ngramSet->begin();
	
	while (constructIt != ngramSet->end()) {
		std::string * serStr = new std::string();
		int actualSize = serializeSet(ngramSet, &constructIt, COMMUNICATION_BATCH_SIZE, serStr);
		
		float * rawResults = evaluateSerSet(sock, serStr, actualSize);
		
		delete serStr;
		
		placeResults(&fillIt, rawResults, actualSize);
		
		free(rawResults);
	}
}

void LanguageModelRemote::ScoreHypoBatch(std::vector<Hypothesis*> * hypoBatch) {
	NgramSet * ngramSet = generateSetFromHypos(hypoBatch);
	
	evaluateNgramSet(sock, ngramSet);
	
	//TODO: score the hungry hungry hypos
}

//===============================

LanguageModelRemote::~LanguageModelRemote() {
  // Step 8 When finished send all lingering transmissions and close the connection
  close(sock); 
}

}
