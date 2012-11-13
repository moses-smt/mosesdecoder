from libcpp.string cimport string
from libcpp.vector cimport vector

cdef extern from 'Word.h' namespace 'OnDiskPt':
    cdef cppclass Word

cdef extern from 'Phrase.h' namespace 'OnDiskPt':
    cdef cppclass Phrase

cdef extern from 'SourcePhrase.h' namespace 'OnDiskPt':
    cdef cppclass SourcePhrase

#cdef extern from 'TargetPhrase.h' namespace 'OnDiskPt':
#    cdef cppclass TargetPhrase

#cdef extern from 'OnDiskWrapper.h' namespace 'OnDiskPt':   
#    cdef cppclass OnDiskWrapper

cdef extern from 'TargetPhraseCollection.h' namespace 'OnDiskPt':
    cdef cppclass TargetPhraseCollection
    
cdef extern from 'PhraseNode.h' namespace 'OnDiskPt':
    cdef cppclass PhraseNode
#    cdef cppclass PhraseNodePointer 'PhaseNode*'

cdef extern from 'OnDiskWrapper.h' namespace 'OnDiskPt':
    cdef cppclass OnDiskWrapper

cdef extern from 'PhraseNode.h' namespace 'OnDiskPt':
    cdef cppclass PhraseNode:
        PhraseNode* GetChild(Word& word, OnDiskWrapper& wrapper)
        TargetPhraseCollection* GetTargetPhraseCollection(unsigned tableLimit, OnDiskWrapper& wrapper)

cdef extern from 'OnDiskWrapper.h' namespace 'OnDiskPt':

    cdef cppclass OnDiskWrapper:
        OnDiskWrapper()
        bint BeginLoad(string& path)
        PhraseNode& GetRootSourceNode()

cdef extern from 'Util.h' namespace 'OnDiskPt':
#    cdef void Tokenize(Phrase& phrase, string& token, bint addSourceNonTerm, bint addTargetNonTerm, OnDiskWrapper& wrapper)
    cdef SourcePhrase Tokenize(vector[string]& tokens, OnDiskWrapper& wrapper)


