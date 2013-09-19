#!/usr/bin/env python

# Usage: extract-target-trees.py [FILE]
#
# Reads moses-chart's -T output from FILE or standard input and writes trees to
# standard output in Moses' XML tree format.

import re
import sys

class Tree:
    def __init__(self, label, children):
        self.label = label
        self.children = children
    def is_leaf(self):
        return len(self.children) == 0


class Derivation(list):
    def find_root(self):
        assert len(self) > 0
        root = None
        for hypothesis in self:
            if hypothesis.span[0] != 0:
                continue
            if root == None or hypothesis.span[1] > root.span[1]:
                root = hypothesis
        assert root
        return root

    def construct_target_tree(self):
        map = {}
        for hypothesis in self:
            map[hypothesis.span] = hypothesis
        root = self.find_root()
        return self._buildTree(root, map)

    def _buildTree(self, root, map):
        def escapeLabel(label):
            s = label.replace("&", "&amp;")
            s = s.replace("<", "&lt;")
            s = s.replace(">", "&gt;")
            return s

        # Build list of NT spans in source order...
        nonTermSpans = []
        for item in root.sourceSymbolInfo:
            span = item[0]
            if span in map: # In map iff symbol is NT
                nonTermSpans.append(span)
        nonTermSpans.sort()

        # ... then convert to target order.
        alignmentPairs = root.ntAlignments[:]
        alignmentPairs.sort()
        targetOrderNonTermSpans = {}
        for i, pair in enumerate(alignmentPairs):
            targetOrderNonTermSpans[pair[1]] = nonTermSpans[i]

        children = []
        numNonTerms = 0

        for i, symbol in enumerate(root.targetRHS):
            if i in targetOrderNonTermSpans:
                hyp = map[targetOrderNonTermSpans[i]]
                children.append(self._buildTree(hyp, map))
                numNonTerms += 1
            else:
                children.append(Tree(escapeLabel(symbol), []))

        assert numNonTerms == len(root.ntAlignments)

        return Tree(root.targetLHS, children)


class Hypothesis:
    def __init__(self):
        self.sentenceNum = None
        self.span = None
        self.sourceSymbolInfo = None
        self.targetLHS = None
        self.targetRHS = None
        self.ntAlignments = None

    def __str__(self):
        return str(self.id) + " " + str(self.component_scores)


def readDerivations(input, lineNum):
    prevSentenceNum = None
    derivation = Derivation()
    for line in input:
        lineNum += 1
        hypothesis = parseLine(line)
        if hypothesis.sentenceNum != prevSentenceNum:
            # We've started reading the next derivation...
            prevSentenceNum = hypothesis.sentenceNum
            if len(derivation):
                yield derivation, lineNum
                derivation = Derivation()
        derivation.append(hypothesis)
    if len(derivation):
        yield derivation, lineNum


# Extract the hypothesis components and return a Hypothesis object.
def parseLine(s):
    pattern = r"Trans Opt (\d+) " + \
              r"\[(\d+)\.\.(\d+)\]:" + \
              r"((?: \[\d+\.\.\d+\]=\S+  )+):" + \
              r" (\S+) ->" + \
              r"((?:\S+ )+):" + \
              r"((?:\d+-\d+ )*): pC="
    regexp = re.compile(pattern)
    match = regexp.match(s)
    if not match:
        sys.stderr.write("%s\n" % s)
    assert match
    group = match.groups()
    hypothesis = Hypothesis()
    hypothesis.sentenceNum = int(group[0]) + 1
    hypothesis.span = (int(group[1]), int(group[2]))
    hypothesis.sourceSymbolInfo = []
    for item in group[3].split():
        pattern = "\[(\d+)\.\.(\d+)\]=(\S+)"
        regexp = re.compile(pattern)
        match = regexp.match(item)
        assert(match)
        start, end, symbol = match.groups()
        span = (int(start), int(end))
        hypothesis.sourceSymbolInfo.append((span, symbol))
    hypothesis.targetLHS = group[4]
    hypothesis.targetRHS = group[5].split()
    hypothesis.ntAlignments = []
    for pair in group[6].split():
        match = re.match(r'(\d+)-(\d+)', pair)
        assert match
        ai = (int(match.group(1)), int(match.group(2)))
        hypothesis.ntAlignments.append(ai)
    return hypothesis


def tree_to_xml(tree):
    if tree.is_leaf():
        return tree.label
    else:
        s = '<tree label="%s"> ' % tree.label
        for child in tree.children:
            s += tree_to_xml(child)
            s += " "
        s += '</tree>'
        return s


def main():
    if len(sys.argv) > 2:
        sys.stderr.write("usage: %s [FILE]\n" % sys.argv[0])
        sys.exit(1)
    if len(sys.argv) == 1 or sys.argv[1] == "-":
        input = sys.stdin
    else:
        input = open(sys.argv[1])
    lineNum = 0
    for derivation, lineNum in readDerivations(input, lineNum):
        try:
            tree = derivation.construct_target_tree()
        except:
            msg = "error processing derivation at line %d\n" % lineNum
            sys.stderr.write(msg)
            raise
        print tree_to_xml(tree)


if __name__ == '__main__':
    main()
