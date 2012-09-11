from libcpp.string cimport string
from libcpp.vector cimport vector

cdef class QueryResult(object):
    '''This class represents a query result, that is,
    a target phrase (tuple of words/strings),
    a feature vector (tuple of floats)
    and possibly an alignment info (string).
    Here we don't bother parsing the alignment info, as it's often only
    used as is, threfore saving some time.'''

    cdef tuple _words
    cdef tuple _scores
    cdef bytes _wa

    def __cinit__(self, words, scores, wa = None):
        self._words = words
        self._scores = scores
        self._wa = wa

    @property
    def words(self):
        return self._words

    @property
    def scores(self):
        return self._scores

    @property
    def wa(self):
        return self._wa

    def __str__(self):
        if self._wa:
            return ' ||| '.join( (' '.join(self._words),
                ' '.join([str(x) for x in self._scores]),
                self._wa) )
        else:
            return ' ||| '.join( (' '.join(self._words),
                ' '.join([str(x) for x in self._scores]) ) )

    def __repr__(self):
        return repr((repr(self._words), repr(self._scores), repr(self._wa)))

cdef bytes as_str(data):
    if isinstance(data, bytes):
        return data
    elif isinstance(data, unicode):
        return data.encode('UTF-8')
    raise TypeError('Cannot convert %s to string' % type(data))


cdef QueryResult get_query_result(StringTgtCand& cand, wa = None):
    '''Converts a StringTgtCandidate (c++ object) to a tuple (python object).
    The tuple contains a tuple of words at the first position
    and a tuple of scores at the second position.'''
    cdef tuple words = tuple([cand.first[i].c_str() for i in range(cand.first.size())])
    cdef tuple scores = tuple([cand.second[i] for i in range(cand.second.size())])
    return QueryResult(words, scores, wa)

cdef class PhraseTable:
    '''This class encapsulates a Moses::PhraseDictionaryTree for operations over
    binary phrase tables.'''

    cdef PhraseDictionaryTree* tree

    def __cinit__(self, char* path, unsigned nscores = 5, bint wa = False):
        self.tree = new PhraseDictionaryTree(nscores)
        self.tree.UseWordAlignment(wa)
        self.tree.Read(path)

    def __dealloc__(self):
        del self.tree

    def query(self, char* line):
        '''Queries the phrase table and returns a list of matches.
        Each match is a QueryResult.'''
        cdef bytes text = as_str(line)
        cdef vector[string] fphrase = Tokenize(string(text), string(' '))
        cdef vector[StringTgtCand]* rv = new vector[StringTgtCand]()
        cdef vector[string]* wa
        cdef list phrases
        if not self.tree.UseWordAlignment():
            self.tree.GetTargetCandidates(fphrase, rv[0])
            phrases = [get_query_result(rv[0][i]) for i in range(rv.size())]
        else:
            wa = new vector[string]()
            self.tree.GetTargetCandidates(fphrase, rv[0], wa[0])
            phrases = [get_query_result(rv[0][i], wa[0][i].c_str()) for i in range(rv.size())]
            del wa
        del rv
        return phrases
        
