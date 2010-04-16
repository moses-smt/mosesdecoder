#!/usr/bin/env python

# Author: Phil Williams

# Usage: filter-rule-table.py INPUT
#
# Given a rule table (on stdin) and an input text, filter out rules that
# couldn't be used in parsing the input and write the resulting rule table
# to stdout.  The input text is assumed to contain the same factors as
# the rule table and is assumed to be small (not more than a few thousand
# sentences): the current algorithm won't scale well to large input sets.
#
# The filtering algorithm considers a source RHS to be a sequence of
# words and gaps, which must match a sequence of words in one of the
# input sentences, with at least one input word per gap.  The NT labels
# are ignored, so for example a rule with the source RHS "the JJ dog"
# would be allowed if the sequence "the slobbering dog" occurs in one of
# the input sentences, even if there's no rule to derive a JJ from
# "slobbering."  (If "slobbering" were an unknown word, the 'unknown-lhs'
# decoder option would allow it to take a number of NT labels, likely
# including JJ, with varying probabilities, so removing the rule would
# be a bad idea.)

import sys

class NGram(tuple):
    pass

class Gap:
    def __init__(self, minSpan):
        self.minSpan = minSpan

    def getMinSpan(self):
        return self.minSpan

def printUsage():
    sys.stderr.write("Usage: filter-rule-table.py INPUT")

def main():
    if len(sys.argv) != 2:
        printUsage()
        sys.exit(1)
    N = 7
    inputSentences = []
    for line in open(sys.argv[1]):
        inputSentences.append(line.split())
    filterRuleTable(sys.stdin, inputSentences, N)

def filterRuleTable(ruleTable, inputSentences, N):
    # Map each input n-gram (n = 1..N) to a map from sentence indices to
    # lists of intra-sentence indices.
    occurrences = {}
    for i, sentence in enumerate(inputSentences):
        for n in range(1, N+1):
            for j in range(0, len(sentence)-n+1):
                ngram = NGram(sentence[j:j+n])
                innerMap = occurrences.setdefault(ngram, {})
                indices = innerMap.setdefault(i, [])
                indices.append(j)
    # Compare rules against input.
    prevRHS = None
    prevRuleIncluded = None
    for line in ruleTable:
        rhs = parseRule(line)
        if rhs == prevRHS:
            if prevRuleIncluded:
                print line,
            continue
        prevRHS = rhs
        # <s> and </s> can appear in glue rules.
        if rhs[0] == "<s>" or rhs[-1] == "</s>":
            print line,
            prevRuleIncluded = True
            continue
        segments = segmentRHS(rhs, N)
        ngramMaps = [occurrences.get(s, {}) for s in segments if isinstance(s, NGram)]
        if len(ngramMaps) == 0:
            print line,
            prevRuleIncluded = True
            continue
        # Determine the sentences in which all n-grams co-occur.
        sentences = set(ngramMaps[0].keys())
        for ngramMap in ngramMaps[1:]:
            sentences &= set(ngramMap.keys())
        # Try to match rule in candidate sentences.
        match = False
        for sentenceIndex in sentences:
            sentenceLength = len(inputSentences[sentenceIndex])
            for indexSeq in enumerateIndexSeqs(ngramMaps, sentenceIndex):
                if matchSegments(segments, indexSeq, sentenceLength):
                    print line,
                    match = True
                    break
            if match:
                break
        prevRuleIncluded = match

# Parse a line of the rule table and return the list of RHS source symbols.
def parseRule(line):
    cols = line.split(" ||| ")
    return cols[0].split()[:-1]

def isNT(symbol):
    return symbol[0] == '[' and symbol[-1] == ']'

def segmentRHS(rhs, N):
    segments = []
    terminals = []
    minGapWidth = 0
    for symbol in rhs:
        if isNT(symbol):
            if len(terminals) > 0:
                assert minGapWidth == 0
                segments.append(NGram(terminals))
                terminals = []
            minGapWidth += 1
        else:
            if minGapWidth > 0:
                assert len(terminals) == 0
                segments.append(Gap(minGapWidth))
                minGapWidth = 0
            terminals.append(symbol)
            if len(terminals) == N:
                segments.append(NGram(terminals))
                terminals = []
    if minGapWidth > 0:
        assert len(terminals) == 0
        segments.append(Gap(minGapWidth))
    elif len(terminals) > 0:
        segments.append(NGram(terminals))
    return segments

def matchSegments(segments, indexSeq, sentenceLength):
    assert len(segments) > 0
    firstSegment = segments[0]
    i = 0
    if isinstance(firstSegment, Gap):
        minPos = firstSegment.getMinSpan()
        maxPos = sentenceLength-1
    else:
        minPos = indexSeq[i] + len(firstSegment)
        i += 1
        maxPos = minPos
    for segment in segments[1:]:
        if isinstance(segment, Gap):
            if minPos + segment.getMinSpan() > sentenceLength:
                return False
            minPos = minPos + segment.getMinSpan()
            maxPos = sentenceLength-1
        else:
            pos = indexSeq[i]
            i += 1
            if pos < minPos or pos > maxPos:
                return False
            minPos = pos + len(segment)
            maxPos = minPos
    return True

def enumerateIndexSeqs(ngramMaps, sentenceIndex):
    assert len(ngramMaps) > 0
    if len(ngramMaps) == 1:
        for index in ngramMaps[0][sentenceIndex]:
            yield [index]
        return
    for index in ngramMaps[0][sentenceIndex]:
        for seq in enumerateIndexSeqs(ngramMaps[1:], sentenceIndex):
            if seq[0] > index:
                yield [index] + seq

if __name__ == "__main__":
    main()
