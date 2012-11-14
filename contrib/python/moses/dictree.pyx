# This module wraps phrase/rule tables

from libcpp.string cimport string
from libcpp.vector cimport vector
from itertools import chain
import os
import cython
cimport cdictree
cimport condiskpt
from math import log

cpdef int fsign(float x):
    """Simply returns the sign of float x (zero is assumed +), it's defined here just so one gains a little bit with static typing"""
    return 1 if x >= 0 else -1

cdef bytes as_str(data):
    if isinstance(data, bytes):
        return data
    elif isinstance(data, unicode):
        return data.encode('UTF-8')
    raise TypeError('Cannot convert %s to string' % type(data))

cdef class QueryResult(object):
    """This class represents a query result, that is, the target rule.
    It is made of a lhs (string), rhs (tuple of strings), alginment information (tuple of pairs of 0-based integers) and
    a tuple of real-valued scores.
    """

    cdef readonly bytes lhs
    cdef readonly tuple rhs
    cdef readonly tuple alignment
    cdef readonly tuple scores

    def __cinit__(self, rhs, scores, alignment = [], lhs = None):
        """
        :rhs right-hand side tokens (sequence of terminals and nonterminals)
        :scores tuple of real-valued scores
        :alignment tuple of pairs of 0-based integers
        :lhs left-hand side nonterminal (None in phrase-based)
        """
        self.lhs = lhs
        self.rhs = tuple(rhs)
        self.scores = tuple(scores)
        self.alignment = tuple(alignment)

    @staticmethod
    def desc(x, y, key = lambda r: r.scores[0]):
        """Returns the sign of key(y) - key(x).
        Can only be used if scores is not an empty vector as
        keys defaults to scores[0]"""
        return fsign(key(y) - key(x))

    def __str__(self):
        """Returns a string such as: <words> ||| <scores> [||| word-alignment info]"""
        if self.lhs:
            lhs = [self.lhs]
        else:
            lhs = []
        return ' ||| '.join((' '.join(chain(self.rhs, lhs)),
            ' '.join(str(x) for x in self.scores),
            ' '.join('%d-%d' % (s, t) for s, t in self.alignment)))

    def __repr__(self):
        return repr((repr(self.rhs), repr(self.lhs), repr(self.scores), repr(self.alignment)))

cdef class DictionaryTree(object):

    @classmethod
    def canLoad(cls, path, bint wa = False):
        """Whether or not the path represents a valid table for that class."""
        raise NotImplementedError

    def query(self, line, top = 0, converter = None, cmp = None, key = None):
        """
        :line query (string)
        :top table limit (int)
        :converter applies a transformation to the score (function)
        :cmp define it to get a sorted list
        :key defines the key of the comparison
        """
        raise NotImplementedError

cdef class PhraseDictionaryTree(DictionaryTree):
    """This class encapsulates a Moses::PhraseDictionaryTree for operations over
    binary phrase tables."""

    cdef cdictree.PhraseDictionaryTree* __tree
    cdef readonly bytes path
    cdef readonly unsigned nscores
    cdef readonly bint wa
    cdef readonly bytes delimiters

    def __cinit__(self, bytes path, unsigned nscores = 5, bint wa = False, delimiters = ' \t'):
        """It requies a path to binary phrase table (stem of the table, e.g europarl.fr-en 
        is the stem for europar.fr-en.binphr.*).
        Moses::PhraseDictionaryTree also needs to be aware of the number of scores (usually 5),
        and whether or not there is word-alignment info in the table (usually not).
        One can also specify the token delimiters, for Moses::Tokenize(text, delimiters), which is space or tab by default."""

        if not PhraseDictionaryTree.canLoad(path, wa):
            raise ValueError, "'%s' doesn't seem a valid binary table." % path
        self.path = path
        self.nscores = nscores
        self.wa = wa
        self.delimiters = delimiters
        self.__tree = new cdictree.PhraseDictionaryTree(nscores)
        self.__tree.UseWordAlignment(wa)
        self.__tree.Read(path)

    def __dealloc__(self):
        del self.__tree

    @classmethod
    def canLoad(cls, stem, bint wa = False):
        """This sanity check was added to the constructor, but you can access it from outside this class
        to determine whether or not you are providing a valid stem to BinaryPhraseTable."""
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
    
    cdef QueryResult getQueryResult(self, cdictree.StringTgtCand& cand, wa = None, converter = None):
        """Converts a StringTgtCandidate (c++ object) and possibly a word-alignment info (string) to a QueryResult (python object)."""
        cdef list words = [cand.tokens[i].c_str() for i in xrange(cand.tokens.size())]
        cdef list scores = [score for score in cand.scores] if converter is None else [converter(score) for score in cand.scores]
        if wa is None:
            return QueryResult(words, scores)
        else:
            alignments = []
            for point in wa.split():
              s, t = point.split('-')
              alignments.append((int(s), int(t)))
            return QueryResult(words, scores, alignments)

    def query(self, line, top = 0, converter = lambda x: log(x), cmp = None, key = None):
        """Queries the phrase table and returns a list of matches.
        Each match is a QueryResult.
        If 'cmp' is defined the return list is sorted.
        If 'top' is defined, only the top elements will be returned."""
        cdef bytes text = as_str(line)
        cdef vector[string] fphrase = cdictree.Tokenize(text, self.delimiters)
        cdef vector[cdictree.StringTgtCand]* rv = new vector[cdictree.StringTgtCand]()
        cdef vector[string]* wa = NULL
        cdef list phrases
        if not self.__tree.UseWordAlignment():
            self.__tree.GetTargetCandidates(fphrase, rv[0])
            phrases = [self.getQueryResult(rv[0][i], converter) for i in range(rv.size())]
        else:
            wa = new vector[string]()
            self.__tree.GetTargetCandidates(fphrase, rv[0], wa[0])
            phrases = [self.getQueryResult(rv[0][i], wa[0][i].c_str(), converter) for i in range(rv.size())]
            del wa
        del rv
        if cmp:
            phrases.sort(cmp=cmp, key=key)
        if top > 0:
            return phrases[0:top]
        else:  
            return phrases
    
