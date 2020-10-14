#include <msclr/marshal_cppstd.h>
#include "legacy\Parameter.h"
#include "System.h"

using namespace System;
using namespace msclr::interop;

// A wrapper around Faiss that lets you build indexes
// Right now just proof-of-concept code to makes sure it all works from C#,
// eventually may want to rework the interface, or possibly look at extending
// FaissSharp to support the windows dll 

namespace Moses {

    public ref class System
    {

    public:
        
    };

    public ref class Parameter
    {
    public:
        Parameter() { m_pWrapper = new Moses2::Parameter(); }
        ~Parameter() { this->!Parameter(); }

        

    private:
        // Review: I'm not using e.g. unique_ptr here because I don't know the lifetime  
        // semantics behind ref classes. 
        Moses2::Parameter* m_pWrapper;
    };

}
