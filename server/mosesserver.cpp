#include <cassert>
#include <stdexcept>
#include <iostream>

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

#include "Hypothesis.h"
#include "Manager.h"
#include "StaticData.h"


using namespace Moses;
using namespace std;


class Translator : public xmlrpc_c::method {
public:
    Translator() {
        // signature and help strings are documentation -- the client
        // can query this information with a system.methodSignature and
        // system.methodHelp RPC.
        this->_signature = "s:s";
        this->_help = "Does translation";
    }
    void
    execute(xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP) {
        
        const string  source(paramList.getString(0));
        cerr << "Input: " << source << endl;
        paramList.verifyEnd(1);

        /*
        const StaticData &staticData = StaticData::Instance();
        //TODO Cleanup without messing with caches
        staticData.CleanUpAfterSentenceProcessing();

        Sentence sentence(Input);
        stringstream in(source + "\n");
        const vector<FactorType> &inputFactorOrder = 
            staticData.GetInputFactorOrder();
        sentence.Read(in,inputFactorOrder);
        //FIXME: global ops
        staticData.ResetSentenceStats(sentence);
        staticData.InitializeBeforeSentenceProcessing(sentence); 
        auto_ptr<TranslationOptionCollection>
            toc(sentence.CreateTranslationOptionCollection());
        const vector<DecodeGraph*>& decodeStepVL =
            staticData.GetDecodeStepVL();
        toc->CreateTranslationOptions(decodeStepVL);
        auto_ptr<Search> searcher(new SearchNormal(sentence,*toc));
        searcher->ProcessSentence();
        const Hypothesis* hypo = searcher->GetBestHypothesis();
        */
        const StaticData &staticData = StaticData::Instance();
        Sentence sentence(Input);
        const vector<FactorType> &inputFactorOrder = 
            staticData.GetInputFactorOrder();
        stringstream in(source + "\n");
        sentence.Read(in,inputFactorOrder);
        Manager manager(sentence,staticData.GetSearchAlgorithm());
        manager.ProcessSentence();
        const Hypothesis* hypo = manager.GetBestHypothesis();

        stringstream out;
        outputHypo(out,hypo);

        *retvalP = xmlrpc_c::value_string(out.str());

    }

    void outputHypo(ostream& out, const Hypothesis* hypo) {
        if (hypo != NULL) {
            outputHypo(out,hypo->GetPrevHypo());
            TargetPhrase p = hypo->GetTargetPhrase();
            for (size_t pos = 0 ; pos < p.GetSize() ; pos++)
            {
                const Factor *factor = p.GetFactor(pos, 0);
                out << *factor << " ";

            }
        }
    }
};


int main(int argc, char** argv) {
    try {

        //Extract port and log, send other args to moses
        char** mosesargv = new char*[argc];
        int mosesargc = 0;
        int port = 8080;
        const char* logfile = "/dev/null";

        for (int i = 0; i < argc; ++i) {
            if (!strcmp(argv[i],"--server-port")) {
                ++i;
                if (i >= argc) {
                    cerr << "Error: Missing argument to --server-port" << endl;
                    exit(1);
                } else {
                    port = atoi(argv[i]);
                }
            } else if (!strcmp(argv[i],"--server-log")) {
                ++i;
                if (i >= argc) {
                    cerr << "Error: Missing argument to --server-log" << endl;
                    exit(1);
                } else {
                    logfile = argv[i];
                }
            } else {
                mosesargv[i] = new char[strlen(argv[i])+1];
                strcpy(mosesargv[i],argv[i]);
                ++mosesargc;
            }
        }

        Parameter* params = new Parameter();
        if (!params->LoadParam(mosesargc,mosesargv)) {
            params->Explain();
            exit(1);
        }
        if (!StaticData::LoadDataStatic(params)) {
            exit(1);
        }
       
    
        xmlrpc_c::registry myRegistry;

        xmlrpc_c::methodPtr const translator(new Translator);

        myRegistry.addMethod("translate", translator);
        
        xmlrpc_c::serverAbyss myAbyssServer(
            myRegistry,
            port,              // TCP port on which to listen
            logfile  
            );
        
        cerr << "Listening on port " << port << endl;
        myAbyssServer.run();
        // xmlrpc_c::serverAbyss.run() never returns
        assert(false);
    } catch (exception const& e) {
        cerr << "Something failed.  " << e.what() << endl;
    }
    return 0;
}
