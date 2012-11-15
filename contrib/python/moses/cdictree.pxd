from libcpp.string cimport string
from libcpp.vector cimport vector

ctypedef string* str_pointer
ctypedef string* const_str_pointer "const str_pointer"
ctypedef vector[const_str_pointer] Tokens
ctypedef float FValue
ctypedef vector[FValue] Scores

cdef extern from 'PhraseDictionaryTree.h' namespace 'Moses':

    cdef struct StringTgtCand:
        Tokens tokens
        Scores scores
        Tokens fnames
        Scores fvalues

    cdef cppclass PhraseDictionaryTree:
        PhraseDictionaryTree(unsigned nscores)
        void NeedAlignmentInfo(bint value)
        void PrintWordAlignment(bint value)
        bint PrintWordAlignment()
        int Read(string& path)
        void GetTargetCandidates(vector[string]& fs, 
                vector[StringTgtCand]& rv)
        void GetTargetCandidates(vector[string]& fs, 
                vector[StringTgtCand]& rv,
                vector[string]& wa)

cdef extern from 'Util.h' namespace 'Moses':
    cdef vector[string] Tokenize(string& text, string& delimiters)

