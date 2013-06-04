#ifndef moses_LanguageModelRemote_h
#define moses_LanguageModelRemote_h

#include "SingleFactor.h"
#include "moses/TypeDef.h"
#include "moses/Factor.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

namespace Moses
{

/** @todo ask miles
 */
class LanguageModelRemote : public LanguageModelSingleFactor
{
private:
  struct Cache {
    std::map<const Factor*, Cache> tree;
    float prob;
    State boState;
    Cache() : prob(0) {}
  };

  int sock, port;
  struct hostent *hp;
  struct sockaddr_in server;
  mutable size_t m_curId;
  mutable Cache m_cache;
  bool start(const std::string& host, int port);
  static const Factor* BOS;
  static const Factor* EOS;
public:
  ~LanguageModelRemote();
  void ClearSentenceCache() {
    m_cache.tree.clear();
    m_curId = 1000;
  }
  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const;
  bool Load(const std::string &filePath
            , FactorType factorType
            , size_t nGramOrder);
};

}
#endif
