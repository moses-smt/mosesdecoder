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

cdef class Production(object):
    """
    General class that represents a context-free production or a flat contiguous phrase.
    Note: we can't extend from tuple yet (Cython 0.17.1 does not support it), so a few protocols are implemented so that
    it feels like Production is a tuple.
    """

    cdef readonly bytes lhs
    cdef readonly tuple rhs

    def __init__(self, rhs, lhs = None):
        """
        :rhs right-hand side of the production (or the flat contiguous phrase) - sequence of strings
        :lhs left-hand side nonterminal (or None in the case of flat contiguous phrases)
        """
        self.rhs = tuple(rhs)
        self.lhs = lhs

    def __len__(self):
        return len(self.rhs)

    def __getitem__(self, key):
        if 0 <= key < len(self.rhs):
            return self.rhs[key]
        else:
            return IndexError, 'Index %s out of range' % str(key)

    def __iter__(self):
        for x in self.rhs:
            yield x

    def __contains__(self, item):
        return item in self.rhs

    def __reversed__(self):
        return reversed(self.rhs)

    def __hash__(self):
        return hash(self.rhs)

    def __str__(self):
        if self.lhs:
            return '%s -> %s' % (self.lhs, ' '.join(self.rhs))
        else:
            return ' '.join(self.rhs)

    def __repr__(self):
        return repr(self.as_tuple())

    def as_tuple(self, lhs_first = False):
        """
        Returns a tuple (lhs) + rhs or rhs + (lhs) depending on the flag 'lhs_first'.
        """
        if self.lhs:
            if lhs_first:
                return tuple([self.lhs]) + self.rhs
            else:
                return self.rhs + tuple([self.lhs])
        else:
            return self.rhs

    def __richcmp__(self, other, op):
        """
        The comparison uses 'as_tuple()', therefore in the CFG case, the lhs will be part of the production and it will be placed in the end
        (just to keep with Moses convention which has mostly to do with sorting for scoring on disk).
        """
        x = self.as_tuple()
        y = other.as_tuple()
        if op == 0:
            return x < y
        elif op == 1:
            return x <= y
        elif op == 2:
            return x == y
        elif op == 3:
            return x != y
        elif op == 4:
            return x > y
        elif op == 5:
            return x >= y

cdef class Alignment(list):
    """
    This represents a list of alignment points (pairs of integers).
    It should inherit from tuple, but that is not yet supported in Cython (as for Cython 0.17.1).
    """

    def __init__(self, alignment):
        if type(alignment) is str:
            pairs = []
            for point in alignment.split():
              s, t = point.split('-')
              pairs.append((int(s), int(t)))
            super(Alignment, self).__init__(pairs)
        elif type(alignment) in [list, tuple]:
            super(Alignment, self).__init__(alignment)
        else:
            ValueError, 'Cannot figure out pairs from: %s' % type(alignment)

    def __str__(self):
        return ' '.join('%d-%d' % (s, t) for s, t in self)

cdef class FValues(list):
    """
    This represents a list of feature values (floats).
    It should inherit from tuple, but that is not yet supported in Cython (as for Cython 0.17.1).
    """

    def __init__(self, values):
        super(FValues, self).__init__(values)

    def __str__(self):
        return ' '.join(str(x) for x in self)

cdef class TargetProduction(Production):
    """This class specializes production making it the target side of a translation rule.
    On top of lhs and rhs it comes with alignment information a tuple of real-valued features.
    """
    cdef readonly Alignment alignment
    cdef readonly FValues scores

    def __init__(self, rhs, scores, alignment = [], lhs = None):
        """
        :rhs right-hand side tokens (sequence of terminals and nonterminals)
        :scores tuple of real-valued features
        :alignment tuple of pairs of 0-based integers
        :lhs left-hand side nonterminal (None in phrase-based)
        """
        super(TargetProduction, self).__init__(rhs, lhs)
        self.scores = FValues(scores)
        self.alignment = Alignment(alignment)

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
            str(self.scores),
            str(self.alignment)))

    def __repr__(self):
        return repr((repr(self.rhs), repr(self.lhs), repr(self.scores), repr(self.alignment)))

cdef class QueryResult(list):

    cdef readonly Production source

    def __init__(self, source, targets = []):
        super(QueryResult, self).__init__(targets)
        self.source = source


cdef class DictionaryTree(object):

    @classmethod
    def canLoad(cls, path, bint wa = False):
        """Whether or not the path represents a valid table for that class."""
        raise NotImplementedError

    def query(self, line, converter = None, cmp = None, key = None):
        """
        Returns a list of target productions that translate a given source production
        :line query (string)
        :converter applies a transformation to the score (function)
        :cmp define it to get a sorted list (design it compatible with your converter)
        :key defines the key of the comparison
        :return QueryResult
        """
        raise NotImplementedError

