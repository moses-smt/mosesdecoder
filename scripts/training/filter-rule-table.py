#!/usr/bin/env python

import sys

# Given a rule table (on stdin) and an input text, filter out rules that
# couldn't be used to parse the input and write the resulting rule table
# to stdout.  The input text is assumed to contain the same factors as
# the rule table and is assumed to be small (not more than a few thousand
# sentences): the current algorithm won't scale well to large input sets.
#
# The filtering algorithm considers a source RHS to be a sequence of
# words and gaps, which must match a sequence of words in one of the
# input sentences, with at least one input word per gap.  The NT labels
# are ignored, so for example a rule with the source RHS "the JJ dog"
# will be allowed if there's an input sequence "the slobbering dog," even
# if there's no rule to derive a JJ from "slobbering."  (If "slobbering"
# were an unknown word, the 'unknown-lhs' decoder option would allow it
# to take a number of NT labels, including JJ, with varying
# probabilities, so removing the rule would be a bad idea.)

def printUsage():
    sys.stderr.write("Usage: filter-rule-table.py INPUT")

def main():
    if len(sys.argv) != 2:
        printUsage()
        sys.exit(1)
    inputSentences = []
    for line in open(sys.argv[1]):
        inputSentences.append(line.split())
    filterRuleTable(sys.stdin, inputSentences)

def filterRuleTable(ruleTable, inputSentences):
    # Map each input word to a map from sentence indices to lists of
    # intra-sentence indices.
    occurrences = {}
    for i, sentence in enumerate(inputSentences):
        for j, word in enumerate(sentence):
            innerMap = occurrences.setdefault(word, {})
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
        i = findTerminal(rhs)
        if i == None:
            print line,
            prevRuleIncluded = True
            continue
        terminals = getTerminals(rhs)
        innerMap = occurrences.get(rhs[i], {})
        # Determine the sentences in which all terminals co-occur.
        sentences = set(innerMap.keys())
        for terminal in terminals[1:]:
            sentences &= set(occurrences.get(terminal, {}).keys())
        # Try to match rule in candidate sentences.
        match = False
        for sentenceIndex in sentences:
            indices = innerMap[sentenceIndex]
            for start in indices:
                if start < i:
                    continue
                if matchRule(rhs[i:], inputSentences[sentenceIndex], start):
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

def getTerminals(symbols):
    return [s for s in symbols if not isNT(s)]

def findTerminal(symbols):
    for i, symbol in enumerate(symbols):
        if not isNT(symbol):
            return i
    return None

def matchRule(rhsSuffix, sentence, start):
    assert(rhsSuffix[0] == sentence[start])
    if len(rhsSuffix) == 1:
        return True
    i = findTerminal(rhsSuffix[1:])
    if i == None:
        remainingNTs = len(rhsSuffix) - 1
        remainingWords = len(sentence) - start - 1
        return remainingWords >= remainingNTs
    i += 1
    if i == 1:
        # No intervening NTs: try to match next the word of sentence then
        # recursively match whatever's left.
        remainingWords = len(sentence) - start - 1
        if remainingWords > 0 and rhsSuffix[1] == sentence[start+1]:
            return matchRule(rhsSuffix[1:], sentence, start+1)
        else:
            return False
    else:
        # Search for the next terminal in the remainder of sentence (after
        # skipping one word per intervening NT).  If it's found, recursively
        # match whatever's left.
        j = None
        # (In the following slice, it's fine for start+i to be out of range:
        # we'll get an empty slice.)
        for k, word in enumerate(sentence[start+i:]):
            if word == rhsSuffix[i]:
                j = k
                break
        if j == None:
            return False
        else:
            return matchRule(rhsSuffix[i:], sentence, start+i+j)

if __name__ == "__main__":
    main()
