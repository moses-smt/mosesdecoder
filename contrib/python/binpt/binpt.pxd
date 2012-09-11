from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp.pair cimport pair

ctypedef string* str_pointer

cdef extern from 'TypeDef.h' namespace 'Moses':
    ctypedef vector[float] Scores
    ctypedef pair[vector[str_pointer], Scores] StringTgtCand

cdef extern from 'PhraseDictionaryTree.h' namespace 'Moses':
    cdef cppclass PhraseDictionaryTree:
        PhraseDictionaryTree(unsigned nscores)
        void UseWordAlignment(bint use)
        bint UseWordAlignment()
        int Read(char* path)
        void GetTargetCandidates(vector[string]& fs, 
                vector[StringTgtCand]& rv)
        void GetTargetCandidates(vector[string]& fs, 
                vector[StringTgtCand]& rv,
                vector[string]& wa)

cdef extern from 'Util.h' namespace 'Moses':
    cdef vector[string] Tokenize(string& text, string& delimiters)

