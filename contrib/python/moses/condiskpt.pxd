from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp.pair cimport pair
from cdictree cimport Scores 

cdef extern from 'Vocab.h' namespace 'OnDiskPt':
    cdef cppclass Vocab:
        Vocab()

cdef extern from 'Word.h' namespace 'OnDiskPt':
    cdef cppclass Word:
        Word()
        string &GetString(Vocab vocab)

cdef extern from 'Phrase.h' namespace 'OnDiskPt':
    cdef cppclass Phrase:
        Phrase()
        unsigned GetSize()
        Word &GetWord(unsigned pos)

cdef extern from 'SourcePhrase.h' namespace 'OnDiskPt':
    cdef cppclass SourcePhrase:
        SourcePhrase()
        unsigned GetSize()
        Word &GetWord(unsigned pos)

cdef extern from 'TargetPhrase.h' namespace 'OnDiskPt':
  
    ctypedef pair[int,int] AlignPair
    ctypedef vector[AlignPair] AlignType
    cdef cppclass TargetPhrase:
        TargetPhrase()
        unsigned GetSize()
        Word &GetWord(unsigned pos)
        AlignType &GetAlign()
        Scores &GetScores()


cdef extern from 'TargetPhraseCollection.h' namespace 'OnDiskPt':
    cdef cppclass TargetPhraseCollection:
        TargetPhraseCollection()
        TargetPhrase &GetTargetPhrase(unsigned index)
        unsigned GetSize()
        string GetDebugStr()

cdef extern from 'OnDiskWrapper.h' namespace 'OnDiskPt':
    cdef cppclass OnDiskWrapper

cdef extern from 'PhraseNode.h' namespace 'OnDiskPt':
    cdef cppclass PhraseNode:
        PhraseNode()
        PhraseNode* GetChild(Word &word, OnDiskWrapper &wrapper)
        TargetPhraseCollection* GetTargetPhraseCollection(unsigned tableLimit, OnDiskWrapper &wrapper)
    ctypedef PhraseNode* ConstPhraseNodePointer 'const PhraseNode*'

cdef extern from 'OnDiskWrapper.h' namespace 'OnDiskPt':

    cdef cppclass OnDiskWrapper:
        OnDiskWrapper()
        bint BeginLoad(string& path)
        PhraseNode& GetRootSourceNode()
        Vocab& GetVocab()

cdef extern from 'OnDiskQuery.h' namespace 'OnDiskPt':
    cdef cppclass OnDiskQuery:
        OnDiskQuery(OnDiskWrapper &wrapper)
        SourcePhrase Tokenize(vector[string]& tokens)
        PhraseNode* Query(SourcePhrase& sourcePhrase)
        PhraseNode* Query(vector[string]& tokens)

