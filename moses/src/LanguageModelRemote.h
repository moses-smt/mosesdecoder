#ifndef moses_LanguageModelRemote_h
#define moses_LanguageModelRemote_h

#include "LanguageModelSingleFactor.h"
#include "TypeDef.h"
#include "Factor.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

namespace Moses
{

class LanguageModelRemote : public LanguageModelSingleFactor {
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
		LanguageModelRemote(bool registerScore, ScoreIndexManager &scoreIndexManager);
		~LanguageModelRemote();
		void ClearSentenceCache() { m_cache.tree.clear(); m_curId = 1000; }
		virtual float GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0, unsigned int* len = 0) const;
        	bool Load(const std::string &filePath
                                        , FactorType factorType
                                        , float weight
                                        , size_t nGramOrder);
};

}
#endif
