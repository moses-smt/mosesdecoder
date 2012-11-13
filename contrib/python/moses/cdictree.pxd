from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp.pair cimport pair

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
        void UseWordAlignment(bint use)
        bint UseWordAlignment()
        int Read(string& path)
        void GetTargetCandidates(vector[string]& fs, 
                vector[StringTgtCand]& rv)
        void GetTargetCandidates(vector[string]& fs, 
                vector[StringTgtCand]& rv,
                vector[string]& wa)

cdef extern from 'Util.h' namespace 'Moses':
    cdef vector[string] Tokenize(string& text, string& delimiters)

