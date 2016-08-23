#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include "Remote.h"
#include "moses/Factor.h"
#include "util/string_stream.hh"

#if !defined(_WIN32) && !defined(_WIN64)
#include <arpa/inet.h>
#endif

namespace Moses
{

const Factor* LanguageModelRemote::BOS = NULL;
const Factor* LanguageModelRemote::EOS = (LanguageModelRemote::BOS + 1);

bool LanguageModelRemote::Load(const std::string &filePath
                               , FactorType factorType
                               , size_t nGramOrder)
{
  m_factorType    = factorType;
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


bool LanguageModelRemote::start(const std::string& host, int port)
{
  //std::cerr << "host = " << host << ", port = " << port << "\n";
  sock = socket(AF_INET, SOCK_STREAM, 0);
  hp = gethostbyname(host.c_str());
  if (hp==NULL) {
#if defined(_WIN32) || defined(_WIN64)
    fprintf(stderr, "gethostbyname failed\n");
#else
    herror("gethostbyname failed");
#endif
    exit(1);
  }

  memset(&server, '\0', sizeof(server));
  memcpy((char *)&server.sin_addr, hp->h_addr, hp->h_length);
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

LMResult LanguageModelRemote::GetValue(const std::vector<const Word*> &contextFactor, State* finalState) const
{
  LMResult ret;
  ret.unknown = false;
  size_t count = contextFactor.size();
  if (count == 0) {
    if (finalState) *finalState = NULL;
    ret.score = 0.0;
    return ret;
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
    ret.score = cur->prob;
    return ret;
  }
  cur->boState = *reinterpret_cast<const State*>(&m_curId);
  ++m_curId;

  util::StringStream os;
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
  os << "\n";
  write(sock, os.str().c_str(), os.str().size());
  char res[6];
  int r = read(sock, res, 6);
  int errors = 0;
  int cnt = 0;
  while (1) {
    if (r < 0) {
      errors++;
      sleep(1);
      //std::cerr << "Error: read()\n";
      if (errors > 5) exit(1);
    } else if (r==0 || res[cnt] == '\n') {
      break;
    } else {
      cnt += r;
      if (cnt==6) break;
      read(sock, &res[cnt], 6-cnt);
    }
  }
  cur->prob = FloorScore(TransformLMScore(*reinterpret_cast<float*>(res)));
  if (finalState) {
    *finalState = cur->boState;
  }
  ret.score = cur->prob;
  return ret;
}

LanguageModelRemote::~LanguageModelRemote()
{
  // Step 8 When finished send all lingering transmissions and close the connection
  close(sock);
}

}
