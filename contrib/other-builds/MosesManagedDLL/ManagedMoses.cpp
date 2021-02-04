#include <msclr/marshal_cppstd.h>
#include "Moses2Wrapper.h"

using namespace System;
using namespace msclr::interop;

//TODO: include headers as per the build process
namespace Moses {
    public ref class Moses2Wrapper
    {
        public:
            Moses2Wrapper(String^ filePath) { 
                const std::string standardString = marshal_as<std::string>(filePath);
                m_pWrapper = new Moses2::Moses2Wrapper(standardString); 
            }
           ~Moses2Wrapper() { this->!Moses2Wrapper(); }
           String^ Translate(String^ input, long requestId) {
               const std::string standardString = marshal_as<std::string>(input);
               std::string output = m_pWrapper->Translate(standardString, requestId);
               String^ str = gcnew String(output.c_str());
               return str;
           }
        protected:
            !Moses2Wrapper() { delete m_pWrapper; m_pWrapper = nullptr; }
        private:
            Moses2::Moses2Wrapper *m_pWrapper;
    };
}