cdef class PhraseDictionaryTree(DictionaryTree):
    """This class encapsulates a Moses::PhraseDictionaryTree for operations over
    binary phrase tables."""

    cdef cdictree.PhraseDictionaryTree* tree
    cdef readonly bytes path
    cdef readonly unsigned nscores
    cdef readonly bint wa
    cdef readonly bytes delimiters
    cdef readonly unsigned tableLimit

    def __cinit__(self, bytes path, unsigned tableLimit = 20, unsigned nscores = 5, bint wa = False, delimiters = ' \t'):
        """
        :path stem of the table, e.g europarl.fr-en is the stem for europar.fr-en.binphr.*
        :tableLimit maximum translations per source (defaults to 20 - use zero to impose no limit)
        :wa whether or not it has word-alignment information
        :delimiters for tokenization (defaults to space and tab)
        """

        if not PhraseDictionaryTree.canLoad(path, wa):
            raise ValueError, "'%s' doesn't seem a valid binary table." % path
        self.path = path
        self.tableLimit = tableLimit
        self.nscores = nscores #used to be passed to PhraseDictionaryTree, not used now
        self.wa = wa
        self.delimiters = delimiters
        self.tree = new cdictree.PhraseDictionaryTree()
        self.tree.NeedAlignmentInfo(wa)
        self.tree.Read(path)

    def __dealloc__(self):
        del self.tree

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

    cdef TargetProduction getTargetProduction(self, cdictree.StringTgtCand& cand, wa = None, converter = None):
        """Converts a StringTgtCandidate (c++ object) and possibly a word-alignment info (string) to a TargetProduction (python object)."""
        cdef list words = [cand.tokens[i].c_str() for i in xrange(cand.tokens.size())]
        cdef list scores = [score for score in cand.scores] if converter is None else [converter(score) for score in cand.scores]
        return TargetProduction(words, scores, wa)

    def query(self, line, converter = lambda x: log(x), cmp = lambda x, y: fsign(y.scores[2] - x.scores[2]), key = None):
        """
        Returns a list of target productions that translate a given source production
        :line query (string)
        :converter applies a transformation to the score (function) - defaults to the natural log (since by default binary phrase-tables store probabilities)
        :cmp define it to get a sorted list - defaults to sorting by t(e|f) (since by default binary phrase-tables are not sorted)
        :key defines the key of the comparison - defauls to none
        :return QueryResult
        """
        cdef bytes text = as_str(line)
        cdef vector[string] fphrase = cdictree.Tokenize(text, self.delimiters)
        cdef vector[cdictree.StringTgtCand]* rv = new vector[cdictree.StringTgtCand]()
        cdef vector[string]* wa = NULL
        cdef Production source = Production(f.c_str() for f in fphrase)
        cdef QueryResult results = QueryResult(source)

        if not self.wa:
            self.tree.GetTargetCandidates(fphrase, rv[0])
            results.extend([self.getTargetProduction(candidate, None, converter) for candidate in rv[0]])
        else:
            wa = new vector[string]()
            self.tree.GetTargetCandidates(fphrase, rv[0], wa[0])
            results.extend([self.getTargetProduction(rv[0][i], wa[0][i].c_str(), converter) for i in range(rv.size())])
            del wa
        del rv
        if cmp:
            results.sort(cmp=cmp, key=key)
        if self.tableLimit > 0:
            return QueryResult(source, results[0:self.tableLimit])
        else:
            return results

cdef class OnDiskWrapper(DictionaryTree):

    cdef condiskpt.OnDiskWrapper *wrapper
    cdef condiskpt.OnDiskQuery *finder
    cdef readonly bytes delimiters
    cdef readonly unsigned tableLimit

    def __cinit__(self, bytes path, unsigned tableLimit = 20, delimiters = ' \t'):
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

    cdef Production getSourceProduction(self, vector[string] ftokens):
        cdef list tokens = [f.c_str() for f in ftokens]
        return Production(tokens[:-1], tokens[-1])

    def query(self, line, converter = None, cmp = None, key = None):
        """
        Returns a list of target productions that translate a given source production
        :line query (string)
        :converter applies a transformation to the score (function) - defaults to None (since by default OnDiskWrapper store the ln(prob))
        :cmp define it to get a sorted list - defaults to None (since by default OnDiskWrapper is already sorted)
        :key defines the key of the comparison - defauls to none
        :return QueryResult
        """
        cdef bytes text = as_str(line)
        cdef vector[string] ftokens = cdictree.Tokenize(text, self.delimiters)
        cdef condiskpt.PhraseNode *node = <condiskpt.PhraseNode *>self.finder.Query(ftokens)
        if node == NULL:
            return []
        cdef Production source = self.getSourceProduction(ftokens)
        cdef condiskpt.TargetPhraseCollection ephrases = node.GetTargetPhraseCollection(self.tableLimit, self.wrapper[0])[0]
        cdef condiskpt.Vocab vocab = self.wrapper.GetVocab()
        cdef condiskpt.TargetPhrase ephr
        cdef condiskpt.Word e
        cdef unsigned i, j
        cdef QueryResult results = QueryResult(source)
        for i in xrange(ephrases.GetSize()):
            ephr = ephrases.GetTargetPhrase(i)
            words = [ephr.GetWord(j).GetString(vocab).c_str() for j in xrange(ephr.GetSize())]
            if converter is None:
                results.append(TargetProduction(words[:-1], ephr.GetScores(), ephr.GetAlign(), words[-1]))
            else:
                scores = tuple(ephr.GetScores())
                results.append(TargetProduction(words[:-1], (converter(score) for score in scores), ephr.GetAlign(), words[-1]))
        if cmp:
            results.sort(cmp=cmp, key=key)
        return results

def load(path, nscores, limit):
    """Finds out the correct implementation depending on the content of 'path' and returns the appropriate dictionary tree."""
    if PhraseDictionaryTree.canLoad(path, False):
        return PhraseDictionaryTree(path, limit, nscores, False)
    elif PhraseDictionaryTree.canLoad(path, True):
        return PhraseDictionaryTree(path, limit, nscores, True)
    elif OnDiskWrapper.canLoad(path):
        return OnDiskWrapper(path, limit)
    else:
        raise ValueError, '%s does not seem to be a valid table' % path
