#define NOMINMAX // Windows max macro collides with std::
#include <Windows.h>
#include <memory>
#include <vcclr.h>
#include <msclr/marshal_cppstd.h>
#include "Moses2Wrapper.h"

using namespace System;
using namespace msclr::interop;


namespace Moses {
    public ref class Moses2Wrapper
    {
    public:
        Moses2Wrapper(String^ filePath) { 
            const std::string standardString = marshal_as<std::string>(filePath);
            m_pWrapper = new Moses2::Moses2Wrapper(standardString); 
        }
       ~Moses2Wrapper() { this->!Moses2Wrapper(); }
       String^ Translate(String^ input) {
           const std::string standardString = marshal_as<std::string>(input);
           std::string output = m_pWrapper->Translate(standardString);
           //Console::WriteLine(output);
           String^ str = gcnew String(output.c_str());
           return str;
       }
        
    protected:
        !Moses2Wrapper() { delete m_pWrapper; m_pWrapper = nullptr; }
    private:
        Moses2::Moses2Wrapper *m_pWrapper;
    };
}
/*
public class ManagedMoses
{
    Moses2::Moses2Wrapper *m_Instance;
public:
    ManagedMoses(String^ filepath) {
        const std::string standardString = marshal_as<std::string>(filepath);
        m_Instance = new Moses2::Moses2Wrapper(standardString);
        
    }
    String^ Translate(String^ input){
        const std::string standardString = marshal_as<std::string>(input);
        std::string output = m_Instance->Translate(standardString);
        //Console::WriteLine(output);
        String^ str = gcnew String(output.c_str());
        return str;
    }
};


/*
#include <winsock2.h>
#ifndef WIN32
#define WIN32
#endif
#include <msclr/marshal_cppstd.h>
#include "legacy/Parameter.h"
#include "System.h"

using namespace System;
using namespace msclr::interop;

// A wrapper around Faiss that lets you build indexes
// Right now just proof-of-concept code to makes sure it all works from C#,
// eventually may want to rework the interface, or possibly look at extending
// FaissSharp to support the windows dll 

namespace Moses {



    public ref class Parameter
    {
    public:
        Parameter() { m_pWrapper = new Moses2::Parameter(); }
        ~Parameter() { this->!Parameter(); }
        bool LoadParams(String^ filePath) {
            const std::string standardString = marshal_as<std::string>(filePath);
            auto flag = m_pWrapper->LoadParam(standardString);
            return bool(flag);
        }
        Parameter* GetInstance()
        {
            return m_pWrapper;
        }
    protected:
        !Parameter() { delete m_pWrapper; m_pWrapper = nullptr; }
    private:
        Moses2::Parameter* m_pWrapper;
    };


    public ref class System {
    public:
        System(const Parameter^ paramsArg) {
            new Moses2::System(paramsArg->GetInstance());
        }
        ~System() { this->!System(); }
    protected:
        !System() { delete m_sWrapper; m_sWrapper = nullptr; }
    private:
        Moses2::System* m_sWrapper;
        Moses2::Parameter* paramArgs;
    };





}

*/