cdef class OnDiskWrapper(DictionaryTree):

    cdef condiskpt.OnDiskWrapper *wrapper
    cdef condiskpt.OnDiskQuery *finder
    cdef readonly bytes delimiters
    cdef readonly unsigned tableLimit

    def __cinit__(self, bytes path, unsigned tableLimit, delimiters = ' \t'):
        self.delimiters = delimiters
        self.tableLimit = tableLimit
        self.wrapper = new condiskpt.OnDiskWrapper()
        self.wrapper.BeginLoad(string(path))
        self.finder = new condiskpt.OnDiskQuery(self.wrapper[0])
    
    @classmethod
    def canLoad(cls, stem, bint wa = False):
        return os.path.isfile(stem + "/Misc.dat") \
            and os.path.isfile(stem + "/Source.dat") \
            and os.path.isfile(stem + "/TargetColl.dat") \
            and os.path.isfile(stem + "/TargetInd.dat") \
            and os.path.isfile(stem + "/Vocab.dat")

    def query(self, line, top = 0, converter = None, cmp = None, key = None):
        cdef bytes text = as_str(line)
        cdef vector[string] ftokens = cdictree.Tokenize(text, self.delimiters)
        cdef condiskpt.PhraseNode *node = <condiskpt.PhraseNode *>self.finder.Query(ftokens)
        if node == NULL:
            return []
        cdef condiskpt.TargetPhraseCollection ephrases = node.GetTargetPhraseCollection(self.tableLimit, self.wrapper[0])[0]
        cdef condiskpt.Vocab vocab = self.wrapper.GetVocab() 
        cdef condiskpt.TargetPhrase ephr
        cdef condiskpt.Word e
        cdef unsigned i, j
        cdef list results = [None for _ in xrange(ephrases.GetSize())]
        for i in xrange(ephrases.GetSize()):
            ephr = ephrases.GetTargetPhrase(i)
            words = [ephr.GetWord(j).GetString(vocab).c_str() for j in xrange(ephr.GetSize())]
            if converter is None:
                results[i] = QueryResult(words[:-1], ephr.GetScores(), ephr.GetAlign(), words[-1])
            else:
                scores = tuple(ephr.GetScores())
                results[i] = QueryResult(words[:-1], (converter(score) for score in scores), ephr.GetAlign(), words[-1])
        if cmp:
            results.sort(cmp=cmp, key=key)
        if top > 0:
            return results[0:top]
        else:  
            return results
    
def load(path, nscores):
    """Finds out the correct implementation depending on the content of 'path' and returns the appropriate dictionary tree."""
    if PhraseDictionaryTree.canLoad(path, False):
        return PhraseDictionaryTree(path, nscores, False)
    elif PhraseDictionaryTree.canLoad(path, True):
        return PhraseDictionaryTree(path, nscores, True)
    elif OnDiskWrapper.canLoad(path):
        return OnDiskWrapper(path, nscores)
    else:
        raise ValueError, '%s does not seem to be a valid table' % path
