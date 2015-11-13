#include "Prob.h"
#include "Ngram.h"
#include "Vocab.h"

#include <sstream>
#include <string>
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <map>

struct Cache {
  map<int, Cache> tree;
  float prob;
  Cache() : prob(0) {}
};

struct LMClient {
  Vocab* voc;
  int sock, port;
  char *s;
  struct hostent *hp;
  struct sockaddr_in server;
  char res[8];

  LMClient(Vocab* v, const char* host) : voc(v), port(6666) {
    s = strchr(host, ':');

    if (s != NULL) {
	    *s = '\0';
	    s+=1;
	    port = atoi(s);
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);

    hp = gethostbyname(host);
    if (hp == NULL) {
	    fprintf(stderr, "unknown host %s\n", host);
	    exit(1);
    }

    memset(&server, '\0', sizeof(server));
    memcpy((char *)&server.sin_addr, hp->h_addr, hp->h_length);
    server.sin_family = hp->h_addrtype;
    server.sin_port = htons(port);

    int errors = 0;
    while (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
      cerr << "Error: connect()\n";
      sleep(1);
      errors++;
      if (errors > 5) exit(1);
    }
    std::cerr << "Connected to LM on " << host << " on port " << port << std::endl;
  }
  float wordProb(int word, int* context) {
    Cache* cur = &cache;
    int i = 0;
    while (context[i] > 0) {
      cur = &cur->tree[context[i++]];
    }
    cur = &cur->tree[word];
    if (cur->prob) { return cur->prob; }

    i = 0;
    ostringstream os;
    os << "prob " << voc->getWord((VocabIndex)word);
    while (context[i] > 0) {
      os << ' ' << voc->getWord((VocabIndex)context[i++]);
    }
    os << endl;
    string out = os.str();
    write(sock, out.c_str(), out.size());
    int r = read(sock, res, 6);
    int errors = 0;
    int cnt = 0;
    while (1) {
      if (r < 0) {
        errors++; sleep(1);
	cerr << "Error: read()\n";
	if (errors > 5) exit(1);
      } else if (r==0 || res[cnt] == '\n') { break; }
      else {
        cnt += r;
	if (cnt==6) break;
	read(sock, &res[cnt], 6-cnt);
      }
    }
    cur->prob = *reinterpret_cast<float*>(res);
    return cur->prob;
  }
  void clear() {
    cache.tree.clear();
  }
  Cache cache;
};

