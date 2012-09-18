from libcpp.string cimport string
from libcpp.vector cimport vector
import os
import cython

cpdef int fsign(float x):
    '''Simply returns the sign of float x (zero is assumed +), it's defined here just so one gains a little bit with static typing'''
    return 1 if x >= 0 else -1

cdef bytes as_str(data):
    if isinstance(data, bytes):
        return data
    elif isinstance(data, unicode):
        return data.encode('UTF-8')
    raise TypeError('Cannot convert %s to string' % type(data))

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
        '''Requires a tuple of words (as strings) and a tuple of scores (as floats).
        Word-alignment info (as string) may be provided'''
        self._words = words
        self._scores = scores
        self._wa = wa

    @property
    def words(self):
        '''Tuple of words (as strings)'''
        return self._words

    @property
    def scores(self):
        '''Tuple of scores (as floats)'''
        return self._scores

    @property
    def wa(self):
        '''Word-alignment info (as string)'''
        return self._wa

    @staticmethod
    def desc(x, y, keys = lambda r: r.scores[0]):
        '''Returns the sign of keys(y) - keys(x).
        Can only be used if scores is not an empty vector as
        keys defaults to scores[0]'''
        return fsign(keys(y) - keys(x))

    def __str__(self):
        '''Returns a string such as: <words> ||| <scores> [||| word-alignment info]'''
        if self._wa:
            return ' ||| '.join( (' '.join(self._words),
                ' '.join([str(x) for x in self._scores]),
                self._wa) )
        else:
            return ' ||| '.join( (' '.join(self._words),
                ' '.join([str(x) for x in self._scores]) ) )

    def __repr__(self):
        return repr((repr(self._words), repr(self._scores), repr(self._wa)))

cdef QueryResult get_query_result(StringTgtCand& cand, object wa = None):
    '''Converts a StringTgtCandidate (c++ object) and possibly a word-alignment info (string)
    to a QueryResult (python object).'''
    cdef tuple words = tuple([cand.first[i].c_str() for i in range(cand.first.size())])
    cdef tuple scores = tuple([cand.second[i] for i in range(cand.second.size())])
    return QueryResult(words, scores, wa)

cdef class BinaryPhraseTable(object):
    '''This class encapsulates a Moses::PhraseDictionaryTree for operations over
    binary phrase tables.'''

    cdef PhraseDictionaryTree* __tree
    cdef bytes _path
    cdef unsigned _nscores
    cdef bint _wa
    cdef bytes _delimiters

    def __cinit__(self, bytes path, unsigned nscores = 5, bint wa = False, delimiters = ' \t'):
        '''It requies a path to binary phrase table (stem of the table, e.g europarl.fr-en 
        is the stem for europar.fr-en.binphr.*).
        Moses::PhraseDictionaryTree also needs to be aware of the number of scores (usually 5),
        and whether or not there is word-alignment info in the table (usually not).
        One can also specify the token delimiters, for Moses::Tokenize(text, delimiters), which is space or tab by default.'''

        if not BinaryPhraseTable.isValidBinaryTable(path, wa):
            raise ValueError, "'%s' doesn't seem a valid binary table." % path
        self._path = path
        self._nscores = nscores
        self._wa = wa
        self._delimiters = delimiters
        self.__tree = new PhraseDictionaryTree(nscores)
        self.__tree.UseWordAlignment(wa)
        self.__tree.Read(string(path))

    def __dealloc__(self):
        del self.__tree

    @staticmethod
    def isValidBinaryTable(stem, bint wa = False):
        '''This sanity check was added to the constructor, but you can access it from outside this class
        to determine whether or not you are providing a valid stem to BinaryPhraseTable.'''
        if wa:
            return os.path.isfile(stem + ".binphr.idx") \
                and os.path.isfile(stem + ".binphr.srctree.wa") \
                and os.path.isfile(stem + ".binphr.srcvoc") \
                and os.path.isfile(stem + ".binphr.tgtdata.wa") \
                and os.path.isfile(stem + ".binphr.tgtvoc")
        else:
            return os.path.isfile(stem + ".binphr.idx") \
                and os.path.isfile(stem + ".binphr.srctree") \
                and os.path.isfile(stem + ".binphr.srcvoc") \
                and os.path.isfile(stem + ".binphr.tgtdata") \
                and os.path.isfile(stem + ".binphr.tgtvoc")

    @property
    def path(self):
        return self._path

    @property
    def nscores(self):
        return self._nscores

    @property
    def wa(self):
        return self._wa

    @property
    def delimiters(self):
        return self._delimiters

    def query(self, line, cmp = None, top = 0):
        '''Queries the phrase table and returns a list of matches.
        Each match is a QueryResult.
        If 'cmp' is defined the return list is sorted.
        If 'top' is defined, onlye the top elements will be returned.'''
        cdef bytes text = as_str(line)
        cdef vector[string] fphrase = Tokenize(string(text), string(self._delimiters))
        cdef vector[StringTgtCand]* rv = new vector[StringTgtCand]()
        cdef vector[string]* wa = NULL
        cdef list phrases
        if not self.__tree.UseWordAlignment():
            self.__tree.GetTargetCandidates(fphrase, rv[0])
            phrases = [get_query_result(rv[0][i]) for i in range(rv.size())]
        else:
            wa = new vector[string]()
            self.__tree.GetTargetCandidates(fphrase, rv[0], wa[0])
            phrases = [get_query_result(rv[0][i], wa[0][i].c_str()) for i in range(rv.size())]
            del wa
        del rv
        if cmp:
            phrases.sort(cmp=cmp)
        if top > 0:
            return phrases[0:top]
        else:  
            return phrases
        
